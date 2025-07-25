/**
 * @file life.cpp
 * @author Bc. Martin Baláž
 * @date 25.4.2024
 * @brief This file implements Game of Life (no-person game) using library OpenMPI.
 *        It is implemented using solid walls, so the cells on the edges are not affected by the cells outside the board.
 *        The board is divided into slices (2 lines or more), each slice is processed by one processor. All slices have same size.
 *        Program works correctly only for even number of lines and columns.
 * @note The program will not work for extremely large boards!!
 */

#include "mpi.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <vector>

using namespace std;

// Constants
const int MASTER = 0; // Rank of the root process
const int TAG = 0; // Tag for messages (not needed)

const int COLUMNS = 0; // Index of the number of columns in the message vector
const int SLICEROWS = 1; // Index of the number of slice rows in the message vector
const int GENERATIONS = 2; // Index of the number of generations in the message vector

/**
 * @brief Function for root process (rank = 0), that reads the board from the file and sends the slices to all other processes including itself.
 * @param size Number of processes.
 * @param rank Rank of the process.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return void
*/
void processRoot(int size, int rank, int argc, char *argv[])
{

    ifstream file(argv[1]); // Input file
    int generations = atoi(argv[2]); // Number of steps to simulate

    if (!file)
    {
        cerr << "Error opening file (Try checking the name of file)" << endl;
        cerr << "Usage: ./test.sh <input file> <number of generations>" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1); // Abort the MPI execution environment
    }

    vector<vector<int>> board; // 2D vector representing the board
    string line; // Line of the board

    // Read the board from a file to a 2D vector of integers
    while (getline(file, line))
    {
        vector<int> row;
        // '0' have ASCII value of 48 and numbers (1-9) have ASCII values from 49 to 57. This way we convert char to int
        for (char c : line) row.push_back(c - '0'); 
        board.push_back(row);
    }
    file.close();

    // In case of 0 generations, print the initial state and exit
    if (generations == 0)
    {
        // Print and remove lines from the vector until it'sliceRows empty
        while (!board.empty())
        {
            std::cout << rank << ": ";
            for (int val : board.back()) std::cout << val; // Print all elements of the last row
            std::cout << endl;
            board.pop_back(); // Remove the last row
        }
    } 
    else if (generations < 0)
    {
        cerr << "Number of generations must be a non-negative integer" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1); // Abort the MPI execution environment
    }

    // MxN board of even size
    int rows = board.size();
    int columns = board[0].size();
    int sliceRows = rows / size; // Number of rows in a slice (every slice has the same number of rows)
    int slice[sliceRows][columns]; // Slice of the board

    // Vector containing information about the slice size and number of generations
    vector<int> sendInfoVector = {columns, sliceRows, generations};

    // Send the information to all other processes including itself
    for (int dest = 0; dest < size; dest++) MPI_Send(sendInfoVector.data(), 3, MPI_INT, dest, TAG, MPI_COMM_WORLD);
    
    for (int p = 0; p < size; p++) // For each processor (thread)
    {
        for (int row = 0; row < sliceRows; row++) // For each row in a slice
        {                         
            for (int column = 0; column < columns; column++) // For each column in a slice
            {
                slice[row][column] = board[row + (p * sliceRows)][column]; // Fill the slice with the board values
            }
        }
        // Slice size = columns * sliceRows;
        MPI_Send(&slice, columns * sliceRows, MPI_INT, p, 2, MPI_COMM_WORLD); // And send it to all processors
    }

}

