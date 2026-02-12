/**
 * cpu.c
 */
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

int Base = 0;
int PC = 0;
int IR0 = 0;
int IR1 = 0;
int AC = 0;
int MAR = 0;
int MBR = 0;

/**
 * required func to define for project 1
 * compute base + l_addr and return that as true memory address
 */
int mem_address(int l_addr)
{
    return Base + l_addr;
}

/**
 * required func to define for project 1
 * reads the instruction in memory at location addr, and place
 * the OP Code in IR0 and the argument (if applicable) in IR1
 */
void fetch_instruction(int addr)
{
    int *slot = mem_read(addr);
    if (slot == NULL) {
        IR0 = 0; IR1 = 0;
        return;
    }
    IR0 = slot[0];
    IR1 = slot[1];
}

/**
 * required func to define for project 1
 * execute the instruction in IR0/IR1
 * Opcode mapping:
 * 0  exit
 * 1  load_const (IR1 contains integer) -> AC = IR1
 * 2  move_from_mbr -> AC = MBR
 * 3  move_from_mar -> AC = MAR
 * 4  move_to_mbr   -> MBR = AC
 * 5  move_to_mar   -> MAR = AC
 * 6  load_at_addr  -> MBR = memory[ mem_address(MAR) ]
 * 7  write_at_addr -> memory[ mem_address(MAR) ] = MBR
 * 8  add           -> AC += MBR
 * 9  multiply      -> AC *= MBR
 * 10 and           -> AC = (AC!=0 && MBR!=0) ? 1 : 0
 * 11 or            -> AC = (AC!=0 || MBR!=0) ? 1 : 0
 * 12 ifgo addr     -> if (AC != 0) PC = addr - 1
 * 13 sleep         -> do nothing
 */
void execute_instruction(void)
{
    switch (IR0) {
        case 0: /* exit/halt: do nothing here; clock_cycle will stop */
            break;

        case 1: /* load_const: IR1 contains immediate integer */
            AC = IR1;
            PC++;
            break;

        case 2: /* move_from_mbr */
            AC = MBR;
            PC++;
            break;

        case 3: /* move_from_mar */
            AC = MAR;
            PC++;
            break;

        case 4: /* move_to_mbr */
            MBR = AC;
            PC++;
            break;

        case 5: /* move_to_mar */
            MAR = AC;
            PC++;
            break;

        case 6: /* load_at_addr: use MAR as logical address */
        {
            int phys = mem_address(MAR);
            int *slot = mem_read(phys);
            if (slot) MBR = slot[0]; else MBR = 0;
            PC++;
            break;
        }

        case 7: /* write_at_addr: write MBR into memory at MAR */
        {
            int phys = mem_address(MAR);
            int data[2] = {MBR, 0};
            mem_write(phys, data);
            PC++;
            break;
        }

        case 8: /* add */
            AC = AC + MBR;
            PC++;
            break;

        case 9: /* multiply */
            AC = AC * MBR;
            PC++;
            break;

        case 10: /* and (logical) */
            AC = (AC != 0 && MBR != 0) ? 1 : 0;
            PC++;
            break;

        case 11: /* or (logical) */
            AC = (AC != 0 || MBR != 0) ? 1 : 0;
            PC++;
            break;

        case 12: /* ifgo addr */
            if (AC != 0) {
                PC = IR1 - 1; /* -1 so that after the normal PC++ it will be at addr */
            }
            PC++;
            break;

        case 13: /* sleep (Do nothing) */
            PC++;
            break;

        default:
            fprintf(stderr, "Error: invalid opcode %d\n", IR0);
            PC++;
            break;
    }
}

/**
 * required func to define for project 1
 * implements a single clock cycle
 * returns 0 if case 0 (exit), else 1
 */
int clock_cycle(void)
{
    int abs_addr = mem_address(PC);
    fetch_instruction(abs_addr);

    if (IR0 == 0) {
        return 0;
    }

    execute_instruction();
    return 1;
}

/**
 * required fun to define for project 2
 * recieves as input a register_struct with new values to write into the CPU registers
 * returns the old 
 */
register_struct context_switch(register_struct new_vals)
{
    register_struct old_vals;

    old_vals.Base = Base;
    old_vals.PC = PC;
    old_vals.IR0 = IR0;
    old_vals.IR1 = IR1;
    old_vals.AC = AC;
    old_vals.MAR = MAR;
    old_vals.MBR = MBR;

    Base = new_vals.Base;
    PC = new_vals.PC;
    IR0 = new_vals.IR0;
    IR1 = new_vals.IR1;
    AC = new_vals.AC;
    MAR = new_vals.MAR;
    MBR = new_vals.MBR;

    return old_vals;
}
