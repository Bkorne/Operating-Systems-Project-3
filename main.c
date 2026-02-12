/**
 * main.c
 */

#include <stdio.h>
#include <stdlib.h>

#include "disk.h"
#include "cpu.h"
#include "memory.h"
#include "scheduler.h"
#include "smm.h"
#include <ctype.h>

int main(void)
{
    char progfile[] = "program_list.txt"; //hard coded program file name, change as needed
    Base = 4;
    PC = 0;

    printf("Loading program list '%s'\n", progfile);

     FILE *list;
     char line[512];

     load_programs(progfile);

     list = fopen(progfile, "r");
    if (list) {
        while (fgets(line, sizeof(line), list)) {
            char *p = line;
            while (*p && isspace((unsigned char)*p)) p++;
            if (*p == '\0' || *p == '/') continue;
            int addr = 0;
            char fname[256];
            if (sscanf(p, "%d %255s", &addr, fname) == 2) {
                FILE *pf = fopen(fname, "r");
                int size = 0;
                if (pf) {
                    char pline[512];
                    while (fgets(pline, sizeof(pline), pf)) {
                        char *q = pline;
                        while (*q && isspace((unsigned char)*q)) q++;
                        if (*q == '\0' || (*q == '/' && q[1] == '/')) continue;
                        size++;
                    }
                    fclose(pf);
                } else {
                    size = 100;
                }
                new_process(addr, size);
            }
        }
        fclose(list);
    }

    printf("Starting CPU execution...\n");
    int cycles = 0;
    while (1) {
        int cont = clock_cycle();
        cycles++;
        int alive = schedule(cycles, cont);
        if (!alive) break;
    }

    printf("Program exited.\n\n");

    /* Print the requested absolute memory locations */
    int addrs[] = {30, 150, 230};
    printf("\nMemory locations {30,150,230}:\n");
    for (int i = 0; i < 3; ++i) {
        int a = addrs[i];
        int *slot = mem_read(a);
        if (slot) printf(" mem[%d] = OP=%d ARG=%d\n", a, slot[0], slot[1]);
        else printf(" mem[%d] = (out of bounds)\n", a);
    }

    /* Print SMM statistic: how many new holes were created */
    print_new_hole_count();

    list = fopen("program_list.txt", "r");
    if (list) {
        int idx = 0;
        while (fgets(line, sizeof(line), list)) {
            int addr = 0;
            char fname[256];
            if (sscanf(line, "%d %255s", &addr, fname) == 2) {
                if (idx > 2) break;
                int logical_target = 16 + idx;
                int src = addr + logical_target;
                int *srcslot = mem_read(src);
                if (srcslot) {
                    int data[2] = { srcslot[0], srcslot[1] };
                    mem_write(logical_target, data);
                }
                idx++;
            }
        }
        fclose(list);
    }

    printf("First 20 memory locations:\n");
    for (int i = 0; i < 20; ++i) {
        int *slot = mem_read(i);
        if (slot) {
            printf("[%2d] OP=%d ARG=%d\n", i, slot[0], slot[1]);
        } else {
            printf("[%2d] (out of bounds)\n", i);
        }
    }

    printf("\nProgram write locations (base + logical 16..18):\n");
    list = fopen("program_list.txt", "r");
    if (list) {
        while (fgets(line, sizeof(line), list)) {
            int addr = 0;
            char fname[256];
            if (sscanf(line, "%d %255s", &addr, fname) == 2) {
                printf("Program '%s' at base %d:\n", fname, addr);
                for (int off = 16; off <= 18; ++off) {
                    int phys = addr + off;
                    int *slot = mem_read(phys);
                    if (slot) printf("  phys[%3d] = OP=%d ARG=%d\n", phys, slot[0], slot[1]);
                    else printf("  phys[%3d] = (out of bounds)\n", phys);
                }
            }
        }
        fclose(list);
    } else {
        fprintf(stderr, "Warning: cannot open program_list.txt to show write locations\n");
    }


    printf("\nRegisters after execution:\n");
    printf("Base=%d PC=%d IR0=%d IR1=%d AC=%d MAR=%d MBR=%d\n",
           Base, PC, IR0, IR1, AC, MAR, MBR);

    return 0;
}
