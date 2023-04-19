/**
 * @file memory_interface.h
 * @authors Peter Wolfe and Liam Drew
 * @brief The interface of the memory module in the UM. This module supports
 *        access to and modification of the segmented memory of the UM.
 */
 
#ifndef MEMORY_INTERFACE_H__
#define MEMORY_INTERFACE_H__

#include "um_dependencies.h"


typedef struct UM_Memory *UM_Memory;

typedef uint32_t UM_instruction;

extern UM_instruction fetch_instruction(UM_Memory, int program_counter);

extern UM_Memory initialize_memory(FILE *fp);

extern uint32_t get_register(UM_Memory mem, unsigned index);

extern void set_register(UM_Memory mem, unsigned index, uint32_t new_contents);

extern uint32_t segmented_load(UM_Memory mem, uint32_t segment, uint32_t index);

extern void segmented_store(UM_Memory mem, uint32_t segment, uint32_t index,
        uint32_t value);

extern uint32_t map_segment(UM_Memory mem, uint32_t size, bool *mem_exhausted);

extern void unmap_segment(UM_Memory mem, uint32_t segment);

extern void load_program(UM_Memory mem, uint32_t segment);

extern void handle_halt(UM_Memory program_memory);

extern void print_registers(UM_Memory mem);


#endif
