/**
 * disk.h
 */
#ifndef DISK_H
#define DISK_H

void load_prog(char *fname, int addr);

int* translate(char *instruction);

void load_programs(char list_fname[]);

#endif
