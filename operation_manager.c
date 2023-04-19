/**
 * @file operations_manager.
 * @authors Peter Wolfe and Liam Drew
 * @brief The implementation of the UM's operations module. Initializes the UM
 *        and starts the processor cycle, which starts processing the
 *        instructions passed into the UM when it is initialized.
 *        
 */
 
#include "operation_manager.h"

/* Program's current data is stored in a struct for secret-keeping */
typedef struct Program_Data {
        uint32_t program_counter;
} Program_Data;

static inline void processor_cycle(Program_Data *curr_data, UM_Memory
        curr_memory);

/**
 * @name  start_um
 * @brief Load the initial state of the UM. This involves:
 *        Initializing the program counter to 0.
 *        Accepting an argument for the program file.
 *        Load program from file into 0 segment of memory.
 *        Start processor cycle.
 *
 *
 * @note interactions: Interacts with the initialize_memory() function in the
         memory interface, since this interaction is necessary to intialize the
         UM.
 * @returns None
*/
void start_um(FILE *fp)
{
        /* Sets program counter equal to 0 */
        Program_Data *in_use_data;
        NEW(in_use_data);
        in_use_data->program_counter = 0;
        
        /* Initialize memory */
        UM_Memory in_use_memory = initialize_memory(fp);
        
        /* Start intstruction loop */
        processor_cycle(in_use_data, in_use_memory);

        /* Free memory when loop ends*/
        FREE(in_use_data);
        FREE(in_use_memory);
}


/**
 * @name  processor_cycle
 * @brief Starts the processor cycle of the UM. This invovles
 *        Fetching & Handling the program's next instruction.
 *        Updates program counter
 *        Repeats (continues processor cycle)
 *
 *
 * @note interactions: Interacts with the fetch_instruction() method in the
 *       Memory Interface and the handle_next_instruction() method of the
 *       Instruction Handler module. The fetch_instruction() method reads the
 *       next instruction from the 0 segment according to the program counter.
 *       The handle_next_instruction() method returns different exit codes to
 *       its caller (this function) depending on the state of the program. See
 *       the handle_next_instruction() function contract in
 *       instruction_handler.h for more information.
 * @returns None */
static inline void processor_cycle(Program_Data *curr_data,
        UM_Memory curr_memory)
{
        bool keep_running = true;
        while (keep_running) {
                /* Fetching next instruction from memory */
                UM_instruction next_instruction = fetch_instruction(curr_memory,
                        curr_data->program_counter);

                /* Handling fetched instruction */
                int exit_code = handle_instruction(curr_memory,
                        next_instruction);
                if (exit_code == -2) {
                        keep_running = false;
                        return;
                } else if (exit_code >= 0) { /* Set program counter to LV*/
                        curr_data->program_counter = (uint32_t) exit_code;
                } else { /* Incrementing Program Counter*/
                        curr_data->program_counter += 1;
                }
        }
}
