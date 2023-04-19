/**
 * @file main.c
 * @authors Peter Wolfe and Liam Drew
 * @brief The driver file for the UM program. Verifies correct UM usage and
 *        initializes the UM. 
 */

#include "memory_interface.h"
#include "operation_manager.h"

int main(int argc, char *argv[])
{
        if (argc != 2) {
            fprintf(stderr, "Usage: ./um [executable.um]\n");
            return EXIT_FAILURE;
        }

        FILE *fp = fopen(argv[1], "r");
        
        if (fp == NULL) {
            fprintf(stderr, "File could not be opened.\n");
            return EXIT_FAILURE;
        }

        start_um(fp);
        return EXIT_SUCCESS;
}