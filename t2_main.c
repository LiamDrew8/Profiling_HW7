#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "assert.h"
#include "uarray.h"
#include <stdbool.h>
#include "bitpack.h"
#include "math.h"
#include "mem.h"
#include "seq.h"
#include "io.h"

typedef struct Program_Data {
        uint32_t program_counter;
} Program_Data;

/* This will not fail. At least it better not*/
Except_T Exhausted_Segs = { "All memory segments exhausted." };

/* struct storing pointers to memory data, for secret keeping*/
struct UM_Memory {
        Seq_T segment_sequence;
        Seq_T recycled_ids;
        uint32_t *registers;
};

uint32_t registers[8] = {0};


/**  OPERATIONS MANAGER */
void start_um(FILE *fp);

/** MEMORY INTERFACE */
typedef struct UM_Memory *UM_Memory;

typedef uint32_t UM_instruction;

UM_instruction fetch_instruction(UM_Memory, int program_counter);

UM_Memory initialize_memory(FILE *fp);

static inline uint32_t get_register(unsigned index);

static inline void set_register(unsigned index, uint32_t new_contents);

uint32_t segmented_load(UM_Memory mem, uint32_t segment, uint32_t index);

void segmented_store(UM_Memory mem, uint32_t segment, uint32_t index,
        uint32_t value);

uint32_t map_segment(UM_Memory mem, uint32_t size);

void unmap_segment(UM_Memory mem, uint32_t segment);

void load_program(UM_Memory mem, uint32_t segment);

void handle_halt(UM_Memory program_memory);

void print_registers(UM_Memory mem);

static UArray_T load_initial_segment(FILE *fp);

static void processor_cycle(Program_Data *curr_data, UM_Memory
        curr_memory);


/** INTRUCTION HANDLER */
extern int64_t handle_instruction(UM_Memory mem, UM_instruction word);

/** IO MODULE */
void output_register(uint32_t register_contents);

uint32_t read_in_to_register();



/* ---------*/


uint32_t program_counter = 0;



int main(int argc, char *argv[])
{
        //register uint32_t program_counter = 0;

        /* Open and Handle files */
        if (argc != 2) {
            fprintf(stderr, "Usage: ./um [executable.um]\n");
            return EXIT_FAILURE;
        }

        FILE *fp = fopen(argv[1], "r");
        
        if (fp == NULL) {
            fprintf(stderr, "File could not be opened.\n");
            return EXIT_FAILURE;
        }

        /* Starting UM: */

        start_um(fp);

        
        return EXIT_SUCCESS;
}




/** MEMORY INTERFACE */



/**
 * @name fetch_instruction NOW LIVES INLINE
 * @brief Gets an instruction word from the zero-segment at the index of the
 *        program counter.
 *
 * @returns Returns the instruction word
*/
UM_instruction fetch_instruction(UM_Memory curr_memory, int program_counter)
{
        /* Get segment 0 */
        UArray_T segment_zero = (UArray_T) 
                Seq_get(curr_memory->segment_sequence, 0);
        /* get instruction*/
        UM_instruction *instr = (uint32_t *) 
                UArray_at(segment_zero, program_counter);
        return *instr;
}

/**
 * @name load_initial_segment
 * @brief Helper function for initialize_memory. Reads the file and initializes
 *        a segment with the provided words.
 *
 * @returns The new segment, as a UArray_T
*/
UArray_T load_initial_segment(FILE *fp)
{
        assert(fp != NULL);

        /* Get file size here */
        fseek(fp, 0L, SEEK_END);
        int file_size = ftell(fp);
        rewind(fp);

        UArray_T initial_program = UArray_new(file_size, sizeof(uint32_t));
        uint32_t *curr_ptr;
        uint32_t word = 0;
        int c;
        int i = 0;
        unsigned char c_char;
        
        for (c = getc(fp); c != EOF; c = getc(fp)) {
                c_char = (unsigned char)c;
                
                /* Each word is made up of 4 chars. Here we build the word one
                 * char at a time in Big Endian Order */

                if (i % 4 == 0) {
                        word = Bitpack_newu(word, 8, 24, c_char);
                } else if (i % 4 == 1) {
                        word = Bitpack_newu(word, 8, 16, c_char);
                } else if (i % 4 == 2) {
                        word = Bitpack_newu(word, 8, 8, c_char);
                } else if (i % 4 == 3) {
                        word = Bitpack_newu(word, 8, 0, c_char);
                        curr_ptr = (uint32_t*) UArray_at(initial_program, 
                                (i / 4));
                        *curr_ptr = word;
                        word = 0;
                }
                i++;
        }
        fclose(fp);
        return initial_program;
}

