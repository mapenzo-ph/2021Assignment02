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
    float_t split[NDIM];
    int axis;
    int left;
    int right;
};

// declare MPI datatype for comms as global variable
MPI_Datatype MPI_kdnode_t;
int iteration = 0;
// ==================================================================
//                          Helper functions
// ==================================================================
kdnode_t *parseInputFile(int mpi_rank)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and stores points into an
     * array. Returns the array.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // allocate nodes
    if (mpi_rank == 0)
    {
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
        fclose(fp);
        return tree;
    }
    else return NULL;
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

int growTreeSerial(kdnode_t *tree, int low, int high, int axis, int offset)
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
        return offset + low;
    }

    if (low > high)
    {
        return -1;
    }

    // recursion strategy
    int median = (high+low)/2;
    int new_axis = (axis + 1) % NDIM;
    
    // assign values to median node
    quicksort(tree, low, high, axis);
    (tree+median) -> axis = axis;

    // grow subtrees
    (tree+median)->left = growTreeSerial(tree, low, median-1, new_axis, offset);
    (tree+median)->right = growTreeSerial(tree, median+1, high, new_axis, offset);
    
    return offset+median;
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

int growTree(kdnode_t *tree, int low, int high, int axis, MPI_Comm comm, int offset)
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
        return growTreeSerial(tree, low, high, axis, offset);
    }

    // parallel case
    int median = (high + low) / 2;
    int new_axis = (axis + 1) % NDIM;
    int right_count = high - median;
    MPI_Status status;

    if (comm_rank == 0)
    {
        // if root: quicksort, assign axis to median node, send right points to 1
        quicksort(tree, low, high, axis);
        MPI_Send(tree+median+1, right_count, MPI_kdnode_t, 1, 10*comm_size, comm);
    }
    else if (comm_rank == 1)
    {
        // if proc 1: allocate space for points and then receive
        tree = (kdnode_t *)malloc(right_count*sizeof(kdnode_t));
        MPI_Recv(tree, right_count, MPI_kdnode_t, 0, 10*comm_size, comm, &status);
    }

    // create left and right communicators
    MPI_Comm new_comm;
    splitComms(comm, &new_comm);

    // call next step
    int nleft, nright;
    if (comm_rank % 2 == 0)
    {
        nleft = growTree(tree, 0, median-1, new_axis, new_comm, offset);
    }
    else
    {
        nright = growTree(tree, 0, right_count-1, new_axis, new_comm, offset+median+1);
    }

    // send points back to root
    if (comm_rank == 0)
    {   
        MPI_Recv(&nright, 1, MPI_INT, 1, 11*comm_size, comm, &status);
        printf("left %d, right %d\n", nleft, nright);
        (tree+median) -> axis = axis;
        (tree+median) -> left = nleft;
        (tree+median) -> right = nright;
        // receive reordered points
        MPI_Recv(tree+median+1, right_count, MPI_kdnode_t, 1, 13*comm_size, comm, &status);
        
    }
    else if (comm_rank == 1)
    {
        MPI_Send(&nright, 1, MPI_INT, 0, 11*comm_size, comm);
        // send reordered points
        MPI_Send(tree, right_count, MPI_kdnode_t, 0, 13*comm_size, comm);
        free(tree);
    }

    return offset + median;
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
    int blocklen[2] = {NDIM, 3};
    MPI_Aint disp[2] = {0, NDIM*sizeof(float_t)};
    MPI_Datatype oldtypes[2] = {MPI_FLOAT_T, MPI_INT};
    MPI_Type_create_struct(
        2, blocklen, disp, oldtypes, &MPI_kdnode_t);
    MPI_Type_commit(&MPI_kdnode_t);

    // get size and ranks
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    
    // read file
    kdnode_t *tree = parseInputFile(mpi_rank);

    // grow the tree and take time
    double time = MPI_Wtime();  
    int root = growTree(tree, 0, NPTS-1, 0, MPI_COMM_WORLD, 0);
    time = MPI_Wtime() - time;
    
#ifndef NDEBUG
    // print tree for debug
    printTree(tree, 0, mpi_rank);
#endif
    
    // average runtimes of different procs
    double avg_time;
    MPI_Reduce(&time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (mpi_rank == 0)
    {
        // print information about the tree
        printf("Tree grown in %lfs\n", avg_time/mpi_size);
        printf("Tree root is at node %d\n", root);
    }
    

    MPI_Finalize();
    return 0;
}