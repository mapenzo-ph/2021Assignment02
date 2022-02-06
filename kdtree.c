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

void getLocalParams(
    char *filename, char sep, int *size, int *local_size, int *dims)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Writes in @param size, @param int and 
     * @param localSizes the corresponding values.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (mpi_rank == 0)
    {
        // read data from input file
        FILE *instream = fopen(filename, "r");
        if (instream == NULL)
        {
            perror("Unable to open file. Exiting...\n");
            exit(1);
        }
        *size = countLines(instream);
        *dims = countDims(instream, sep);
        fclose(instream);
    }

    // communicate to other processes
    MPI_Bcast(size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(dims, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // get local size in each process
    int rest = *size % mpi_size;
    *local_size = (mpi_rank < rest) ? *size / mpi_size + 1
                                    : *size / mpi_size;
}

void parseLocalPoints(
    FILE *fp, char sep, int npoints, int dims, float_t *buffer)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Reads @param npoints from @param fp 
     * into @param buffer.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    char FMTSEP[] = FMT;
    char sepstr[] = {sep, '\0'};
    strncat(FMTSEP, sepstr, 1);
    int r;
    for (int np = 0; np < npoints; ++np)
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
}

void parse(
    char *filename, char sep, int local_size, int dims, float_t *data)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses @param filename and distributes data 
     * across all the mpi processes. 
     * * * * * * * * * * * * * * * * * * * * * * * * */

    MPI_Status status;

    if (mpi_rank == 0)
    {
        FILE *fp = fopen(filename, "r");
        if (fp == NULL)
        {
            perror("Failed to open input file. Exiting...");
            exit(4);
        }

        // directly read points in process 0
        parseLocalPoints(fp, sep, local_size, dims, data);

        // read in send buffer and send to other processes
        float_t send_buffer[local_size*dims];
        int send_size;
        for (int proc = 1; proc < mpi_size; ++proc)
        {
            // receive send size
            MPI_Recv(
                &send_size, 1, MPI_INT, proc, 
                proc*10, MPI_COMM_WORLD, &status);

            printf("to rank %d, size %d\n", proc, send_size);
            parseLocalPoints(fp, sep, send_size, dims, send_buffer);
            
            // send to destination process
            MPI_Send(
                send_buffer, send_size*dims, MPI_FLOAT_T, proc, 
                proc*10, MPI_COMM_WORLD);
        }

        fclose(fp);
    }
    else
    {
        MPI_Send(
            &local_size, 1, MPI_INT, 0, 
            mpi_rank*10, MPI_COMM_WORLD);

        // receive local data
        MPI_Recv(
            data, local_size*dims, MPI_FLOAT_T, 0, 
            mpi_rank*10, MPI_COMM_WORLD, &status);
    }
}
    
void head(float_t *points, int len, int dims)
{
    /*************************************************
    * Prints the first @param len entries in the 
    * @param points buffer.
    *************************************************/

    // print point by point, coord by coord
    for (size_t np = 0; np < len; ++np)
    {
        for (size_t nc = 0; nc < dims-1; ++nc)
        {
            printf(PFMT, points[np*dims+nc]); 
        }
        printf(PFMTLAST, points[np*dims+dims-1]);
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

    int size, dims, local_size;
    getLocalParams("test_data.csv", ',', &size, &local_size, &dims);
    printf("My rank is %d, size is %d, dims is %d and my local size is %d\n",
           mpi_rank, size, dims, local_size);
    
    MPI_Barrier(MPI_COMM_WORLD);

    float_t *points = (float_t *)malloc(local_size * dims * sizeof(float_t));
    
    parse("test_data.csv", ',', local_size, dims, points);
    if (mpi_rank == 0)
    {
        printf("Process %d\n", mpi_rank);
        head(points, local_size, dims);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (mpi_rank == 1)
    {
        printf("Process %d\n", mpi_rank);
        head(points, local_size, dims);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (mpi_rank == 2)
    {
        printf("Process %d\n", mpi_rank);
        head(points, local_size, dims);
    }

    MPI_Finalize();
    return 0;
}