/**
 * @name initialize_memory
 * @brief Initializes the data that the provided UM_Memory struct points to.
 *        Reads the file pointer into the zero-segment.
 *
 * @note  Assumes the provided UM_Memory is uninitialized. Also, the memory 
 *        allocated in this function will be freed in the operation_manager
 *        module
 * @returns Returns the UM_Memory struct
*/
UM_Memory initialize_memory(FILE *fp)
{
        UArray_T instruction_seq = load_initial_segment(fp);
        UM_Memory new_memory;
        NEW(new_memory);
        new_memory->segment_sequence = Seq_new(1);
        new_memory->recycled_ids = Seq_new(1);
        Seq_addlo(new_memory->segment_sequence, instruction_seq);

        return new_memory;
} 

/**
 * @name get_register
 * @brief Returns the value of a specified register
 *
 * @note Will CRE if register index is not between 0-7, inclusive
 * @returns The word in the specified register
*/
static inline uint32_t get_register(unsigned index)
{
        //assert(mem);
        assert(index < 8);
        return registers[index];
}

/**
 * @name set_register
 * @brief Sets the value of a register to the provided word
 *
 * @note Will CRE if register index is not between 0-7, inclusive
 * @returns The word in memory
*/
static inline void set_register(unsigned index, uint32_t new_contents)
{

        // assert(mem);
        assert(index < 8);
        registers[index] = new_contents;
}

/**
 * @name map_segment
 * @brief Initialize a new memory segment and return the segment #.
 *
 * @returns The index of the memory segment that was mapped
*/
uint32_t map_segment(UM_Memory mem, uint32_t size)
{
        assert(mem);
        UArray_T new_seg = UArray_new(size, sizeof(uint32_t));

        /* Sets every word in the segment equal to 0 */
        for (uint32_t i = 0; i < size; i++) {
                *(uint32_t*)UArray_at(new_seg, i) = 0;
        }

        uint32_t new_seg_id;
        uint32_t hi = 0;
        hi -= 1;
        int avail_ids = Seq_length(mem->recycled_ids);

        /* If there are no available recycled segment ids, make a new one */
        if (avail_ids == 0) {
                /* Check and make sure that the UM hasn't run out of segments */
                new_seg_id = Seq_length(mem->segment_sequence);
                Seq_addhi(mem->segment_sequence, new_seg);
        } 

        /* Otherwise, reuse an old one */
        else {
                new_seg_id = (uint32_t)(uintptr_t)Seq_remlo(mem->recycled_ids);
                Seq_put(mem->segment_sequence, new_seg_id, new_seg);
        }

        return new_seg_id;
}

/**
 * @name unmap_segment
 * @brief Frees the memory segment associated with the address provided
 *
 * @returns None
*/
void unmap_segment(UM_Memory mem, uint32_t segment)
{
        assert(mem);
        /* Adding the segment identifier of the newly unmapped segment to the 
         * recycling sequence */
        Seq_addhi(mem->recycled_ids, (void *)(uintptr_t)segment);
        UArray_T to_be_freed = (UArray_T) Seq_get(mem->segment_sequence, 
            segment);
        /* Setting the pointer to the unmapped sequence to NULL to avoid 
         * double frees */
        Seq_put(mem->segment_sequence, segment, NULL);
        UArray_free(&to_be_freed);
}

