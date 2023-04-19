# Makefile for um (Comp 40 Assignment 3)
# Edited by: Peter Wolfe and Liam Drew
# Date: April 2023


############## Variables ###############

CC = gcc # The compiler being used

# Updating include path to use Comp 40 .h files and CII interfaces
IFLAGS = -I/comp/40/build/include -I/usr/sup/cii40/include/cii

# Compile flags
# Set debugging information, allow the c99 standard,
# max out warnings, and use the updated include path
# CFLAGS = -g -std=c99 -Wall -Wextra -Werror -Wfatal-errors -pedantic $(IFLAGS)
# 
# For this assignment, we have to change things a little.  We need
# to use the GNU 99 standard to get the right items in time.h for the
# the timing support to compile.
# 
CFLAGS = -g -std=gnu99 -Wall -Wextra -Werror -Wfatal-errors -pedantic -O2 \
	$(IFLAGS)

# Linking flags
# Set debugging information and update linking path
# to include course binaries and CII implementations

LDFLAGS = -g -L/comp/40/build/lib -L/usr/sup/cii40/lib64 -O2

# Libraries needed for linking
# All programs cii40 (Hanson binaries) and *may* need -lm (math)
# 40locality is a catch-all for this assignment, netpbm is needed for pnm
# rt is for the "real time" timing library, which contains the clock support

#NOTE: we modified this flag to include files from l40arith
#LDLIBS = -l40locality -lnetpbm -lcii40 -lm -lrt
LDLIBS = -lbitpack -lum-dis -lcii -lcii40 -lm -lrt



# Collect all .h files in your directory.
# This way, you can never forget to add
# a local .h file in your dependencies.
#
# This bugs Mark, who dislikes false dependencies, but
# he agrees with Noah that you'll probably spend hours 
# debugging if you forget to put .h files in your 
# dependency list.
INCLUDES = $(shell echo *.h)

############### Rules ###############

all: um

## Compile step (.c files -> .o files)

# To get *any* .o file, compile its .c file with the following rule.
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

## Linking step (.o -> executable program)

unit_test: 
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

um: main.o operation_manager.o memory_interface.o io.o instruction_handler.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f *.o