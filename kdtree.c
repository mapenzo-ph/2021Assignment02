#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// compile with float or double (using -DDOUBLE_PRECISION)
#ifndef DOUBLE_PRECISION
#define float_t float
#else
#define float_t double
#endif

// hard code dimensionality of data
#define NDIM 2

/* =================== STRUCT DEFS =============================== */
typedef struct
{
    float_t data[NDIM];
} kdpoint;

typedef struct kdnode kdnode;
struct kdnode
{
    size_t axis;
    kdpoint *split;
    kdnode *left, *right;
};

/* =================== USER FUNCTIONS ============================ */
kdpoint *get_points(char *filename);

void sort_along(size_t axis, kdpoint *start, kdpoint *end);

kdnode *build_kdtree(kdpoint *points, size_t axis, size_t size);

/* =================== MAIN PROGRAM ============================== */
int main(int argc, char **argv)
{
    
    return 0;
}