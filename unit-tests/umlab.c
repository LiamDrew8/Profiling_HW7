/*
 * umlab.c
 *
 * Functions to generate UM unit tests. Once complete, this module
 * should be augmented and then linked against umlabwrite.c to produce
 * a unit test writing program.
 *  
 * A unit test is a stream of UM instructions, represented as a Hanson
 * Seq_T of 32-bit words adhering to the UM's instruction format.  
 * 
 * Any additional functions and unit tests written for the lab go
 * here. 
 *  
 */


#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <seq.h>
#include <bitpack.h>


typedef uint32_t Um_instruction;
typedef enum Um_opcode {
        CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
        NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;


/* Functions that return the two instruction types */

Um_instruction three_register(Um_opcode op, int ra, int rb, int rc);
Um_instruction loadval(unsigned ra, unsigned val);


/* Wrapper functions for each of the instructions */

static inline Um_instruction halt(void) 
{
        return three_register(HALT, 0, 0, 0);
}

typedef enum Um_register { r0 = 0, r1, r2, r3, r4, r5, r6, r7 } Um_register;

static inline Um_instruction add(Um_register a, Um_register b, Um_register c) 
{
        return three_register(ADD, a, b, c);
}

Um_instruction output(Um_register c);

/* Functions for working with streams */

static inline void append(Seq_T stream, Um_instruction inst)
{
        assert(sizeof(inst) <= sizeof(uintptr_t));
        Seq_addhi(stream, (void *)(uintptr_t)inst);
}

const uint32_t Um_word_width = 32;

void Um_write_sequence(FILE *output, Seq_T stream)
{
        assert(output != NULL && stream != NULL);
        int stream_length = Seq_length(stream);
        for (int i = 0; i < stream_length; i++) {
                Um_instruction inst = (uintptr_t)Seq_remlo(stream);
                for (int lsb = Um_word_width - 8; lsb >= 0; lsb -= 8) {
                        fputc(Bitpack_getu(inst, 8, lsb), output);
                }
        }
      
}


/* Implementing the three-register function*/
Um_instruction three_register(Um_opcode op, int ra, int rb, int rc)
{
    //take the opcode, turn it into binary, and put it first in the instruction
    //if it's anything but 13, put the other stuff, and then put the 3 registers
    //at the end.
    //uint32_t curr_opcode = op;
    printf("current opcode is %u\n", op);
    // Um_instruction new_instruction = curr_opcode;

        /* decode the opcode, a, b, c*/
    // unsigned int a = 0, b = 0, c = 0;
    Um_instruction value_to_load = 0;
    value_to_load = Bitpack_newu(value_to_load, 4, 28, op);

    value_to_load = Bitpack_newu(value_to_load, 3, 0, rc);
    value_to_load = Bitpack_newu(value_to_load, 3, 3, rb);
    value_to_load = Bitpack_newu(value_to_load, 3, 6, ra);

    return value_to_load;
}

Um_instruction loadval(unsigned ra, unsigned val)
{
    Um_instruction curr_instr = 0;
    curr_instr = Bitpack_newu(curr_instr, 4, 28, 13);
    curr_instr = Bitpack_newu(curr_instr, 3, 25, ra);
    curr_instr = Bitpack_newu(curr_instr, 25, 0, val);
    // Um_instruction load_instruction = 13;
    // load_instruction = load_instruction << 3;
    // load_instruction += ra;
    // load_instruction = load_instruction << 25;
    // load_instruction += val;
    return curr_instr;
}

Um_instruction output(Um_register c)
{
    return three_register(OUT, 0, 0, c);

}


static inline Um_instruction seg_load(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(SLOAD, a, b, c);
}
static inline Um_instruction seg_store(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(SSTORE, a, b, c);
}
static inline Um_instruction mult(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(MUL, a, b, c);
}
static inline Um_instruction division(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(DIV, a, b, c);
}
static inline Um_instruction bitNAND(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(NAND, a, b, c);
}
static inline Um_instruction map(Um_register a, Um_register b, 
    Um_register c)
{   
    (void)a;
    return three_register(ACTIVATE, 0, b, c);
}
static inline Um_instruction unmap(Um_register a, Um_register b, Um_register c)
{
    (void)a;
    (void)b;
    return three_register(INACTIVATE, 0, 0, c);
}
static inline Um_instruction input(Um_register a, Um_register b, Um_register c)
{
    (void)a;
    (void)b;
    return three_register(IN, 0, 0, c);
}
static inline Um_instruction load_program(Um_register a, Um_register b, 
    Um_register c)
{
    (void)a;
    return three_register(LOADP, 0, b, c);
}
/*
typedef enum Um_opcode {
        CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
        NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;*/

static inline Um_instruction conditional_move(Um_register a, Um_register b, 
    Um_register c)
{
    return three_register(CMOV, a, b, c);
}

/* Unit tests for the UM */

/* Should segfault on purpose */
void no_halt_test(Seq_T stream)
{
        append(stream, loadval(r1, 'B'));
}

void build_halt_test(Seq_T stream)
{
        append(stream, halt());
}

void build_verbose_halt_test(Seq_T stream)
{
        append(stream, loadval(r1, 'B'));
        append(stream, output(r1));
        append(stream, loadval(r1, 'a'));
        append(stream, output(r1));
        append(stream, loadval(r1, 'd'));
        append(stream, output(r1));
        append(stream, loadval(r1, '!'));
        append(stream, output(r1));
        append(stream, loadval(r1, '\n'));
        append(stream, output(r1));
        append(stream, halt());
}


void build_message_test(Seq_T stream)
{
    append(stream, loadval(r1, 'H'));
    append(stream, loadval(r2, 'e'));
    append(stream, loadval(r3, 'l'));
    append(stream, loadval(r4, 'o'));
    append(stream, loadval(r5, '\n'));
    append(stream, output(r1));
    append(stream, output(r2));
    append(stream, output(r3));
    append(stream, output(r4));
    append(stream, output(r5));
    append(stream, halt());
}

void add_test(Seq_T stream)
{
    append(stream, add(r1, r2, r3));
    append(stream, halt());
}

void print_digit(Seq_T stream)
{
    append(stream, loadval(r1, 48));
    append(stream, loadval(r2, 6));
    append(stream, add(r3, r1, r2));
    append(stream, output(r3));
    append(stream, halt());
}

/* Expecting r1 to be Y*/
void conditional_move_true(Seq_T stream)
{
    append(stream, loadval(r1, 'k'));
    append(stream, loadval(r2, 'Y'));
    append(stream, loadval(r3, 1));
    append(stream, output(r3));
    append(stream, conditional_move(r1, r2, r3));
    append(stream, output(r1));
    append(stream, halt());
}

/* Expecting r1 to be N*/
void conditional_move_false(Seq_T stream)
{
    append(stream, loadval(r1, 'N'));
    append(stream, loadval(r2, 'b'));
    append(stream, loadval(r3, 0));
    append(stream, output(r3));
    append(stream, conditional_move(r1, r2, r3));
    append(stream, output(r1));
    append(stream, halt());
}

void add_big_nums(Seq_T stream)
{
    /* Should be 0*/
    uint32_t x = 894234;
    append(stream, loadval(r1, x));
    append(stream, loadval(r2, 1));
    append(stream, add(r3, r1, r2));
    // append(stream, output(r3));
    append(stream, halt());

}

void mult_big_nums(Seq_T stream)
{
    /* Should be 2^32 - 1*/

    append(stream, loadval(r1, 234));
    append(stream, loadval(r2, 22));
    append(stream, mult(r3, r1, r2));
    // append(stream, output(r3));
    append(stream, halt());

}

/*
division
division by 0

subtractions

I/O with numbers > 255
Load new program
*/
void io_G_255(Seq_T stream)
{
    append(stream, loadval(r1, 256));
    append(stream, output(r1));
    append(stream, halt());
}

void div_by_0(Seq_T stream)
{
    append(stream, loadval(r1, 4));
    append(stream, loadval(r2, 0));
    append(stream, division(r3, r1, r2));
    append(stream, output(r3));
    append(stream, halt());
}

void access_unmapped(Seq_T stream)
{
    append(stream, loadval(r1, 4));
    append(stream, loadval(r2, 4));
    append(stream, load_program(r3, r1, r2));
    append(stream, halt());
}


/* map segment */
/* unmap segment */