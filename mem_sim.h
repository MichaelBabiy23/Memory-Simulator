// 323073734

#ifndef OSHOMEWORK4_MEM_SIM_H
#define OSHOMEWORK4_MEM_SIM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PAGE_SIZE 8
#define NUM_OF_PAGES 25
#define MEMORY_SIZE 40
#define SWAP_SIZE 200

typedef struct {
    unsigned int V; // valid
    unsigned int D; // dirty
    unsigned int P; // permission
    unsigned int frame_swap; // frame or swap number
} page_descriptor;

typedef struct {
    page_descriptor page_table[NUM_OF_PAGES]; // page table
    int swapfile_fd; // swap file descriptor
    int program_fd; // executable file descriptor
    char main_memory[MEMORY_SIZE]; // main memory
    int text_size;
    int data_size;
    int bss_heap_stack_size;
} sim_database;

sim_database* init_system(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_heap_stack_size);
char load(sim_database* mem_sim, int address);
void store(sim_database* mem_sim, int address, char value);
void print_memory(sim_database* mem_sim);
void print_swap(sim_database* mem_sim);
void print_page_table(sim_database* mem_sim);
void clear_system(sim_database* mem_sim);


#endif //OSHOMEWORK4_MEM_SIM_H