/**
 * @brief Function that processes the generations of the game of life with all processors using the slices of the board.
 * @param size Number of processes.
 * @param rank Rank of the process.
 * @return void
*/
void generationsLoop(int size, int rank) {

    // Vector containing information about the board size and number of generations
    vector<int> receiveInfoVector = {0, 0, 0};

    // Receive the information about the slice size and number of generations
    MPI_Recv(receiveInfoVector.data(), 3, MPI_INT, MASTER, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    int columns = receiveInfoVector[COLUMNS];
    int sliceRows = receiveInfoVector[SLICEROWS];
    int generations = receiveInfoVector[GENERATIONS];
    int slice[sliceRows][columns];
    int tmpSlice[sliceRows][columns]; // Temporary slice will be used to store the new values of cells

    MPI_Recv(&slice, columns * sliceRows, MPI_INT, MASTER, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the slice from the root process

    vector<int> toBottom(columns), fromTop(columns), toTop(columns), fromBottom(columns); // Vectors for sending/receiving rows to/from other processors

    for (int g = 1; g <= generations; g++) // For each step (generation) of the game
    {

        // Every processor except for the last one will send its last row to the next processor under it
        if (rank != size - 1) 
        {
            for (int j = 0; j < columns; j++) toBottom[j] = slice[sliceRows - 1][j]; // Last row of the slice is sent down to the next processor
            MPI_Send(toBottom.data(), columns, MPI_INT, rank + 1, 3, MPI_COMM_WORLD);
        }
        else fill(fromBottom.begin(), fromBottom.end(), 0); // Last processor has no next processor, so it doesn't send anything

        // Every processor except for the first one will receive the last row from the previous processor above it
        if (rank != 0) MPI_Recv(fromTop.data(), columns, MPI_INT, rank - 1, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the "toBottom" from the previous processor
        else fill(fromTop.begin(), fromTop.end(), 0); // First processor has no previous processor, so it doesn't receive anything

        // Every processor except for the first one will send its first row to the previous processor above it
        if (rank != 0) 
        {
            for (int j = 0; j < columns; j++) toTop[j] = slice[0][j]; // First row of the slice is sent up to the previous processor
            MPI_Send(toTop.data(), columns, MPI_INT, rank - 1, 4, MPI_COMM_WORLD);
        }

        // Every processor except for the last one will receive the first row from the next processor under it
        if (rank != size - 1) MPI_Recv(fromBottom.data(), columns, MPI_INT, rank + 1, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the "toTop" from the next processor

        // Calculate the sum of alive neighbors for each cell in the slice
        int sum = 0; // Sum of alive neighbors
        for (int x = 0; x < sliceRows; x++) // For each row in the slice
        {
            for (int y = 0; y < columns; y++) // For each column in the slice
            {
                sum = 0; // Reset the sum for each cell
                for (int dx = -1; dx <= 1; dx++) // For each neighbor in the x direction
                {
                    for (int dy = -1; dy <= 1; dy++) // For each neighbor in the y direction
                    {
                        // Skip the cell itself
                        if (dx == 0 && dy == 0) continue;

                        // Calculate the coordinates of the neighbor
                        int nx = x + dx;
                        int ny = y + dy;

                        // Check if the neighbor is within the bounds of the slice
                        if (nx >= 0 && nx < sliceRows && ny >= 0 && ny < columns) sum += slice[nx][ny];
                        // If the neighbor is outside the slice, check the corresponding row from the neighboring slice
                        // If there is no neighboring slice, the array is filled with zeros, so nothing will be added to the sum
                        else if (nx < 0) sum += fromTop[ny];
                        else if (nx >= sliceRows) sum += fromBottom[ny];
                    }
                }

                // Apply the rules of the game to each cell using sum, which now contains the number of living neighbors of the cell
                if (slice[x][y] == 1 && sum < 2) tmpSlice[x][y] = 0; // Live cell with less than 2 living neighbours dies
                else if (slice[x][y] == 1 && (sum == 2 || sum == 3)) tmpSlice[x][y] = 1; // Live cell with 2 or 3 living neighbours lives
                else if (slice[x][y] == 1 && sum > 3) tmpSlice[x][y] = 0; // Live cell with more than 3 living neighbours dies
                else if (slice[x][y] == 0 && sum == 3) tmpSlice[x][y] = 1; // Dead cell with 3 living neighbours lives
                else tmpSlice[x][y] = 0; // Dead cell with other than 3 living neighbours stays dead
            }
        }

        // Once the final generation is reached, print the board
        if (g == generations)
        {
            // Only the root process will print the board
            if (rank == 0)
            {
                int sliceToPrint[sliceRows][columns];
                for (int x = 0; x < sliceRows; x++) // Print the root's slice
                {
                    cout << rank << ": ";
                    for (int y = 0; y < columns; y++) cout << tmpSlice[x][y]; // Print the row of the slice
                    cout << endl;
                }
                for (int i = 1; i < size; i++)
                {
                    MPI_Recv(&sliceToPrint, columns * sliceRows, MPI_INT, i, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the slice from the other processors
                    for (int x = 0; x < sliceRows; x++) // Print the slices from the other processors
                    {
                        cout << i << ": "; // Print the corresponding rank of the processor
                        for (int y = 0; y < columns; y++) cout << sliceToPrint[x][y];
                        cout << endl;
                    }
                }
            }
            // As non-root process, send the slice to the root process
            else MPI_Send(&tmpSlice, columns * sliceRows, MPI_INT, MASTER, 5, MPI_COMM_WORLD);
        }

        // Copy temporary slice into original slice
        for (int x = 0; x < sliceRows; x++)
        {
            for (int y = 0; y < columns; y++) 
            {
                slice[x][y] = tmpSlice[x][y]; // It will be used in the next generation
            }
        }

    }
}

/**
 * @brief Main function that initializes MPI, gets the rank and size of the process and calls the appropriate function for the root, and then the loop for all processes.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0
*/
int main(int argc, char* argv[])
{
    MPI_Init(&argc,&argv);
    int rank, size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) processRoot(size, rank, argc, argv);
    generationsLoop(size, rank);

    MPI_Finalize();
    return 0;
}