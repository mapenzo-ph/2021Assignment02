#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

// ==================================================================
//                      Set input data parameters
// ==================================================================
#define INFILE "../test_data.csv"
#define NPTS 50
#define NDIM 2
#define SEP ','

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

// ==================================================================
//                          Struct definitions
// ==================================================================
typedef struct kdnode kdnode_t;
struct kdnode
{
    int axis;
    float_t split[NDIM];
    int left, right;
};

// declare MPI datatype for comms as global variable
MPI_Datatype MPI_kdnode_t;

// ==================================================================
//                          Helper functions
// ==================================================================
kdnode_t *parseInputFile()
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and stores points into an
     * array. Returns the array.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // allocate nodes
    kdnode_t *tree = (kdnode_t *)malloc(NPTS * sizeof(kdnode_t));

    // open input file
    FILE *fp = fopen(INFILE, "r");
    if (fp == NULL)
    {
        perror("Unable to open input file! Exiting...\n");
        exit(1);
    }

    // set format for reading
    char FMTSEP[] = FMT;
    char sepstr[] = {SEP, '\0'};
    strncat(FMTSEP, sepstr, 1);

    int r;
    for (int np = 0; np < NPTS; ++np)
    {
        for (int nc = 0; nc < NDIM - 1; ++nc)
        {
            r = fscanf(fp, FMTSEP, (tree + np)->split + nc);
            if (r == EOF)
            {
                perror("Unexpected end of file while parsing. Exiting...");
                exit(2);
            }
        }
        r = fscanf(fp, FMTLAST, (tree + np)->split + NDIM - 1);
        if (r == EOF)
        {
            perror("Unexpected end of file while parsing. Exiting...");
            exit(3);
        }
    }
    rewind(fp);
    return tree;
}

void swapFloats(float_t *a, float_t *b)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Swaps memory position of a and b.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    float_t tmp = *a;
    *a = *b;
    *b = tmp;
}

void swapNodes(kdnode_t *a, kdnode_t *b)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Swaps memory position of two nodes
     * * * * * * * * * * * * * * * * * * * * * * * * */

    for (size_t i = 0; i < NDIM; ++i)
    {
        swapFloats(a->split + i, b->split + i);
    }
}

int partition(kdnode_t *nodes, int low, int high, int axis)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Organizes nodes between @params start and 
     * @params stop along @param axis in smaller 
     * and larger than the pivot element (high).
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low >= high) return low; // single node case}

    float_t pivot = (nodes+high)->split[axis];
    // printf("Pivot: %6.3lf\n", pivot);

    int i = low - 1;

    for (int j = low; j < high; ++j)
    {
        if ((nodes+j)->split[axis] <= pivot)
        {
            ++i;
            swapNodes(nodes+i, nodes+j);
        }
    }
    swapNodes(nodes+i+1, nodes+high);
    return i+1;   
}

void quicksort(kdnode_t *nodes, int low, int high, int axis)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Performs a parallel quicksort using openMP tasks.
     * Assumes a parallel region has been opened.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low >= high) return; // single node case

    // partitioning points
    int pivot = partition(nodes, low, high, axis);
    
    // task to quicksort lower/upper sides
    quicksort(nodes, low, pivot-1, axis);
    quicksort(nodes, pivot, high, axis);
}

void growTreeSerial(kdnode_t *tree, int low, int high, int axis, int *idx)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * growTree serial for when communicator is just 
     * one process
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low == high) // leaf handling
    {
        (tree + low)->axis = axis;
        (tree + low)->left = -1;
        (tree + low)->right = -1;
        *idx = low;
        return;
    }

    if (low > high)
    {
        *idx = -1;
        return;
    }

    // quicksort and take median
    quicksort(tree, low, high, axis);
    int median = (high+low)/2;
    int new_axis = (axis + 1) % NDIM;

    // assign values to median node
    (tree+median) -> axis = axis;
    *idx = median;

    // grow subtrees
    growTreeSerial(tree, low, median-1, new_axis, &((tree+median)->left));
    growTreeSerial(tree, median+1, high, new_axis, &((tree+median)->right));
}

void splitComms(MPI_Comm comm, MPI_Comm *new_comm)
{
    // copy in comm
    int rank, color, key;

    MPI_Comm_rank(comm, &rank);
    color = rank % 2;
    key = rank / 2;

    MPI_Comm_split(comm, color, key, new_comm);
}

