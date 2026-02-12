/*
 * smm.c
 * Simple Memory Manager (dynamic partitioning, first-fit)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smm.h"

/* Try to respect an external memory size if provided; default to 256 */
#ifndef MEM_SIZE
#define MEM_SIZE 1024
#endif

struct hole {
    int base;
    int size;
    struct hole *next;
};

/* Head of holes linked list (sorted by base address) */
static struct hole *holes_head = NULL;

/* Allocation table: [row][0]=pid, [row][1]=base, [row][2]=size */
static int alloc_table[256][3];

/* Count of times a new hole is created (global as required) */
static int new_hole_count = 0;
static int smm_initialized = 0;

static void smm_init(void);
void print_new_hole_count(void) { printf("SMM: new holes created: %d\n", new_hole_count); }

static void smm_init(void)
{
    if (smm_initialized) return;
    /* initialize allocation table to zeros */
    memset(alloc_table, 0, sizeof(alloc_table));

    /* start with one big hole covering memory */
    holes_head = (struct hole*)malloc(sizeof(struct hole));
    if (!holes_head) {
        fprintf(stderr, "SMM: failed to initialize hole list (malloc)\n");
        exit(1);
    }
    holes_head->base = 0;
    holes_head->size = MEM_SIZE;
    holes_head->next = NULL;

    /* register at-exit printer for new_hole_count */
    atexit(print_new_hole_count);

    smm_initialized = 1;
}

int find_empty_row(void)
{
    smm_init();
    for (int i = 0; i < 256; ++i) {
        if (alloc_table[i][2] == 0) return i;
    }
    return -1;
}

int find_hole(int size)
{
    smm_init();
    struct hole *prev = NULL;
    struct hole *cur = holes_head;
    while (cur) {
        if (cur->size >= size) {
            int base = cur->base;
            if (cur->size == size) {
                /* remove this hole */
                if (prev) prev->next = cur->next;
                else holes_head = cur->next;
                free(cur);
            } else {
                /* shrink hole from front */
                cur->base += size;
                cur->size -= size;
            }
            return base;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1; /* no suitable hole */
}

int allocate(int pid, int size)
{
    smm_init();
    if (size <= 0) return 0;

    int row = find_empty_row();
    if (row == -1) {
        fprintf(stderr, "SMM: allocation failed for PID %d (no free table row)\n", pid);
        return 0;
    }

    int base = find_hole(size);
    if (base == -1) {
        fprintf(stderr, "SMM: allocation failed for PID %d (no hole large enough for %d)\n", pid, size);
        return 0;
    }

    /* fill allocation table row */
    alloc_table[row][0] = pid;
    alloc_table[row][1] = base;
    alloc_table[row][2] = size;

    return 1; /* success */
}

void remove_hole(int base)
{
    smm_init();
    struct hole *prev = NULL;
    struct hole *cur = holes_head;
    while (cur) {
        if (cur->base == base) {
            if (prev) prev->next = cur->next;
            else holes_head = cur->next;
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void merge_holes(void)
{
    smm_init();
    struct hole *cur = holes_head;
    while (cur && cur->next) {
        struct hole *n = cur->next;
        if (cur->base + cur->size == n->base) {
            /* merge n into cur */
            cur->size += n->size;
            cur->next = n->next;
            free(n);
            /* continue without advancing cur to check for further merges */
        } else {
            cur = cur->next;
        }
    }
}

void add_hole(int base, int size)
{
    smm_init();
    if (size <= 0) return;
    struct hole *node = (struct hole*)malloc(sizeof(struct hole));
    if (!node) {
        fprintf(stderr, "SMM: add_hole malloc failed\n");
        return;
    }
    node->base = base;
    node->size = size;
    node->next = NULL;

    /* insert sorted by base */
    struct hole *prev = NULL;
    struct hole *cur = holes_head;
    while (cur && cur->base < base) {
        prev = cur;
        cur = cur->next;
    }
    if (prev == NULL) {
        node->next = holes_head;
        holes_head = node;
    } else {
        node->next = prev->next;
        prev->next = node;
    }

    new_hole_count++;
    merge_holes();
}

void deallocate(int pid)
{
    smm_init();
    for (int i = 0; i < 256; ++i) {
        if (alloc_table[i][0] == pid && alloc_table[i][2] > 0) {
            int base = alloc_table[i][1];
            int size = alloc_table[i][2];
            /* mark table row free */
            alloc_table[i][0] = 0;
            alloc_table[i][1] = 0;
            alloc_table[i][2] = 0;
            /* add a hole */
            add_hole(base, size);
            return;
        }
    }
    fprintf(stderr, "SMM: deallocate called for unknown PID %d\n", pid);
}

int get_base_address(int pid)
{
    smm_init();
    for (int i = 0; i < 256; ++i) {
        if (alloc_table[i][0] == pid && alloc_table[i][2] > 0) return alloc_table[i][1];
    }
    return -1;
}

int is_allowed_address(int pid, int addr)
{
    smm_init();
    for (int i = 0; i < 256; ++i) {
        if (alloc_table[i][0] == pid && alloc_table[i][2] > 0) {
            int base = alloc_table[i][1];
            int size = alloc_table[i][2];
            if (addr >= base && addr < base + size) return 1;
            else return 0;
        }
    }
    return 0; /* pid not found */
}
