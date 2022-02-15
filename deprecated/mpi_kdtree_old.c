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
void getProcOrder(int *order, int mpi_size)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Returns staggered proc order for distributing
     * extra points.
     * * * * * * * * * * * * * * * * * * * * * * * * */
    
    int previous[mpi_size/2];
    int halflen = mpi_size/2;
    int previous_len = 1;
    int start_idx, idx = 2;

    order[0] = 0;
    order[1] = halflen;
    previous[0] = halflen;

    while (idx < mpi_size)
    {
        // remember sarting index
        start_idx = idx;
        // update order list
        for (int i = 0; i < previous_len; ++i)
        {
            order[idx] = previous[i]/2;
            order[idx+1] = previous[i]/2 + halflen;
            idx = idx+2;
        }
        // update previous 
        previous_len *= 2;
        for (int i = 0; i < previous_len; ++i) 
        {
            previous[i] = order[start_idx+i];
        }
    }       
}

void parseLocalPoints(FILE * fp, int local_size, kdnode_t *buffer)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and stores data in buffer.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // set format for reading
    char FMTSEP[] = FMT;
    char sepstr[] = {SEP, '\0'};
    strncat(FMTSEP, sepstr, 1);

    int r;
    for (int np = 0; np < local_size; ++np)
    {
        for (int nc = 0; nc < NDIM - 1; ++nc)
        {
            r = fscanf(fp, FMTSEP, (buffer + np)->split + nc);
            if (r == EOF)
            {
                perror("Unexpected end of file while parsing. Exiting...");
                exit(2);
            }
        }
        r = fscanf(fp, FMTLAST, (buffer + np)->split + NDIM - 1);
        if (r == EOF)
        {
            perror("Unexpected end of file while parsing. Exiting...");
            exit(3);
        }
    }
}

kdnode_t *parseInputFile(int mpi_rank, int mpi_size, int *local_tree_size)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and distributes points to
     * each process.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // get local number of points
    int local_size = NPTS / mpi_size;
    int rest = NPTS % mpi_size;

    // order to distribute extra points
    int proc_order[mpi_size];
    getProcOrder(proc_order, mpi_size);
    for (int i = 0; i < rest; ++i)
    {
        if (mpi_rank == proc_order[i]) ++local_size;
    }

    // assign to output variable
    *local_tree_size = local_size;

    // allocate nodes
    kdnode_t *local_tree = (kdnode_t*)malloc(local_size*sizeof(kdnode_t));
    MPI_Status status;

    // use root process to send data to the others
    if (mpi_rank == 0)
    {
        FILE *fp = fopen(INFILE, "r");
        if (fp == NULL)
        {
            perror("Unable to open input file! Exiting...\n");
            exit(1);
        }

        // get points in root process
        printf("Local size: %d\n", local_size);
        parseLocalPoints(fp, local_size, local_tree);

        // get local size of dest process and transfer data
        int send_size;
        kdnode_t buffer[local_size];

        for (int dest = 1; dest < mpi_size; ++dest)
        {
            MPI_Recv(&send_size, 1, MPI_INT, dest, 
                     dest*10, MPI_COMM_WORLD, &status);

            printf("Send size %d: %d\n",dest, send_size);
            parseLocalPoints(fp, send_size, buffer);

            MPI_Send(buffer, send_size, MPI_kdnode_t, dest, 
                     dest*10, MPI_COMM_WORLD);
        }      
    }
    else
    {
        MPI_Send(&local_size, 1, MPI_INT, 0, 
                 mpi_rank*10, MPI_COMM_WORLD);

        MPI_Recv(local_tree, local_size, MPI_kdnode_t, 0, 
                 mpi_rank*10, MPI_COMM_WORLD, &status);
    }
    return local_tree;
}

int getLocalTreeOffset(int mpi_size, int mpi_rank, int local_tree_size)
{
    // gather all local sizes
    int local_sizes[mpi_size];
    MPI_Allgather(&local_tree_size, 1, MPI_INT, local_sizes, 1, MPI_INT, MPI_COMM_WORLD);
    int displacement = 0;
    for (int i = 0; i < mpi_rank; ++i)
    {
        displacement += local_sizes[i];
    }
    return displacement;
}

void printLocalTree(kdnode_t *local_tree, int local_tree_size, int displacement)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the three 
     * * * * * * * * * * * * * * * * * * * * * * * * */

    for (int np = 0; np < local_tree_size; ++np)
    {
        printf("idx: %3d | vals: ",displacement+np);
        for (size_t nc = 0; nc < NDIM; ++nc)
        {
            printf(PFMT, (local_tree + np)->split[nc]);
        }
        printf(" | ax: %2d | children: %3d , %3d\n", 
                (local_tree+np) -> axis, (local_tree+np)->left, (local_tree+np)->right);
    }
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

void swap(kdnode_t *a, kdnode_t *b)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Swaps memory position of two nodes
     * * * * * * * * * * * * * * * * * * * * * * * * */

    for (size_t i = 0; i < NDIM; ++i)
    {
        swapFloats(a->split + i, b->split + i);
    }
}

int partitionFloats(float_t *vals, int low, int high)
{
    if (low >= high) return low; // single node case}

    float_t pivot = vals[high];
    // printf("Pivot: %6.3lf\n", pivot);
    int i = low - 1;
    for (int j = low; j < high; ++j)
    {
        if (vals[j] <= pivot)
        {
            ++i;
            swapFloats(vals+i, vals+j);
        }
    }
    swapFloats(vals+i+1, vals+high);
    return i + 1;
}