kdnode_t *growTree(kdnode_t *tree, int low, int high, int axis, MPI_Comm comm, int *idx)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Wrapper for the growTreeParallel and Serial, 
     * uses one or the other depending on the size
     * of the communicator
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // get size and ranks in communicator
    int comm_size, comm_rank;
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    // single process in communicator case
    if (comm_size <= 1)
    {
        growTreeSerial(tree, low, high, axis, idx);
        return tree;
    }

    // parallel case
    int median = (high + low) / 2;
    int new_axis = (axis + 1) % NDIM;
    int right_count = high - median;
    MPI_Status status;
    *idx = median;

    if (comm_rank == 0)
    {
        // if root: quicksort, assign axis to median node, send right points to 1
        quicksort(tree, low, high, axis);
        (tree + median)->axis = axis;
        printf("first print 0\n");
        MPI_Send(tree+median+1, right_count, MPI_kdnode_t, 1, 10*comm_size, comm);
    }
    else if (comm_rank == 1)
    {
        // if proc 1: allocate space for points and then receive
        tree = (kdnode_t *)malloc(right_count*sizeof(kdnode_t));
        // receive nodes
        printf("first print 1\n");
        MPI_Recv(tree, right_count, MPI_kdnode_t, 0, 10*comm_size, comm, &status);
    }

    // create left and right communicators
    MPI_Comm new_comm;
    splitComms(comm, &new_comm);

    // call next step
    int left, right;
    if (comm_rank % 2 == 0)
    {
        growTree(tree, 0, median-1, new_axis, new_comm, &left);
    }
    else
    {
        growTree(tree, 0, right_count-1, new_axis, new_comm, &right);
        right = median + right;
        // send right value to all processes
        // MPI_Bcast(&right, 1, MPI_INT, 1, comm);
    }

    // send points back to root
    if (comm_rank == 0)
    {
        // receive reordered points
        MPI_Recv(tree+median+1, right_count, MPI_kdnode_t, 1, 10*comm_size, comm, &status);
        (tree + median)->left = left;
        (tree + median)->right = right;
    }
    else if (comm_rank == 1)
    {
        // send reordered points
        MPI_Send(tree, right_count, MPI_kdnode_t, 0, 10*comm_size, comm);
        free(tree);
    }

    return tree;
}

void printTree(kdnode_t *tree, int branch, int mpi_rank)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the three
     * * * * * * * * * * * * * * * * * * * * * * * * */
    if (mpi_rank == branch)
    {
        printf("\n\nPrinting branch %d\n\n", branch);
        for (int np = 0; np < NPTS; ++np)
        {
            printf("idx: %3d | vals: ", np);
            for (size_t nc = 0; nc < NDIM; ++nc)
            {
                printf(PFMT, (tree + np)->split[nc]);
            }
            printf(" | ax: %2d | children: %3d , %3d\n",
                (tree + np)->axis, (tree + np)->left, (tree + np)->right);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

// ==================================================================
//                          MAIN PROGRAM
// ==================================================================
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    // define custom MPI datatype for passing kdnode
    int blocklen[3] = {1, NDIM, 2};
    MPI_Aint disp[3] = {0, sizeof(int), sizeof(int)+NDIM*sizeof(float_t)};
    MPI_Datatype oldtypes[3] = {MPI_INT, MPI_FLOAT_T, MPI_INT};
    MPI_Type_create_struct(
        3, blocklen, disp, oldtypes, &MPI_kdnode_t);
    MPI_Type_commit(&MPI_kdnode_t);

    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    
    int root;

    // read data in root process
    kdnode_t *tree = NULL;
    if (mpi_rank == 0)
    {
        tree = parseInputFile();
        // quicksort(tree, 0, NPTS-1, 0);
        // growTreeSerial(tree, 0, NPTS-1, 0, &root);
        //printTree(tree);
    }

    // int new_rank;
    // MPI_Comm new_comm;
    // splitComms(MPI_COMM_WORLD, &new_comm);
    // MPI_Comm_rank(new_comm, &new_rank);

    // printf("Proc %d has rank %d in its new comm\n", mpi_rank, new_rank);
    
    tree = growTree(tree, 0, NPTS-1, 0, MPI_COMM_WORLD, &root);
    
    printTree(tree, 0, mpi_rank);
    // printTree(tree, 1, mpi_rank);

    MPI_Finalize();
    return 0;
}