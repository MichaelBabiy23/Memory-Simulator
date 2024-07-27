#include "mem_sim.h"

int total_frames = 0;

/**
 * @brief Initializes the simulation database.
 *
 * @param exe_file_name The name of the executable file.
 * @param swap_file_name The name of the swap file.
 * @param text_size The size of the text segment.
 * @param data_size The size of the data segment.
 * @param bss_heap_stack_size The size of the BSS, heap, and stack segments.
 * @return sim_database* Pointer to the initialized simulation database.
 */
sim_database* init_system(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_heap_stack_size) {
    sim_database* mem_sim = (sim_database*)malloc(sizeof(sim_database));
    if (!mem_sim) {
        perror("Failed to allocate memory for sim_database");
        return NULL;
    }

    mem_sim->program_fd = open(exe_file_name, O_RDONLY);
    if (mem_sim->program_fd < 0) {
        perror("Failed to open executable file");
        free(mem_sim);
        return NULL;
    }

    mem_sim->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (mem_sim->swapfile_fd < 0) {
        perror("Failed to open swap file");
        close(mem_sim->program_fd);
        free(mem_sim);
        return NULL;
    }

    // Initialize swap file with empty pages
    char empty_page[PAGE_SIZE];
    memset(empty_page, '0', PAGE_SIZE); // <-- this line was changed for clearer initialization

    for (int i = 0; i < SWAP_SIZE / PAGE_SIZE; i++) {
        if (write(mem_sim->swapfile_fd, empty_page, PAGE_SIZE) != PAGE_SIZE) {
            perror("Failed to initialize swap file with empty pages");
            close(mem_sim->program_fd); // <-- this line was changed for better resource management in case of an error
            close(mem_sim->swapfile_fd); // <-- this line was changed for better resource management in case of an error
            free(mem_sim); // <-- this line was changed for better resource management in case of an error
            return NULL;
        }
    }


    mem_sim->text_size = text_size;
    mem_sim->data_size = data_size;
    mem_sim->bss_heap_stack_size = bss_heap_stack_size;

    memset(mem_sim->main_memory, '0', MEMORY_SIZE);

    for (int i = 0; i < NUM_OF_PAGES; i++) {
        mem_sim->page_table[i].V = 0;
        mem_sim->page_table[i].D = 0;
        mem_sim->page_table[i].P = (i < text_size / PAGE_SIZE) ? 1 : 0;
        mem_sim->page_table[i].frame_swap = -1;
    }
    total_frames = 0;
    return mem_sim;
}

/**
 * Loads a page from the swap file, executable file, or allocates a new page
 * based on the page type and validity.
 *
 * @param mem_sim Pointer to the simulation database structure
 * @param page_number The number of the page to be loaded
 */
void load_page(sim_database *mem_sim, int page_number) {
    int current_frame = total_frames % (MEMORY_SIZE / PAGE_SIZE);
    unsigned int free_page = 0;
    int found = 0;

    // Check if a free frame exists
    if (total_frames >= MEMORY_SIZE / PAGE_SIZE) {
        for (int k = 0; k < NUM_OF_PAGES; k++)
        {
            if (mem_sim->page_table[k].frame_swap == current_frame && mem_sim->page_table[k].V == 1) {
                free_page = k;
                found = 1;
                break;
            }
        }
        if (mem_sim->page_table[free_page].D) { // If the page is dirty, write it to the swap file
            char temp_buffer[PAGE_SIZE];
            int swap_index = -1;
            for (int j = 0; j < (SWAP_SIZE / PAGE_SIZE); j++) {
                lseek(mem_sim->swapfile_fd, PAGE_SIZE * j, SEEK_SET);
                if (read(mem_sim->swapfile_fd, temp_buffer, PAGE_SIZE) == -1) {
                    perror("READ");
                    return;
                }
                char temp[PAGE_SIZE];
                memset(temp, '0', PAGE_SIZE);
                if (strncmp(temp_buffer, temp, PAGE_SIZE) == 0) {
                    swap_index = j;
                    lseek(mem_sim->swapfile_fd, -PAGE_SIZE, SEEK_CUR);
                    break;
                }
            }
            if (swap_index == -1) {
                perror("No empty swap space found");
                return;
            }
            if (write(mem_sim->swapfile_fd, mem_sim->main_memory + current_frame * PAGE_SIZE, PAGE_SIZE) != PAGE_SIZE) {
                perror("Error writing");
                return;
            }
            mem_sim->page_table[free_page].frame_swap = swap_index; // <-- this line was changed for updating frame_swap correctly
        }
    }
    // Check if the page is in the swap file
    if (mem_sim->page_table[page_number].D == 1) {
        // Load the page from the swap file

        if (lseek(mem_sim->swapfile_fd, mem_sim->page_table[page_number].frame_swap * PAGE_SIZE, SEEK_SET) == -1) {
            perror("lseek error");
            return;
        }
        if (read(mem_sim->swapfile_fd, mem_sim->main_memory + current_frame * PAGE_SIZE, PAGE_SIZE) == -1) {
            perror("read error");
            return;
        }
        char empty_page[PAGE_SIZE];
        for (int i = 0; i < PAGE_SIZE; i++) {
            empty_page[i] = '0';
        }
        write(mem_sim->swapfile_fd, empty_page, PAGE_SIZE);
    } else {
        // Check if the page is from the exec_file
        if (page_number * PAGE_SIZE < mem_sim->text_size + mem_sim->data_size) {
            // Load the page from the executable file
            if (lseek(mem_sim->program_fd, page_number * PAGE_SIZE, SEEK_SET) == -1) {
                perror("lseek error");
                return;
            }
            if (read(mem_sim->program_fd, mem_sim->main_memory + current_frame * PAGE_SIZE, PAGE_SIZE) == -1) {
                perror("read error");
                return;
            }

        } else {
            // Allocate a new page for heap, stack, or bss (read-write)
            memset(mem_sim->main_memory + current_frame * PAGE_SIZE, '0', PAGE_SIZE);
        }

    }
    if (found == 1) {
        mem_sim->page_table[free_page].V = 0;
        if (mem_sim->page_table[free_page].D == 0)
            mem_sim->page_table[free_page].frame_swap = -1;
    }
    // Update the page table
    mem_sim->page_table[page_number].V = 1;
    mem_sim->page_table[page_number].frame_swap = current_frame;

    // Increment total_frames for the next page load
    total_frames++;
}

