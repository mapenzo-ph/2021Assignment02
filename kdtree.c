#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// ========================================================
//                  Set precision
// ========================================================
#ifndef DOUBLE_PRECISION
#define float_t float
#else
#define float_t double
#endif

// ========================================================
//                  Struct definitions
// ========================================================


// ========================================================
//                  Helper functions
// ========================================================
int countLines(FILE *fstream)
{
    /*************************************************
    * Counts number of lines in file stream.
    *************************************************/

    if (fstream == NULL) return 0;
    
    int count = 0;
    for (char r; fscanf(fstream, "%c", &r) == 1; )
    {
        if (r == '\n') ++count;
    }
    rewind(fstream); // turn back to the top of file
    return count;
}

int countDims(FILE *fstream, char sep)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Counts data entries on one line of file, assuming
     * data separator is @param sep.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (fstream == NULL) return 0;
    
    int count = 1; // dims are 1 + num of separators
    for (char r; fscanf(fstream, "%c", &r) == 1; )
    {
        if (r == sep) ++count;
        if (r == '\n') break;
    }
    rewind(fstream); // turn back to the top of file
    return count;
}

char* getParserString(int dims, char sep)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Composes appropriate string for parsing file
     * and reading data
     * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DOUBLE_PRECISION
    // single precision parser
    char *parser = (char*)malloc((3*dims+1)*sizeof(char));
    char fmt[3] = "%f";
    int nadd = 2;
#else
    // double precision parser
    char *parser = (char*)malloc((4*dims+1)*sizeof(char));
    char fmt[4] = "%lf";
    int nadd = 3;
#endif 

    char sepstr[2] = {sep, '\0'};   // separator string
    char newline[2] = {'\n', '\0'}; // newline string
    for (int i = 0; i < dims-1; ++i)
    {
        // concatenating to create parser string
        strncat(parser, fmt, nadd);
        strncat(parser, sepstr, 1);
    }
    strncat(parser, fmt, nadd);
    strncat(parser, newline, 1);
    return parser;
}

void parse(char *filename, char sep, int mpi_size, int mpi_rank, int *size, int *dims)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and reads datapoints.
     * @param char* string (in) -> input file path
     * @param char sep     (in) -> data separator (in file)
     * @param int* size   (out) -> address of size variable 
     * @param int* dims   (out) -> address of dims variable
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (mpi_rank == 0)
    {
        // handling input via root process
        FILE *instream = fopen(filename, "r");
        if (instream == NULL)
        {
             perror("Unable to open file. Aborting ...\n");
             exit(1);
        }
        *size = countLines(instream);
        *dims = countDims(instream, sep);
        fclose(instream);
    }

        // broadcast size
        MPI_Bcast(size, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // broadcast dims
        MPI_Bcast(dims, 1, MPI_INT, 0, MPI_COMM_WORLD);

}
    




// void print_head(kdpoint *points, size_t len)
// {
//     /*************************************************
//     * Prints the first len entries of the dataset
//     *
//     * @param kdpoint *points: pointer to the dataset
//     * @param size_t len: number of entries to be printed
//     *************************************************/

//     // build string to print datapoint
// #ifndef DOUBLE_PRECISION
//     char fmt[7] = {'%', '5', '.', '3', 'f', ' ', '\0'};
//     char fmtlast[7] = {'%', '5', '.', '3', 'f', '\n', '\0'};
// #else
//     char fmt[8] = {'%', '5', '.', '3', 'l', 'f', ' ', '\0'};
//     char fmtlast[8] = {'%', '5', '.', '3', 'l', 'f', '\n', '\0'};
// #endif

//     // print point by point, coord by coord
//     for (size_t i=0; i<len; ++i)
//     {
//         for (size_t coord=0; coord<NDIM-1; ++coord)
//         {
//             printf(fmt, (points+i)->data[coord]); 
//         }
//         printf(fmtlast, (points+i)->data[NDIM-1]);
//     }
// }

// void swap(float_t *a, float_t *b)
// {
//     /*
//     * Swaps two values a and b
//     */
//     float_t tmp = *a;
//     *a = *b;
//     *b = tmp;
// }

// void swap_points(kdpoint *a, kdpoint *b)
// {
//     /*
//     * Swaps two vectors acting coordinate by coordinate
//     */
//     for (size_t i=0; i<NDIM; ++i)
//     {
//        swap(a->data+i, b->data+i);
//     }
// }

// void partition(kdpoint *start, size_t len, size_t p, size_t axis)
// {
//     /*
//     * Sorts elements along the chosen dimension, with respect to 
//     * a pivotal element 
//     * 
//     * @params kdpoint *points: array of points to be sorted
//     * @params size_t len: lenght of the array
//     * @params size_t p: coordinate of the pivot
//     * @params size_t dim: dimension alog which to sort
//     */

//     if (p > len-1)
//     {
//         printf("Pivot must be inside the range of points. Aborting...\n");
//         exit(-1);
//     }

//     if (len <= 1) return; // single point case
    
//     // take selected element as pivot and put in front
//     float_t pivot = (start+p)->data[axis];
//     printf("Pivot: %lf\n", pivot);

//     kdpoint *left = start;
//     kdpoint *right = start+len-1;
    
//     while (left <= right)
//     {
//         while (right->data[axis] > pivot && left<=right)
//         {
//             printf("Moving right\n");
//             --right;
//         }
//         while (left->data[axis] < pivot && left <= right)
//         {
//             printf("Moving left\n");
//             ++left;
//         }
//         if (left->data[axis] >= right->data[axis] && left <= right)
//         {
//             printf("Swapping\n");
//             swap_points(left, right);
//             --right;
//             ++left;
//         }
//     }
//     return;
// }

// ========================================================
//                      Main program
// ========================================================
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    int size, dims;
    parse("test_data.csv", ',', mpi_size, mpi_rank, &size, &dims);

    printf("My rank is %d, size is %d and dims are %d\n", mpi_rank, size, dims);

    MPI_Finalize();
    return 0;
}