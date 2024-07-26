// 323073734
#include "mem_sim.h"


int main(int argc, char *argv[]) {
    char val;
    sim_database* mem_sim = init_system ("exec_file", "swap_file" ,40,40,120);
    val = load (mem_sim , 44);
    val = load (mem_sim , 46);
    val = load (mem_sim , 2);
    store(mem_sim , 50, 'X');
    val = load (mem_sim ,16);
    store (mem_sim ,70, 'A');
    store(mem_sim ,55,'Y');
    store (mem_sim ,15,'Z');
    val = load (mem_sim ,23);
    print_memory(mem_sim);
    print_swap(mem_sim);
    clear_system(mem_sim);
    if (val == '1')
        val++;
    return 0;
}