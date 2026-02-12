/*
 * smm.h
 * Simple Memory Manager (SMM) interface
 */
#ifndef SMM_H
#define SMM_H

int allocate(int pid, int size);
void deallocate(int pid);
void add_hole(int base, int size);
void remove_hole(int base);
void merge_holes(void);
int find_hole(int size);
int get_base_address(int pid);
int find_empty_row(void);
int is_allowed_address(int pid, int addr);
void print_new_hole_count(void);

#endif