/**
 * @name segmented_load
 * @brief Get the word stored at the index of a memory segment
 *
 * @returns The word in memory
*/
uint32_t segmented_load(UM_Memory mem, uint32_t segment, uint32_t index)
{
        assert(mem);
        UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
                segment);
        assert(get_segment);
        return *(uint32_t*)UArray_at(get_segment, index);
}

/**
 * @name segmented_store
 * @brief Put a word into a position of memory
 *
 * @returns None
*/
void segmented_store(UM_Memory mem, uint32_t segment, uint32_t index, 
        uint32_t value)
{
        assert(mem);
        UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
                segment);
        assert(get_segment);
        *(uint32_t*)UArray_at(get_segment, index) = value;

}

/**
 * @name load_program
 * @brief Copies the memory segment pointed to by a specified register into
 *        segment 0 (the active program segment). If the provided segment is
 *        0, do nothing.
 *
 * @returns None
*/
void load_program(UM_Memory mem, uint32_t segment)
{
        assert(mem);
        assert(mem->segment_sequence);

        if (segment == 0) { return; }

        UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
                segment);
        UArray_T first_segment = UArray_copy(get_segment, 
                UArray_length(get_segment));
        Seq_put(mem->segment_sequence, 0, first_segment);
        //(void)first_segment;
}

/**
 * @name handle_halt
 * @brief Handles the halt instruction and frees all program memory
 *
 * @returns None
*/
void handle_halt(UM_Memory program_memory)
{
        //free(program_memory->registers);
        int seq_len = Seq_length(program_memory->segment_sequence);

        for (int i = 0; i < seq_len; i++) {
                UArray_T to_be_freed = (UArray_T) 
                        Seq_get(program_memory->segment_sequence, i);
                /* Ensuring that no NULL pointers get freed */
                if (to_be_freed != NULL) { UArray_free(&to_be_freed); } 
        }

        Seq_free(&(program_memory->segment_sequence));
        Seq_free(&(program_memory->recycled_ids));
}

/**
 * @name print_registers
 * @brief Debug helper function. Prints all registers.
 *
 * @returns None
*/
void print_registers(UM_Memory mem)
{
        (void)mem;
        uint32_t r0 = get_register(0);
        uint32_t r1 = get_register(1);
        uint32_t r2 = get_register(2);
        uint32_t r3 = get_register(3);
        uint32_t r4 = get_register(4);
        uint32_t r5 = get_register(5);
        uint32_t r6 = get_register(6);
        uint32_t r7 = get_register(7);
        fprintf(stderr, 
                "REG: 0=%d, 1=%d, 2=%d, 3=%d, 4=%d, 5=%d, 6=%d, 7=%d\n", 
                r0, r1, r2, r3, r4, r5, r6, r7);
}

/** INSTRUCTION HANDLER
 */