/**
 * @brief Loads a byte from the specified address.
 *
 * @param mem_sim The simulation database.
 * @param address The address to load from.
 * @return char The loaded byte.
 */
char load(sim_database* mem_sim, int address) {
    int PAGE_SIZE_LOG2 = 0;
    int temp = PAGE_SIZE;
    while (temp > 1) {
        temp >>= 1;
        PAGE_SIZE_LOG2++;
    }
    int page_number = address >> PAGE_SIZE_LOG2;  // Shift right by PAGE_SIZE_LOG2
    int offset = address & (PAGE_SIZE - 1);       // AND with PAGE_SIZE - 1

    if (page_number >= NUM_OF_PAGES || page_number < 0) {
        fprintf(stderr, "ERR\n");
        return '\0';
    }

    // Check if the page is not valid
    if (!mem_sim->page_table[page_number].V) {
        load_page(mem_sim, page_number);
    }

    // Get the physical address
    int physical_address = mem_sim->page_table[page_number].frame_swap * PAGE_SIZE + offset;

    // Return the value from the physical memory
    return mem_sim->main_memory[physical_address];
}


/**
 * @brief Stores a byte at the specified address.
 *
 * @param mem_sim The simulation database.
 * @param address The address to store at.
 * @param value The byte to store.
 */
void store(sim_database* mem_sim, int address, char value) {
    int PAGE_SIZE_LOG2 = 0;
    int temp = PAGE_SIZE;
    while (temp > 1) {
        temp >>= 1;
        PAGE_SIZE_LOG2++;
    }
    int page_number = address >> PAGE_SIZE_LOG2;  // Shift right by PAGE_SIZE_LOG2
    int offset = address & (PAGE_SIZE - 1);       // AND with PAGE_SIZE - 1

    if (page_number >= NUM_OF_PAGES || page_number < 0) {
        fprintf(stderr, "ERR\n");
        return;
    }

    if (mem_sim->page_table[page_number].P == 1) {
        fprintf(stderr, "ERR\n");
        return;
    }

    // Check if not the page is valid
    if (!mem_sim->page_table[page_number].V) {
        load_page(mem_sim, page_number);
    }


    // Get the physical address
    int physical_address = mem_sim->page_table[page_number].frame_swap * PAGE_SIZE + offset;

    // Set the value in the physical memory
    mem_sim->main_memory[physical_address] = value;

    // Mark the page as dirty
    mem_sim->page_table[page_number].D = 1;
}


/**
 * @brief Prints the current state of the physical memory.
 *
 * @param mem_sim The simulation database.
 */
void print_memory(sim_database* mem_sim) {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", mem_sim->main_memory[i]);
    }
}

/**
 * @brief Prints the current state of the swap file.
 *
 * @param mem_sim The simulation database.
 */
void print_swap(sim_database* mem_sim) {
    char str[PAGE_SIZE];
    int i;
    printf("\n Swap memory\n");
    lseek(mem_sim->swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(mem_sim->swapfile_fd, str, PAGE_SIZE) == PAGE_SIZE) {
        for(i = 0; i < PAGE_SIZE; i++) {
            printf("[%c]\t", str[i]);
        }
        printf("\n");
    }
}

/**
 * @brief Prints the current state of the page table.
 *
 * @param mem_sim The simulation database.
 */
void print_page_table(sim_database* mem_sim) {
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame_swap\n");
    for(i = 0; i < NUM_OF_PAGES; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n", mem_sim->page_table[i].V,
               mem_sim->page_table[i].D,
               mem_sim->page_table[i].P, mem_sim->page_table[i].frame_swap);
    }
}

/**
 * @brief Clears the system, freeing resources and closing files.
 *
 * @param mem_sim The simulation database.
 */
void clear_system(sim_database* mem_sim) {
    if (mem_sim) {
        close(mem_sim->program_fd);
        close(mem_sim->swapfile_fd);
        free(mem_sim);
    }
}
