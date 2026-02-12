/**
 * cpu.h
 */
#ifndef CPU_H
#define CPU_H

extern int Base;
extern int PC;
extern int IR0;
extern int IR1;
extern int AC;
extern int MAR;
extern int MBR;

void fetch_instruction(int addr);
void execute_instruction(void);
int mem_address(int l_addr);
int clock_cycle(void);

typedef struct register_struct {
	int Base;
	int PC;
	int IR0;
	int IR1;
	int AC;
	int MAR;
	int MBR;
} register_struct;

register_struct context_switch(register_struct new_vals);

#endif
