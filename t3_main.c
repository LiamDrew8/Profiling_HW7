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

typedef uint32_t UM_instruction;


/**  OPERATIONS MANAGER */
void start_um(FILE *fp);

/** MEMORY INTERFACE */

// static inline uint32_t get_register(unsigned index);

// static inline void set_register(unsigned index, uint32_t new_contents);

uint32_t map_segment(uint32_t size);

void unmap_segment(uint32_t segment);

void load_program(uint32_t segment);

void handle_halt();

void print_registers();


/** INTRUCTION HANDLER */

/** IO MODULE */
void output_register(uint32_t register_contents);

uint32_t read_in_to_register();



/* ---------*/


uint32_t program_counter = 0;

uint32_t registers[8] = {0};

/* Array of Segments (Will be reallocated) */
uint32_t **segment_sequence;
uint32_t seq_size = 0;
uint32_t seq_capacity = 0;

/* Corresponding Length of each segment */
uint32_t *segment_lengths;


/* Array of Recycled IDs */
uint32_t *recycled_ids;
uint32_t rec_size = 0;
uint32_t rec_capacity = 0;

uint64_t power = (uint64_t)1 << 32;



int main(int argc, char *argv[])
{

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

void initialize_memory(FILE *fp);
void processor_cycle();
void start_um(FILE *fp)
{
        /* Initialize memory */
        initialize_memory(fp);
        
        /* Start intstruction loop */
        processor_cycle();
}


void load_initial_segment(FILE *fp);
void initialize_memory(FILE *fp)
{
        seq_capacity = 128;
        /* This could be a problem area. Pay attention here */
        segment_sequence = malloc(seq_capacity * sizeof(uint32_t*));
        assert(segment_sequence != NULL);

        segment_lengths = malloc(seq_capacity * sizeof(uint32_t));
        assert(segment_lengths != NULL);

        load_initial_segment(fp);

        rec_capacity = 128;
        recycled_ids = (uint32_t*) malloc(rec_capacity * sizeof(uint32_t*));
        assert(recycled_ids != NULL);


}

void load_initial_segment(FILE *fp)
{
        /* Get file size here */
        fseek(fp, 0L, SEEK_END);
        int file_size = ftell(fp);
        rewind(fp);

        /* Creates the new array for segment 0 */
        uint32_t *temp = (uint32_t*) malloc(file_size * sizeof(uint32_t));
        assert(temp != NULL);

        //UArray_T initial_program = UArray_new(file_size, sizeof(uint32_t));
        /* declare first segment*/
        
        //uint32_t *curr_ptr;
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
                        temp[i/4] = word;
                        word = 0;
                }
                i++;
        }
        fclose(fp);
        /* Putting the new segment 0 into the segment array */
        segment_sequence[0] = temp;
        segment_lengths[0] = file_size;
        seq_size++;
        return;
}

void handle_instruction(UM_instruction word);
void processor_cycle()
{
        while (true) {
                handle_instruction(segment_sequence[0][program_counter]);
        }
}

/** INSTRUCTION HANDLER
 */

uint32_t segmented_load(uint32_t segment, uint32_t index);
void segmented_store(uint32_t segment, uint32_t index, uint32_t value);

