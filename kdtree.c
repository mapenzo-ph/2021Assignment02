#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

// compile with float or double (using -DDOUBLE_PRECISION)
#ifndef DOUBLE_PRECISION
#define float_t float
#else
#define float_t double
#endif

// hard code dimensionality of data
#define NDIM 3

// define useful datastructures
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

// =================== USER FUNCTIONS ===============================
kdpoint *get_points(
    char *filepath, size_t size, 
    const size_t headlen, char sep
) {
    /*
    * Function that reads dataset and imports points
    *
    * @param char* string   path of the dataset file
    * @param size_t size    number of dataset entries
    * @param size_t headlen number of header lines to be skipped
    * @param char sep       separator used in the fil
    * 
    * @ret pointer to an array of datapoints
    */

    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("Unable to open file. Aborting ...\n");
        exit(1);
    }

    kdpoint *points = (kdpoint *)malloc(size * sizeof(kdpoint));
    if (points == NULL)
    {
        perror("Allocation failed. Aborting ...\n");
        exit(1);
    }

    // build string to read in datapoints
    #ifndef DOUBLE_PRECISION
    char fmt[4] = {'%', 'f', sep, '\0'};
    char fmtlast[4] = {'%', 'f', '\n', '\0'};
    #else
        char fmt[5] = {'%', 'l', 'f', sep, '\0'};
        char fmtlast[5] = {'%', 'l', 'f', '\n', '\0'};
    #endif

    size_t count = 0;   
    int r;

    while (r != EOF && count < size)
    {
        for (size_t i = 0; i < NDIM; ++i)
        {
            r = fscanf(file, fmt, (points + count)->data + i);
        }
        ++count;
    }

    return points;
}

void sort_along(size_t axis, kdpoint *start, kdpoint *end);

kdnode *build_kdtree(kdpoint *points, size_t axis, size_t size);

/* =================== MAIN PROGRAM ============================== */
int main(int argc, char **argv)
{

    kdpoint *points = get_points("test.csv", 2, 0, ',');
    printf("%f %f %f\n", points->data[0], points->data[1], points->data[2]);
    printf("%f %f %f\n", (points+1)->data[0], (points+1)->data[1], (points+1)->data[2]);
    return 0;
}