/**
 * @file io.c
 * @authors Peter Wolfe and Liam Drew
 * @brief The implementation for the Input/Output (I/O) module of the UM. This
 *        module supports the writing the contents of a register to disk, as
 *        well as reading in information from disk and storing it in a register.
 */

#include "io.h"

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