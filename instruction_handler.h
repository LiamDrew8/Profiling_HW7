/**
 * @file instruction_handler.h
 * @authors Peter Wolfe and Liam Drew
 * @brief The interface of the instruction handler module. This module 
 *        handles the 14 different instructions of the UM.
 */

#include "memory_interface.h"
#include "io.h"

#ifndef INSTRUCTION_HANDLER_H__
#define INSTRUCTION_HANDLER_H__

extern int64_t handle_instruction(UM_Memory mem, UM_instruction word);

#endif