int partition(kdnode_t *nodes, int low, int high, int axis)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Organizes nodes between @params start and
     * @params stop along @param axis in smaller
     * and larger than the pivot element (high).
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low >= high)
        return low; // single node case}

    float_t pivot = (nodes + high)->split[axis];
    // printf("Pivot: %6.3lf\n", pivot);

    int i = low - 1;

    for (int j = low; j < high; ++j)
    {
        if ((nodes + j)->split[axis] <= pivot)
        {
            ++i;
            swap(nodes + i, nodes + j);
        }
    }
    swap(nodes + i + 1, nodes + high);
    return i + 1;
}

void quicksortFloats(float_t *vals, int low, int high)
{
    
    if (low >= high) return; // single node case

    // partitioning points
    int pivot = partitionFloats(vals, low, high);

    // quicksort lower/upper sides
    quicksortFloats(vals, low, pivot - 1);
    quicksortFloats(vals, pivot, high);
}

void quicksort(kdnode_t *nodes, int low, int high, int axis)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Quicksorts nodes in the input array.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low >= high) return; // single node case

    // partitioning points
    int pivot = partition(nodes, low, high, axis);
    
    // quicksort lower/upper sides
    quicksort(nodes, low, pivot-1, axis);
    quicksort(nodes, pivot, high, axis);
}

float_t getCommMedian(kdnode_t  *local_tree, int local_tree_size, int axis, int comm_size)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Returns global median of elements in comm
     * * * * * * * * * * * * * * * * * * * * * * * * */
    
    // find local medians
    int local_median_idx = local_tree_size / 2;
    float_t local_median = (local_tree + local_median_idx)->split[axis];

    // gather local medians in each proc 
    float_t local_medians[comm_size];
    MPI_Allgather(&local_median, 1, MPI_FLOAT_T,
        local_medians, 1, MPI_FLOAT_T, comm);

    // sort and return median of medians
    quicksortFloats(local_medians, 0, comm_size-1);
    int global_median_idx = comm_size / 2;  
    return local_medians[global_median_idx];
}

int getLowerElems(float_t global_median, kdnode_t *local_tree_size, int axis)
{
    int n_lower = 0;
    while (local_tree[n_lower]->split[axis] < global_median)
    {
        ++n_lower;
    }
    return n_lower;
}

int getPowerOfTwo(int comm_size)
{
    int div = comm_size;
    int pow = 0;
    while (div > 1)
    {
        div = div / 2;
        ++pow;
    }
    return pow;
}

void pointRepartition(
    kdnode_t *local_tree, int n_lower, int n_upper, int comm_size, kdnode_t *recv_buffer)
{
    
}

void growTree(
    kdnode_t *local_tree, int local_tree_size, int local_tree_offset, int axis, MPI_Comm comm, int *idx)
{
    // get comm size and rank
    int comm_size, comm_rank;
    MPI_Comm_size(comm, &comm_size);
    MPI_Comm_rank(comm, &comm_rank);

    // get global median 
    float_t comm_median = getCommMedian(local_median, comm_size);      
    
    // get number of elements smaller/bigger than global median on each proc
    int n_lower = getLowerElems(global_median, local_tree, axis);
    int n_upper = local_tree_size - n_lower;
    int n_los[comm_size], n_ups[comm_size];
    MPI_Allgather(&n_lower, 1, MPI_INT, n_los, 1, MPI_INT, comm);
    MPI_Allgather(&n_upper, 1, MPI_INT, n_ups, 1, MPI_INT, comm);

    // allocate receive buffer and send/recv points
    kdnode_t recv_buffer[local_tree_size];
    int dest = comm_size - comm_rank;
    MPI_status status;
    if (rank < comm_size/2)
    {
        MPI_Send(local_tree+n_lower, n_upper, MPI_kdnode_t, dest, comm_rank*10, comm);
        MPI_Recv(recv_buffer, n_ups[comm_size - comm_rank], MPI_kdnode_t, dest);
    }

    // split communicators and rebalance inside
    int color, key;
    
    {
        color = 1;
        key = old_rank - comm_size / 2;
    }
    else
    {
        color = 0;
        key = old_rank;
    }
    MPI_Comm left_comm, right_comm;
    MPI_Comm_split(comm, color, key, &right_comm);
    left_comm = comm;
}
// ========================================================
//                      Main program
// ========================================================
int main(int argc, char **argv)
{
    // init and get parameters
    MPI_Init(&argc, &argv);

    // define custom MPI datatype for passing kdnode
    int blocklen[3] = {1, NDIM, 2};
    MPI_Aint disp[3] = {0, sizeof(int), sizeof(int)+NDIM*sizeof(float_t)};
    MPI_Datatype oldtypes[3] = {MPI_INT, MPI_FLOAT_T, MPI_INT};
    MPI_Type_create_struct(
        3, blocklen, disp, oldtypes, &MPI_kdnode_t);
    MPI_Type_commit(&MPI_kdnode_t);

    // parse input file and distribute data
    int mpi_size, mpi_rank, local_tree_size;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    kdnode_t *local_tree = parseInputFile(mpi_rank, mpi_size, &local_tree_size);

    quicksort(local_tree, 0, local_tree_size-1, 0);

#ifndef NDEBUG
    // print each node consequently (for debug)
    int local_tree_offset = getLocalTreeOffset(mpi_size, mpi_rank, local_tree_size);
    for (int i = 0; i < mpi_size; ++i)
    {
        if (mpi_rank == i)
        {
            printf("\n\nNodes on proc %d:\n", mpi_rank);
            printLocalTree(local_tree, local_tree_size, local_tree_offset);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
#endif

    MPI_Type_free(&MPI_kdnode_t);
    MPI_Finalize();
    return 0;
}