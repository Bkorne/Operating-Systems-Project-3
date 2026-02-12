/**
 * scheduler.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "cpu.h"

int time_quantum = 10;

static PCB process_table[MAX_PROCESSES];
static int occupied[MAX_PROCESSES] = {0};

static ReadyNode *ready_head = NULL;
static ReadyNode *ready_tail = NULL;

static PCB *current = NULL;

static int last_cycle_checkpoint = 0;

//find the first free pid in the process table
static int find_free_pid(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (!occupied[i]) return i;
    }
    return -1;
}

/* Public wrapper to get a free PID (does not mark it occupied). */
int scheduler_get_free_pid(void) {
    return find_free_pid();
}

//adda PCB to the end of the ready queue
static void enqueue_ready(PCB *pcb) {
    ReadyNode *n = (ReadyNode *)malloc(sizeof(ReadyNode));
    if (!n) {
        fprintf(stderr, "enqueue_ready: out of memory\n");
        return;
    }
    n->pcb = pcb;
    n->next = NULL;
    if (ready_tail == NULL) {
        ready_head = ready_tail = n;
    } else {
        ready_tail->next = n;
        ready_tail = n;
    }
}

//remove and return the head node of the ready queue
static ReadyNode *dequeue_ready_node(void) {
    if (!ready_head) return NULL;
    ReadyNode *n = ready_head;
    ready_head = ready_head->next;
    if (ready_head == NULL) ready_tail = NULL;
    n->next = NULL;
    return n;
}

/**
 * required func to define for project 2
 * creates a new process and adds it to the 
 * process table, and enqueues it to the ready queue
 */
void new_process(int base, int size) {
    int pid = find_free_pid();
    if (pid < 0) {
        fprintf(stderr, "new_process: process table full\n");
        return;
    }

    PCB *p = &process_table[pid];
    occupied[pid] = 1;

    p->pid = pid;
    p->base = base;
    p->size = size;
    p->pc = 0;
    p->sp = 0;
    p->flags = 0;
    memset(p->registers, 0, sizeof(p->registers));

    enqueue_ready(p);

    if (current == NULL) {
        scheduler_context_switch();
        last_cycle_checkpoint = 0;
    }
}

/* Create a process using a supplied PID. This marks the PID occupied and
 * initializes the PCB, then enqueues it on the ready queue.
 */
void create_process_with_pid(int pid, int base, int size) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        fprintf(stderr, "create_process_with_pid: invalid pid %d\n", pid);
        return;
    }
    if (occupied[pid]) {
        fprintf(stderr, "create_process_with_pid: pid %d already occupied\n", pid);
        return;
    }

    PCB *p = &process_table[pid];
    occupied[pid] = 1;

    p->pid = pid;
    p->base = base;
    p->size = size;
    p->pc = 0;
    p->sp = 0;
    p->flags = 0;
    memset(p->registers, 0, sizeof(p->registers));

    enqueue_ready(p);

    if (current == NULL) {
        scheduler_context_switch();
        last_cycle_checkpoint = 0;
    }
}

/**
 * required func to define for project 2
 * takes item from the front of the ready queue and puts it at the end
 */
void next_process(void) {
    if (!ready_head || !ready_head->next) return; /* 0 or 1 element -> nothing to rotate */
    ReadyNode *first = dequeue_ready_node();
    // put it at tail
    if (first) {
        if (ready_tail == NULL) {
            ready_head = ready_tail = first;
        } else {
            ready_tail->next = first;
            ready_tail = first;
            ready_tail->next = NULL;
        }
    }
}

void scheduler_context_switch(void) {
    if (!ready_head) {
        current = NULL;
        return;
    }

    PCB *new_pcb = ready_head->pcb;
    PCB *old_pcb = current;

    register_struct new_vals;
    new_vals.Base = new_pcb->base;
    new_vals.PC = (int)new_pcb->pc;
    new_vals.AC = (int)new_pcb->registers[0];
    new_vals.MAR = (int)new_pcb->registers[1];
    new_vals.MBR = (int)new_pcb->registers[2];
    new_vals.IR0 = (int)new_pcb->registers[3];
    new_vals.IR1 = (int)new_pcb->registers[4];

    register_struct old_vals = context_switch(new_vals);

    if (old_pcb) {
        old_pcb->base = old_vals.Base;
        old_pcb->pc = (uint32_t)old_vals.PC;
        old_pcb->registers[0] = (uint32_t)old_vals.AC;
        old_pcb->registers[1] = (uint32_t)old_vals.MAR;
        old_pcb->registers[2] = (uint32_t)old_vals.MBR;
        old_pcb->registers[3] = (uint32_t)old_vals.IR0;
        old_pcb->registers[4] = (uint32_t)old_vals.IR1;
    }

    current = new_pcb;
}

static void remove_head_process(void) {
    ReadyNode *n = dequeue_ready_node();
    if (!n) return;
    PCB *p = n->pcb;
    occupied[p->pid] = 0;
    free(n);
}

int ready_queue_empty(void) {
    return ready_head == NULL;
}

/* Remove a process with the given PID from the ready queue.
 * If the process is found, remove its ReadyNode, mark its table slot free,
 * free the node, and return. If not found, do nothing.
 */
void remove_process_from_ready(int pid) {
    ReadyNode *prev = NULL;
    ReadyNode *cur = ready_head;
    while (cur) {
        if (cur->pcb && cur->pcb->pid == pid) {
            /* unlink node */
            if (prev) prev->next = cur->next;
            else ready_head = cur->next;
            if (cur == ready_tail) {
                ready_tail = prev;
            }

            /* mark process table entry free and free node */
            occupied[pid] = 0;
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

/* Return PID of currently running process, or -1 if none. */
int get_current_pid(void) {
    if (!current) return -1;
    return current->pid;
}

/**
 * required func to define for project 2
 * recieves as input the number of clock cycles
 * calls next_process followed by context switch if time quantum expires
 * also recieves as input the process_status returned by the clock_cycle
 * returns 0 if there is no process to run, 1 otherwise
 */
int schedule(int cycle_num, int process_status) {
    if (ready_queue_empty()) {
        current = NULL;
        return 0;
    }

    if (process_status == 0) {
        remove_head_process();
        if (ready_queue_empty()) {
            current = NULL;
            return 0;
        }
        scheduler_context_switch();
        last_cycle_checkpoint = cycle_num; //start new quantum
        return 1;
    }

    // if quantum expired, rotate queue and pick next
    if ((cycle_num - last_cycle_checkpoint) >= time_quantum) {
        next_process();
        scheduler_context_switch();
        last_cycle_checkpoint = cycle_num;
    }

    return 1;
}

PCB *scheduler_get_current(void) {
    return current;
}
