/*
#include <fcntl.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include "mem_sim.h"

 int main(void) {
     struct sim_database* mem_sim = init_system("exec_file", "swap_file"
             ,40,40,120);
     store(mem_sim, 70, 'b');
    load(mem_sim, 20);

    load(mem_sim, 33);
     load(mem_sim, 42);
     load(mem_sim, 51);
     load(mem_sim, 60);
//    store(mem_sim, 40, 'b');
//    store(mem_sim, 50, 'c');
//    store(mem_sim, 60, 'd');
//     store(mem_sim, 70, 'e');
//     store(mem_sim, 80, 'f');
//     store(mem_sim, 90, 'z');
//     printf("[%c]\n", load(mem_sim, 72));
     //load(mem_sim, 70);

     print_memory(mem_sim);
     print_page_table(mem_sim);
     print_swap(mem_sim);
     //printf("%c\n",load(mem_sim, 17));
     clear_system(mem_sim);
 }
//
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "mem_sim.h"
#include <sys/stat.h>

#define TEXT_SIZE 40
#define EXEC_FILE "exec_file"
#define SWAP_FILE "swap_file"
#define EXEC_CONTENT "aaaaaaaabbbbbbbbccccccccddddddddeeeeeeeeffffffffgggggggghhhhhhhhiiiiiiiijjjjjjjj"

// Initialize the executable file with given content
void initialize_exec_file() {
    int fd = open(EXEC_FILE, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening executable file");
        exit(EXIT_FAILURE);
    }
    if (write(fd, EXEC_CONTENT, 80) != 80) { // 80 bytes of data
        perror("Error writing to executable file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
}

// Perform random tests on the memory simulation system
void random_tests(struct sim_database *mem_sim, int num_tests) {
    int max_address = 200; // Assuming we are dealing with 100 addresses
    srand(time(NULL));

    for (int i = 0; i < num_tests; i++) {
        int random_address = rand() % max_address;
        char random_value = 'A' + (rand() % 26); // Random uppercase letter

        // Perform store operation
        printf("Test %d: Storing value %c at address %d\n", i+1, random_value, random_address);
        store(mem_sim, random_address, random_value);

        // Perform load operation and verify
        char loaded_value = load(mem_sim, random_address);
        printf("Test %d: Loaded value %c from address %d\n", i+1, loaded_value, random_address);

        if (loaded_value != random_value) {
            if (random_address < TEXT_SIZE) {
                printf("Test %d: Can't store in address %d. Skipping to the next test...\n", i + 1, random_address);
                continue;
            }
            printf("Test failed at iteration %d: expected %c, got %c\n", i+1, random_value, loaded_value);
            exit(EXIT_FAILURE);
        }

        // Print current state of memory, swap file, and page table
//        print_memory(mem_sim);
//        print_swap(mem_sim);
//        print_page_table(mem_sim);
    }

    printf("All random tests passed!\n");
}

int main() {
    int num_tests;
    struct sim_database *mem_sim;

    // Initialize the exec file with given content
    initialize_exec_file();

    // Get the number of tests from the user
    printf("Enter the number of random tests to perform: ");
    scanf("%d", &num_tests);

    // Initialize the memory simulation system
    mem_sim = init_system(EXEC_FILE, SWAP_FILE, 40, 40, 120);

    // Perform random tests
    random_tests(mem_sim, num_tests);

    // Clear the system resources
    clear_system(mem_sim);

    return 0;
}

