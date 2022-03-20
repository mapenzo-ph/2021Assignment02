#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

// ==================================================================
//                      Input parameters
// ==================================================================
#define DATA "../test_data_e09.csv"
#define NDIM 2
#define SEP ','

// ==================================================================
//                      Set precision
// ==================================================================
#ifndef DOUBLE_PRECISION
#define float_t float
#define FMT "%f"
#define FMTLAST "%f\n"
#define PFMT "%6.3f "
#define PFMTLAST "%6.3f\n"
#else
#define float_t double
#define FMT "%lf"
#define FMTLAST "%lf\n"
#define PFMT "%6.3lf "
#define PFMTLAST "%6.3lf\n"
#endif

// ==================================================================
//                      Struct definition
// ==================================================================
typedef struct kdnode kdnode_t;
struct kdnode {
    float_t split[NDIM];
    int axis, left, right;
};

// ==================================================================
//                      User functions
// ==================================================================
#if defined(DATA)
kdnode_t *parseFile()
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Parses input file and stores points into an
     * array. Returns the array.
     * * * * * * * * * * * * * * * * * * * * * * * * */

    // allocate nodes
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
    rewind(fp);
    return tree;
}
#endif

#ifndef NDEBUG
void printTree(kdnode_t *tree)
{
    /* * * * * * * * * * * * * * * * * * * * * * * * *
     * Prints the three
     * * * * * * * * * * * * * * * * * * * * * * * * */

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
#endif

inline void swap(kdnode_t *a, kdnode_t *b)
{
    float_t tmp[NDIM];
    memcpy(tmp, a->split, sizeof(tmp));
    memcpy(a->split, b->split, sizeof(tmp));
    memcpy(b->split, tmp, sizeof(tmp));
}

int find_median(kdnode_t *data, int start, int end, int axis)
{
    if (end <= start) return -1;
    if (end == start + 1) return start;

    int p, store;
    int md = start + (end - start)/2;
    double pivot;

    while(1)
    {
        // take median as pivot 
        pivot = (data+md)->split[axis];

        // swap median with end 
        swap((data+md), (data+end-1));

        for (p = store = start; p < end; ++p)
        {
            if ((data+p)->split[axis] < pivot)
            {
                if (p != store) swap((data+p), (data+store));
                ++store;
            }
        }

        swap((data+store), (data+end - 1));

        if ((data+store)->split[axis] == (data+md)->split[axis])
            return md;

        if (store > md) end = store;
        else start = store;
    }
}

int growTree(kdnode_t *tree, int start, int end, int axis)
{
    // when len of input data is 0 return 0
    if (!(end-start)) return -1;

    // else do the recursive procedure
    int n;
    if ((n = find_median(tree, start, end, axis)) >= 0 ) 
    {
        (tree+n)->axis = axis;
        axis = (axis+1) % NDIM;
        (tree+n)->left = growTree(tree, start, n, axis);
        (tree+n)->right = growTree(tree, n+1, end, axis);
    }
    return n;
}

// ==================================================================
//                      Main program
// ==================================================================
int main(int argc, char **argv)
{

    // either read file or generate data
    kdnode_t *tree = parseFile();


    int root;
    double time = omp_get_wtime();
    // build tree
    root = growTree(tree, 0, NPTS, 0);
    time = omp_get_wtime() - time;

#ifndef NDEBUG
    // print for debugging
    printTree(tree);
    printf("\n\n");
#endif

    // print results
    printf("Tree grown in %lfs\n", time);
    printf("Tree root is at node %d\n", root);

    free(tree);
    return 0;
}