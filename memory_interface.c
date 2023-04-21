/**
 * @file memory_interface.c
 * @authors Peter Wolfe and Liam Drew
 * @brief The implementation of the memory module in the UM. This module 
 *        supports access to and modification of the segmented memory of the UM.
 */
 
#include "memory_interface.h"

Except_T Exhausted_Segs = { "All memory segments exhausted." };

const int REGISTER_COUNT = 8;

/* struct storing pointers to memory data, for secret keeping*/
struct UM_Memory {
        Seq_T segment_sequence;
        Seq_T recycled_ids;
        uint32_t *registers;
};

static inline UArray_T load_initial_segment(FILE *fp);

/**
 * @name fetch_instruction
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

        /* Initializing all registers to be 0 */
        new_memory->registers = calloc(REGISTER_COUNT, sizeof(uint32_t));
        assert(new_memory->registers != NULL);

        return new_memory;
} 

/**
 * @name get_register
 * @brief Returns the value of a specified register
 *
 * @note Will CRE if register index is not between 0-7, inclusive
 * @returns The word in the specified register
*/
uint32_t get_register(UM_Memory mem, unsigned index)
{
        assert(mem);
        assert(index < 8);
        return mem->registers[index];
}

/**
 * @name set_register
 * @brief Sets the value of a register to the provided word
 *
 * @note Will CRE if register index is not between 0-7, inclusive
 * @returns The word in memory
*/
void set_register(UM_Memory mem, unsigned index, uint32_t new_contents)
{
        assert(mem);
        assert(index < 8);
        mem->registers[index] = new_contents;
}

/**
 * @name map_segment
 * @brief Initialize a new memory segment and return the segment #.
 *
 * @returns The index of the memory segment that was mapped
*/
uint32_t map_segment(UM_Memory mem, uint32_t size, bool *mem_exhausted)
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
                if (new_seg_id == hi) {
                        RAISE(Exhausted_Segs);
                        UArray_free(&new_seg);
                        *mem_exhausted = true;
                        return 0;
                }
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
        free(program_memory->registers);
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
        uint32_t r0 = get_register(mem, 0);
        uint32_t r1 = get_register(mem, 1);
        uint32_t r2 = get_register(mem, 2);
        uint32_t r3 = get_register(mem, 3);
        uint32_t r4 = get_register(mem, 4);
        uint32_t r5 = get_register(mem, 5);
        uint32_t r6 = get_register(mem, 6);
        uint32_t r7 = get_register(mem, 7);
        fprintf(stderr, 
                "REG: 0=%d, 1=%d, 2=%d, 3=%d, 4=%d, 5=%d, 6=%d, 7=%d\n", 
                r0, r1, r2, r3, r4, r5, r6, r7);
}