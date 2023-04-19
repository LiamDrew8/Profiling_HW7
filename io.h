/**
 * @file io.h
 * @authors Peter Wolfe and Liam Drew
 * @brief The interface for the Input/Output (I/O) module of the UM. This
 *        module supports the writing the contents of a register to disk, as
 *        well as reading in information from disk and storing it in a register.
 */

#include "um_dependencies.h"

#ifndef IO_H__
#define IO_H__

extern void output_register(uint32_t register_contents);

extern uint32_t read_in_to_register();

#endif