void handle_instruction(UM_instruction word)
{
        program_counter++;

        /* decode the opcode, a, b, c*/
        uint32_t  a, b, c;
        uint32_t value_to_load = 0;
        uint32_t opcode = word;
        opcode = opcode >> 28;
        //opcode = Bitpack_getu(word, 4, 28);

        if (opcode == 13) {
        // Load value into $r[A]
                a = (word >> 25) & 0x7;
                value_to_load = word & 0x1FFFFFF;
        } else {
                c = word & 0x7;
                b = (word >> 3) & 0x7;
                a = (word >> 6) & 0x7;
        }
        switch (opcode) {
                case 0:
                        /* Conditional Move */
                        if (registers[c] == 0) { 
                                break; 
                        } else {
                                registers[a] = registers[b];
                        }

                        break;
                case 1:
                        /* Segmented Load (Memory Module) */
                        registers[a] = segmented_load(registers[b], 
                                registers[c]);
                        break;
                case 2:
                        /* Segmented Store (Memory Module) */
                        segmented_store(registers[a], 
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
                        handle_halt();
                        /* This exit code of -2 tells the operations module to
                         * stop the processor cycle */
                        break;
                case 8:
                        /* Map Segment (Memory Module) */
                        registers[b] = map_segment(registers[c]);
                        break;
                case 9:
                        /* Unmap Segment (Memory Module) */
                        unmap_segment(registers[c]);
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
                        //fprintf(stderr, "LOADING  %u\n", registers[b]);
                        load_program(registers[b]);
                        /* This exit code returns the new value of the program
                         * counter */
                        program_counter = registers[c];
                        break;
                case 13:
                        /* Load Value */
                        registers[a] = value_to_load;
                        //fprintf(stderr, "Reg val is %u\n", registers[a]);
                        break;
        }

        /* This exit code of -1 means everything is normal */
}


/** MEMORY INTERFACE */

/**
 * @name map_segment
 * @brief Initialize a new memory segment and return the segment #.
 *
 * @returns The index of the memory segment that was mapped
*/
uint32_t map_segment(uint32_t size)
{

        uint32_t *temp = calloc(size, sizeof(uint32_t));
        uint32_t new_seg_id;

        /* If there are no available recycled segment ids, make a new one */
        if (rec_size == 0) {
                if (seq_size == seq_capacity) {
                    //fprintf(stderr, "Expanding sequence\n");
                    seq_capacity = seq_capacity * 2 + 2;
                    /* If this ever errors, check the multiplication*/
                    segment_sequence = realloc(segment_sequence, (seq_capacity) * sizeof(uint32_t*));
                    segment_lengths = realloc(segment_lengths, (seq_capacity) * sizeof(uint32_t));
                }

                // fprintf(stderr, "SEQ size is %u\n", seq_size);
                segment_sequence[seq_size] = temp;
                new_seg_id = seq_size;
                seq_size++;
        } 

        /* Otherwise, reuse an old one */
        else {
                // fprintf(stderr, "Recycling sequence\n");
                // fprintf(stderr, "Rec size %u\n", rec_size);
                rec_size--;
                new_seg_id = recycled_ids[rec_size];
                // fprintf(stderr, "New id %u\n", new_seg_id);
                recycled_ids[rec_size] = 0;
                segment_sequence[new_seg_id] = temp;
                // new_seg_id = (uint32_t)(uintptr_t)Seq_remlo(mem->recycled_ids);
                // Seq_put(mem->segment_sequence, new_seg_id, new_seg);
        }
        segment_lengths[new_seg_id] = size;

        return new_seg_id;
}

/**
 * @name unmap_segment
 * @brief Frees the memory segment associated with the address provided
 *
 * @returns None
*/
void unmap_segment(uint32_t segment)
{
        uint32_t *to_be_freed = segment_sequence[segment];
        free(to_be_freed);
        segment_sequence[segment] = NULL;

        if (rec_size == rec_capacity) {
            rec_capacity = rec_capacity * 2 + 2;
            //realloc recycled ids
            recycled_ids = realloc(recycled_ids, (rec_capacity) * sizeof(uint32_t));
        }
        recycled_ids[rec_size] = segment;
        segment_lengths[segment] = 0;
        rec_size++;

        // /* Adding the segment identifier of the newly unmapped segment to the 
        //  * recycling sequence */
        // Seq_addhi(mem->recycled_ids, (void *)(uintptr_t)segment);
        // UArray_T to_be_freed = (UArray_T) Seq_get(mem->segment_sequence, 
        //     segment);
        // /* Setting the pointer to the unmapped sequence to NULL to avoid 
        //  * double frees */
        // Seq_put(mem->segment_sequence, segment, NULL);
        // UArray_free(&to_be_freed);
}

/**
 * @name segmented_load
 * @brief Get the word stored at the index of a memory segment
 *
 * @returns The word in memory
*/
uint32_t segmented_load(uint32_t segment, uint32_t index)
{
        return segment_sequence[segment][index];
        // assert(mem);
        // UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
        //         segment);
        // assert(get_segment);
        // return *(uint32_t*)UArray_at(get_segment, index);
}

/**
 * @name segmented_store
 * @brief Put a word into a position of memory
 *
 * @returns None
*/
void segmented_store(uint32_t segment, uint32_t index, 
        uint32_t value)
{
        segment_sequence[segment][index] = value;
        // assert(mem);
        // UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
        //         segment);
        // assert(get_segment);
        // *(uint32_t*)UArray_at(get_segment, index) = value;

}

/**
 * @name load_program
 * @brief Copies the memory segment pointed to by a specified register into
 *        segment 0 (the active program segment). If the provided segment is
 *        0, do nothing.
 *
 * @returns None
*/
void load_program(uint32_t segment)
{

        if (segment == 0) { return; }

        /* get address of segment at provided index*/
        uint32_t copied_seq_size = segment_lengths[segment];
        //fprintf(stderr, "Copying segment of size %u \n", copied_seq_size);

        /* free the first segment */
        uint32_t *to_be_freed = segment_sequence[0];
        free(to_be_freed);

        /* malloc a new segment in index 0 with size of one to be copied */
        segment_sequence[0] = (uint32_t*) malloc(copied_seq_size * sizeof(uint32_t));
        /* Cache performance? */
        for (uint32_t i = 0; i < copied_seq_size; i++) {
            segment_sequence[0][i] = segment_sequence[segment][i];
        }

        segment_lengths[0] = copied_seq_size;

        /* loop through one to be copied and copy everything to seg 0*/


/*         UArray_T get_segment = (UArray_T) Seq_get(mem->segment_sequence, 
                segment);
        UArray_T first_segment = UArray_copy(get_segment, 
                UArray_length(get_segment));
        Seq_put(mem->segment_sequence, 0, first_segment); */
        //(void)first_segment;
}

/**
 * @name handle_halt
 * @brief 
 *
 * @returns None
*/

/* Needs to free memory */
void handle_halt()
{
        // //free(program_memory->registers);
        // int seq_len = Seq_length(program_memory->segment_sequence);

        // for (int i = 0; i < seq_len; i++) {
        //         UArray_T to_be_freed = (UArray_T) 
        //                 Seq_get(program_memory->segment_sequence, i);
        //         /* Ensuring that no NULL pointers get freed */
        //         if (to_be_freed != NULL) { UArray_free(&to_be_freed); } 
        // }

        // Seq_free(&(program_memory->segment_sequence));
        // Seq_free(&(program_memory->recycled_ids));

        /* NOTHING TO FREE FOR REGISTERS*/

        for (uint32_t i = 0; i < seq_size; i++) {
                if (segment_sequence[i] != NULL) {
                        free(segment_sequence[i]);
                }
        }

        free(segment_sequence);
        free(segment_lengths);
        free(recycled_ids);
        
        exit(EXIT_SUCCESS);
}

/**
 * @name print_registers
 * @brief Debug helper function. Prints all registers.
 *
 * @returns None
*/
/* void print_registers(UM_Memory mem)
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
} */


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

