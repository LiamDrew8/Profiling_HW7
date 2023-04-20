#!/bin/bash

# check if an argument was provided
if [ $# -eq 0 ]; then
  echo "Usage: $0 [benchmark]"
  exit 1
fi

# run the benchmark with Callgrind and dump instructions
valgrind --tool=callgrind --dump-instr=yes ./um umbin/"$1" > /dev/null

# wait for the program to finish
wait

# find the first Callgrind output file
callgrind_files=$(ls -t callgrind.out.*)
latest=$(echo "$callgrind_files" | head -n 1)

# extract the process ID from the filename
pid=$(echo "$latest" | sed -n 's/^callgrind.out.\([0-9]*\)$/\1/p')

# launch QCachegrind with the latest output file
qcachegrind "callgrind.out.$pid"
