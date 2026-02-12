#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROCESSES 1024

extern int time_quantum;

typedef struct PCB {
    int pid;
    int base;
    int size;
    uint32_t pc;
    uint32_t registers[8];
    uint32_t sp;
    uint32_t flags;
} PCB;

typedef struct ReadyNode {
    PCB *pcb;
    struct ReadyNode *next;
} ReadyNode;

void new_process(int base, int size);
void next_process(void);
int schedule(int cycle_num, int process_status);

void scheduler_context_switch(void);
int ready_queue_empty(void);
void remove_process_from_ready(int pid);
int get_current_pid(void);
int scheduler_get_free_pid(void);
void create_process_with_pid(int pid, int base, int size);

#ifdef __cplusplus
}
#endif

#endif
