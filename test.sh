#!/bin/bash

# Check if the correct number of arguments are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 <input file> <generations>"
    exit 1
fi

input_file=$1
generations=$2

# Print the input file name for debugging
#echo "Input file: $input_file"

# Count the number of lines in the input file
num_lines=$(grep -c "" "$input_file")

# Print the number of lines for debugging
#echo "Number of lines in input file: $num_lines"

# Get the number of available processors
available_processors=$(grep -c processor /proc/cpuinfo)

# Print the number of available processors for debugging
#echo "Available processors: $available_processors"

# Function to find the lowest full number result of divising number of lines by divisor, 
# that is incrementing from 2. The reuslt must be smaller than number of available threads.
find_lowest_divisor() {
    num_lines=$1
    value=$2
    divisor=2
    while [ $divisor -le $num_lines ]; do
        result=$(($num_lines / $divisor))
        if [ $(($num_lines % $divisor)) -eq 0 ] && [ $result -le $value ]; then
            echo "$result"
            return
        fi
        ((divisor++))
    done
}

# Find the lowest divisor larger than 1 for the desired processors
num_processors=$(find_lowest_divisor $num_lines $available_processors)

# Print the number of processors to use for debugging
#echo "Number of processors to use: $num_processors"

# Compile the C++ code
mpic++ --prefix /usr/local/share/OpenMPI -o life life.cpp

# Run the program using threads instead of processors
mpirun --prefix /usr/local/share/OpenMPI --use-hwthread-cpus -np "$num_processors" life "$input_file" "$generations"

# Clean up
rm -f life