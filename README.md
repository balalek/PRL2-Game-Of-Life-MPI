# Conway's Game of Life with MPI

**Author:** Martin Baláž  
**Course:** Parallel and Distributed Algorithms (PRL)  
**Language:** C++

## Project Description

Parallel implementation of Conway's Game of Life using OpenMPI library. This cellular automaton simulates the evolution of a 2D grid of cells over multiple generations based on simple rules about cell survival, death, and birth. The implementation divides the game board into horizontal slices distributed across multiple MPI processes for parallel computation.

## Game Rules

Conway's Game of Life follows these rules:
1. **Survival**: A living cell with 2 or 3 living neighbors survives
2. **Death by Isolation**: A living cell with fewer than 2 neighbors dies
3. **Death by Overpopulation**: A living cell with more than 3 neighbors dies
4. **Birth**: A dead cell with exactly 3 living neighbors becomes alive

## Implementation Details

### Parallel Strategy
- **Board Division**: Grid is divided into horizontal slices of equal size
- **Process Distribution**: Each MPI process handles one slice of the board
- **Boundary Communication**: Processes exchange boundary rows to calculate neighbor counts
- **Solid Walls**: Cells at the board edges are treated as having dead neighbors outside the boundary

### Key Features
- **Even Grid Support**: Works correctly with even number of rows and columns
- **Scalable Processing**: Automatically determines optimal number of processes
- **Memory Efficient**: Each process only stores its assigned slice
- **Synchronized Evolution**: All processes compute the next generation simultaneously

## File Structure

```
life.cpp         # Main implementation
test.sh          # Automated build and execution script
<input_file>     # Grid configuration file
```

## Input Format

The input file should contain the initial board state as a grid of 0s and 1s:
```
0110
1001
0110
1001
```

Where:
- `0` represents a dead cell
- `1` represents a living cell

## Compilation and Execution

### Manual Compilation
```bash
mpic++ --prefix /usr/local/share/OpenMPI -o life life.cpp
```

### Manual Execution
```bash
mpirun --prefix /usr/local/share/OpenMPI --use-hwthread-cpus -np <num_processes> life <input_file> <generations>
```

### Automated Execution (Recommended)
```bash
chmod +x test.sh
./test.sh <input_file> <generations>
```

## Usage Examples

### Basic Usage
```bash
# Run for 5 generations with input file "board.txt"
./test.sh board.txt 5

# Show initial state (0 generations)
./test.sh board.txt 0
```

### Sample Input File (4x4 grid)
```bash
echo -e "0110\n1001\n0110\n1001" > sample.txt
./test.sh sample.txt 3
```

## Algorithm Implementation

### Process Roles

#### Root Process (rank = 0)
- Reads input file and parses the initial board state
- Distributes board slices to all processes (including itself)
- Coordinates the parallel computation
- Collects and displays final results

#### All Processes
- Receive assigned board slice and configuration
- Execute the generational loop:
  1. Exchange boundary rows with neighboring processes
  2. Calculate neighbor counts for each cell
  3. Apply Game of Life rules
  4. Update cell states for next generation
- Send final slice back to root for output

### Communication Pattern
- **Boundary Exchange**: Each process sends its top/bottom rows to adjacent processes
- **Result Collection**: All processes send final state to root for ordered output
- **Synchronization**: Blocking MPI operations ensure synchronized generation updates

## Technical Specifications

### Requirements
- **OpenMPI**: MPI library for parallel processing
- **Even Dimensions**: Board must have even number of rows and columns
- **Sufficient Processes**: Number of processes must divide evenly into number of rows

### Performance Characteristics
- **Time Complexity**: O(generations × rows × columns / processes)
- **Space Complexity**: O(rows × columns / processes) per process
- **Communication Overhead**: O(columns) per generation for boundary exchange
- **Scalability**: Linear speedup up to optimal process count

### Automatic Process Selection
The test.sh script automatically:
1. Counts board dimensions from input file
2. Determines available CPU cores
3. Finds optimal number of processes that:
   - Evenly divides the number of rows
   - Doesn't exceed available hardware threads
   - Minimizes communication overhead

## Output Format

Each generation's final state is displayed with process rank prefixes:
```
0: 0110
0: 1001
1: 0110
1: 1001
```

Where the number before the colon indicates which process computed that slice.

## Limitations

- **Board Size**: Must have even number of rows and columns
- **Memory Constraints**: Large boards may exceed memory limits
- **Process Count**: Limited by available system cores and board divisibility
- **File Format**: Input must be exactly formatted (no spaces, consistent line lengths)

## Error Handling

- **File Access**: Validates input file existence and readability
- **Parameter Validation**: Checks for correct number of command-line arguments
- **Generation Count**: Ensures non-negative generation values
- **MPI Errors**: Uses MPI_Abort for critical failures

## Example Execution Flow

1. **Initialization**: MPI processes start and get rank assignments
2. **Input Processing**: Root reads board and determines slice sizes
3. **Distribution**: Board slices distributed to all processes
4. **Evolution Loop**: For each generation:
   - Exchange boundary rows
   - Calculate new cell states
   - Synchronize for next iteration
5. **Output**: Root collects and displays final board state
6. **Cleanup**: MPI environment finalized
