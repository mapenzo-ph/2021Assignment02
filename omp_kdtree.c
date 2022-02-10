#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// ==================================================================
//                          Set precision
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
//              Global vars and struct definitions
// ==================================================================
#define INFILE "test_data.csv"
#define NPTS 50
#define NDIM 2
#define SEP ','

int omp_size;
int omp_rank;

typedef struct kdnode kdnode_t;
struct kdnode
{
    int axis;
    float_t split[NDIM];
    int left, right;
};

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
    kdnode_t *tree = (kdnode_t*)malloc(NPTS*sizeof(kdnode_t));

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
            r = fscanf(fp, FMTSEP, (tree+np)->split+nc);
            if (r == EOF)
            {
                perror("Unexpected end of file while parsing. Exiting...");
                exit(2);
            }
        }
        r = fscanf(fp, FMTLAST, (tree + np)->split+NDIM-1);
        if (r == EOF)
        {
            perror("Unexpected end of file while parsing. Exiting...");
            exit(3);
        }
    }
    rewind(fp);
    return tree;
}

void printHead(kdnode_t *tree, int len)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the first @param len entries in the
     * tree.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // print point by point, coord by coord
    for (size_t np = 0; np < len; ++np)
    {
        for (size_t nc = 0; nc < NDIM - 1; ++nc)
        {
            printf(PFMT, (tree+np)->split[nc]);
        }
        printf(PFMTLAST, (tree+np)->split[NDIM-1]);
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

void swapNodes(kdnode_t *a, kdnode_t *b)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Swaps memory position of two nodes
     * * * * * * * * * * * * * * * * * * * * * * * * */

    for (size_t i=0; i < NDIM; ++i)
    {
       swapFloats(a->split+i, b->split+i);
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
     * Performs a paralle quicksort using openMP tasks.
     * Assumes a parallel region has been opened.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low >= high) return; // single node case

    // partitioning points
    int pivot = partition(nodes, low, high, axis);
    
    // task to quicksort lower/upper sides
    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            {
                quicksort(nodes, low, pivot-1, axis);
            }

            #pragma omp task
            {
                quicksort(nodes, pivot, high, axis);
            }
        }
    }
}

void growTree(kdnode_t *nodes, int low, int high, int axis, int *idx)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Grows a tree starting from an unordered list
     * of nodes. Assumes a parallel region is already
     * open to spawn tasks.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    if (low == high) // leaf handling
    {
        (nodes+low) -> axis = axis;
        (nodes+low) -> left = -1;
        (nodes+low) -> right = -1;
        *idx=low;
        return;
    }

    if (low > high)
    {
        *idx=-1;
        return;
    }

    // quicksort the nodes and set median
    quicksort(nodes, low, high, axis);
    
    int median = (high + low) / 2;
    int new_axis = (axis + 1) % NDIM;
    (nodes+median) -> axis = axis;
    *idx = median;

    #pragma omp parallel
    {
        #pragma omp single
        {
            #pragma omp task
            {
                growTree(nodes, low, median-1, new_axis, &((nodes+median)->left));
            }

            #pragma omp task
            {
                growTree(nodes, median+1, high, new_axis, &((nodes+median)->right));
            }
        }
    }
    return;
}

void printTree(kdnode_t *tree)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the three 
     * * * * * * * * * * * * * * * * * * * * * * * * */

    for (int np = 0; np < NPTS; ++np)
    {
        printf("idx: %3d | vals: ",np);
        for (size_t nc = 0; nc < NDIM; ++nc)
        {
            printf(PFMT, (tree + np)->split[nc]);
        }
        printf(" | ax: %2d | children: %3d , %3d\n", 
                (tree+np) -> axis, (tree+np)->left, (tree+np)->right);
    }
}

// ==================================================================
//                          MAIN PROGRAM
// ==================================================================
int main(int argc, char **argv)
{
    // open file and read parameters and points
    kdnode_t *nodes = parseInputFile();

    int root;
    double timer = omp_get_wtime();
    growTree(nodes, 0, NPTS-1, 0, &root);
    timer = omp_get_wtime() - timer;
    printTree(nodes);
    printf("\n\nTree grown in %lfs!\n", timer);
    printf("Tree root is located at node %d\n", root);
    return 0;
}