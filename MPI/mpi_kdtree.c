#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

// ==================================================================
//                      Input parameters
// ==================================================================
#define DATA "../test_data.csv"
#define NPTS 50
#define NDIM 2
#define SEP ','

// ==================================================================
//                      Set precision
// ==================================================================
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
//                          Struct definition
// ==================================================================
typedef struct kdnode kdnode_t;
struct kdnode
{
    float_t split[NDIM];
    int axis, left, right;
};

// declare MPI datatype for comms as global variable
MPI_Datatype MPI_kdnode_t;

// ==================================================================
//                          User functions
// ==================================================================
kdnode_t *parseFile(int mpi_rank)
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
        FILE *fp = fopen(DATA, "r");
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
    else
        return NULL;
}

void printTree(kdnode_t *tree, int mpi_rank)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the three
     * * * * * * * * * * * * * * * * * * * * * * * * */
    if (mpi_rank == 0)
    {
        printf("\n\n");
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
        printf("\n");
    }
}

inline void swap(kdnode_t *a, kdnode_t *b)
{
    float_t tmp[NDIM];
    memcpy(tmp, a->split, sizeof(tmp));
    memcpy(a->split, b->split, sizeof(tmp));
    memcpy(b->split, tmp, sizeof(tmp));
}

int findMedian(kdnode_t *data, int start, int end, int axis)
{
    if (end <= start)
        return -1;
    if (end == start + 1)
        return start;

    int p, store;
    int md = start + (end - start) / 2;
    double pivot;

    while (1)
    {
        // take median as pivot
        pivot = (data + md)->split[axis];

        // swap median with end
        swap((data + md), (data + end - 1));

        for (p = store = start; p < end; ++p)
        {
            if ((data + p)->split[axis] < pivot)
            {
                if (p != store)
                    swap((data + p), (data + store));
                ++store;
            }
        }

        swap((data + store), (data + end - 1));

        if ((data + store)->split[axis] == (data + md)->split[axis])
            return md;

        if (store > md)
            end = store;
        else
            start = store;
    }
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

int growTreeSerial(kdnode_t *tree, int start, int end, int offset, int axis)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * growTree serial for when communicator is just
     * one process
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // when len of input data is 0 return 0
    if (!(end-start)) return -1;

    // else do the recursive procedure
    int md;
    if ((md = findMedian(tree, start, end, axis)) >= 0 ) 
    {
        (tree+md)->axis = axis;
        axis = (axis+1) % NDIM;

        (tree+md)->left = growTreeSerial(tree, start, md, offset, axis);
        (tree+md)->right = growTreeSerial(tree, md+1, end, offset, axis);
    }
    return md+offset;
}

int growTree(kdnode_t *tree, int start, int end, int offset, int axis, MPI_Comm comm)
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
        return growTreeSerial(tree, start, end, offset, axis);
    }

    // parallel case
    int md = 0, right_count = 0;
    int nleft = -1, nright = -1;
    MPI_Status status;

    if (comm_rank == 0)
    {
        md = findMedian(tree, start, end, axis);
        (tree + md)->axis = axis;
    }
    
    MPI_Bcast(&md, 1, MPI_INT, 0, comm);
    right_count = (end - start) - md -1;

    if (comm_rank == 0)
    {
        // find median and amount of data to send
        MPI_Send(tree+md+1, right_count, MPI_kdnode_t, 1, 10*comm_size, comm);
    }
    else if (comm_rank == 1)
    {
        // receive from root
        tree = (kdnode_t *)malloc(right_count*sizeof(kdnode_t));
        MPI_Recv(tree, right_count, MPI_kdnode_t, 0, 10*comm_size, comm, &status);
    }
    
    // update axis
    axis = (axis + 1) % NDIM;

    // split communicator 
    MPI_Comm new_comm;
    splitComms(comm, &new_comm);

    // call next step with new communicators
    if (comm_rank % 2 == 0)
    {
        nleft = growTree(tree, 0, md, offset, axis, new_comm);
    }
    else
    {
        nright = growTree(tree, 0, right_count,  offset+md+1, axis, new_comm);
    }

    // send points back to root
    if (comm_rank == 0)
    {   
        MPI_Recv(&nright, 1, MPI_INT, 1, 11*comm_size, comm, &status);
        (tree+md) -> left = nleft;
        (tree+md) -> right = nright;
        // receive reordered points
        MPI_Recv(tree+md+1, right_count, MPI_kdnode_t, 1, 13*comm_size, comm, &status);    
    }
    else if (comm_rank == 1)
    {
        MPI_Send(&nright, 1, MPI_INT, 0, 11*comm_size, comm);
        // send reordered points
        MPI_Send(tree, right_count, MPI_kdnode_t, 0, 13*comm_size, comm);
        free(tree);
    }

    return offset + md;
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
    kdnode_t *tree = parseFile(mpi_rank);

    // grow the tree and take time
    double time = MPI_Wtime();
    int root = growTree(tree, 0, NPTS, 0, 0, MPI_COMM_WORLD);
    time = MPI_Wtime() - time;

#ifndef NDEBUG
    // print tree for debug
    printTree(tree, mpi_rank);
#endif

    // average runtimes of different procs
    double avg_time;
    MPI_Reduce(&time, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (mpi_rank == 0)
    {
        // print information about the tree
        printf("Tree grown in %lfs\n", avg_time / mpi_size);
        printf("Tree root is at node %d\n\n", root);
    }

    MPI_Finalize();
    return 0;
}