int64_t handle_instruction(UM_Memory mem, UM_instruction word)
{
        /* decode the opcode, a, b, c*/
        uint32_t  a = 0, b = 0, c = 0;
        uint32_t value_to_load = 0;
        uint32_t opcode = word;
        opcode = opcode >> 28;
        //opcode = Bitpack_getu(word, 4, 28);

        

        if (opcode == 13) {
                /* Load value into $r[A] */
                a = word;
                a = a << 4;
                a = a >> 29;
                //a = Bitpack_getu(word, 3, 25);
                value_to_load = word;
                value_to_load = value_to_load << 7;
                value_to_load = value_to_load >> 7;
                //value_to_load = Bitpack_getu(word, 25, 0);
        } else {
                c = word;
                c = c << 29;
                c = c >> 29;
                //c = Bitpack_getu(word, 3, 0);
                
                b = word;
                b = b << 26;
                b = b >> 29;
                //b = Bitpack_getu(word, 3, 3);

                a = word;
                a = a << 23;
                a = a >> 29;
                //a = Bitpack_getu(word, 3, 6);
        }

        /* power stores 2^32 for use with arithmetic operations */
        uint64_t power = (uint64_t)1 << 32;

        /* temp is used in some arithmetic operations to prevent integer
         * overflow when adding and multiplying uint32_ts */

        // uint64_t temp;
        /* In this switch table, we handle every single UM instruction */
        switch (opcode) {
                case 0:
                        /* Conditional Move */
                        if (get_register(c) == 0) { 
                                break; 
                        } else {
                                set_register(a, get_register(b));
                        }

                        break;
                case 1:
                        /* Segmented Load (Memory Module) */
                        registers[a] = segmented_load(mem, 
                                registers[b], registers[c]);
                        break;
                case 2:
                        /* Segmented Store (Memory Module) */
                        segmented_store(mem, registers[a], 
                                registers[b], registers[c]);
                        break;
                case 3:
                        /* Addition */
                        registers[a] = (registers[b] + registers[c]) % power;
                        break;
                case 4:
                        /* Multiplication */
                        registers[a] = (registers[b] * registers[c]) % power;
                        break;
                case 5:
                        /* Division */
                        registers[a] = registers[b] / registers[c];
                        break;
                case 6:
                        registers[a] = ~(registers[b] & registers[c]);
                        break;
                case 7:
                        /* Halt */
                        handle_halt(mem);
                        /* This exit code of -2 tells the operations module to
                         * stop the processor cycle */
                        return -2;
                        break;
                case 8:
                        /* Map Segment (Memory Module) */
                        registers[b] = map_segment(mem, registers[c]);
                        break;
                case 9:
                        /* Unmap Segment (Memory Module) */
                        unmap_segment(mem, registers[c]);
                        break;
                case 10:
                        /* Output (IO) */
                        output_register(registers[c]);
                        break;
                case 11:
                        /* Input (IO) */
                        registers[c] = read_in_to_register();
                        break;
                case 12:
                        /* Load Program (Memory Module) */
                        load_program(mem, registers[b]);
                        /* This exit code returns the new value of the program
                         * counter */
                        return registers[c];
                        break;
                case 13:
                        /* Load Value */
                        registers[a] = value_to_load;
                        break;
        }

        /* This exit code of -1 means everything is normal */
        return -1;
}


/** IO MODULE
 */

/**
 * @name output_register
 * @brief Takes a word as an ASCII char and prints it to standard I/O.
 *
 * @note The word must be between values 0 to 255 inclusive. If it is not,
 *       the UM will produce a CRE.
 * @returns None
*/
void output_register(uint32_t register_contents)
{
        /* Assert it's a char*/
        assert(register_contents <= 255);
        /* Print the char*/
        unsigned char c = (unsigned char) register_contents;
        printf("%c", c);
}

/**
 * @name read_in_to_register
 * @brief Reads a single char from standard I/O and returns it as a word.
 *
 * @note If the input is an EOF character, the returned word will be -1.
 * @returns None
*/
uint32_t read_in_to_register()
{
        int c;
        uint32_t contents = 0;
        c = getc(stdin);
        /* Check it's not an EOF character*/
        if (c == EOF) {
                contents -= 1;
                // fprintf(stderr, "contents is %u \n", contents);
                return contents;
        } else {
                assert(c <= 255);
                contents = (uint32_t) c;
                return contents;
        }
}

void start_um(FILE *fp)
{
        /* Sets program counter equal to 0 */
        Program_Data *in_use_data;
        NEW(in_use_data);
        program_counter = 0;
        
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
        (void)curr_data;
        bool keep_running = true;
        while (keep_running) {
                /* Fetching next instruction from memory */
                        /* Get segment 0 */
                UArray_T segment_zero = (UArray_T) 
                        Seq_get(curr_memory->segment_sequence, 0);
                /* get instruction*/
                UM_instruction *instr = (uint32_t *) 
                        UArray_at(segment_zero, program_counter);
                UM_instruction next_instruction = *instr;

                /* Handling fetched instruction */
                int exit_code = handle_instruction(curr_memory,
                        next_instruction);
                if (exit_code == -2) {
                        keep_running = false;
                        return;
                } else if (exit_code >= 0) { /* Set program counter to LV*/
                        program_counter = (uint32_t) exit_code;
                } else { /* Incrementing Program Counter*/
                        program_counter += 1;
                }
        }
}
