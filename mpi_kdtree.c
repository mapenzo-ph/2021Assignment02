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
#define MPI_FLOAT_T MPI_FLOAT
#define FMT "%f"
#define FMTLAST "%f\n"
#define PFMT "%6.3f "
#define PFMTLAST "%6.3f\n"
#else
#define float_t double
#define MPI_FLOAT_T MPI_DOUBLE
#define FMT "%lf"
#define FMTLAST "%lf\n"
#define PFMT "%6.3lf "
#define PFMTLAST "%6.3lf\n"
#endif

// ========================================================
//          Struct definitions and global vars
// ========================================================
int mpi_size; // global, no need to pass as arg in functions
int mpi_rank; 

typedef struct node node;
struct node {
    int axis;
    float_t *point;
    node *left, *right;
};

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

float_t *parse(char *filename, char sep, char size, char dims)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses @param filename, and reads datapoints
     * using @param sep as data separator.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    float_t *buffer = (float_t *)malloc(size * dims * sizeof(float_t));
    FILE *fp = fopen(filename, "r");
    char FMTSEP[] = FMT;
    char sepstr[] = {sep, '\0'};
    strncat(FMTSEP, sepstr, 1);
    int r;
    for (int np = 0; np < size; ++np)
    {
        for (int nc = 0; nc < dims - 1; ++nc)
        {
            r = fscanf(fp, FMTSEP, buffer + (np * dims) + nc);
            if (r == EOF)
            {
                perror("Unexpected end of file while parsing. Exiting...");
                exit(2);
            }
        }
        r = fscanf(fp, FMTLAST, buffer + (np * dims) + dims - 1);
        if (r == EOF)
        {
            perror("Unexpected end of file while parsing. Exiting...");
            exit(3);
        }
    }
    fclose(fp);
    return buffer;
}

void head(float_t *buffer, int len, int dims)
{
    /*************************************************
    * Prints the first @param len entries from
    * @param buffer.
    *************************************************/

    // print point by point, coord by coord
    for (size_t np = 0; np < len; ++np)
    {
        for (size_t nc = 0; nc < dims-1; ++nc)
        {
            printf(PFMT, buffer[np*dims+nc]); 
        }
        printf(PFMTLAST, buffer[np*dims+dims-1]);
    }
}


// ========================================================
//                      Main program
// ========================================================
int main(int argc, char **argv)
{
    // init and get parameters
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    MPI_Finalize();
    return 0;
}