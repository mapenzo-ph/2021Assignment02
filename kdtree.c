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
    size_t dim;
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
    * @ret pointer to an array addresses of datapoints
    */

    // safely open file
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        perror("Unable to open file. Aborting ...\n");
        exit(1);
    }
    
    // safely allocate memory
    kdpoint *points = (kdpoint*)malloc(size*sizeof(kdpoint));
    if (points == NULL)
    {
        perror("Allocation failed. Aborting ...\n");
        exit(1);
    }

    // build string to read datapoints
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
            r = fscanf(file, fmt, (points+count)->data + i);
            
        }
        ++count;
    }
    return points;
}

void print_head(kdpoint *points, size_t len)
{
    /*
    * Prints the first len entries of the dataset
    *
    * @param kdpoint *points: pointer to the dataset
    * @param size_t len: number of entries to be printed
    */

    // build string to print datapoint
#ifndef DOUBLE_PRECISION
    char fmt[7] = {'%', '5', '.', '3', 'f', ' ', '\0'};
    char fmtlast[7] = {'%', '5', '.', '3', 'f', '\n', '\0'};
#else
    char fmt[8] = {'%', '5', '.', '3', 'l', 'f', ' ', '\0'};
    char fmtlast[8] = {'%', '5', '.', '3', 'l', 'f', '\n', '\0'};
#endif

    // print point by point, coord by coord
    for (size_t i=0; i<len; ++i)
    {
        for (size_t coord=0; coord<NDIM-1; ++coord)
        {
            printf(fmt, (points+i)->data[coord]); 
        }
        printf(fmtlast, (points+i)->data[NDIM-1]);
    }
}

void swap(float_t *a, float_t *b)
{
    /*
    * Swaps two values a and b
    */

    float_t tmp = *a;
    *a = *b;
    *b = tmp;
}

void swap_points(kdpoint *a, kdpoint *b)
{
    /*
    * Swaps two vectors acting coordinate by coordinate
    */

    for (size_t i=0; i<NDIM; ++i)
    {
       swap(a->data+i, b->data+i);
    }
}

void partition(kdpoint *start, size_t len, size_t p, size_t axis)
{
    /*
    * Sorts elements along the chosen dimension, with respect to 
    * a pivotal element 
    * 
    * @params kdpoint *points: array of points to be sorted
    * @params size_t len: lenght of the array
    * @params size_t p: coordinate of the pivot
    * @params size_t dim: dimension alog which to sort
    */

    if (p > len-1)
    {
        printf("Pivot must be inside the range of points. Aborting...\n");
        exit(-1);
    }

    if (len <= 1) return; // single point case
    
    // take selected element as pivot and put in front
    float_t pivot = (start+p)->data[axis];
    printf("Pivot: %lf\n", pivot);

    kdpoint *left = start;
    kdpoint *right = start+len-1;
    
    while (left <= right)
    {
        while (right->data[axis] > pivot && left<=right)
        {
            printf("Moving right\n");
            --right;
        }
        while (left->data[axis] < pivot && left <= right)
        {
            printf("Moving left\n");
            ++left;
        }
        if (left->data[axis] >= right->data[axis] && left <= right)
        {
            printf("Swapping\n");
            swap_points(left, right);
            --right;
            ++left;
        }
    }
    return;
}


kdnode *build_kdtree(kdpoint* *points, size_t npoints, size_t dim, size_t ndim);

/* =================== MAIN PROGRAM ============================== */
int main(int argc, char **argv)
{
    // read data from file
    kdpoint *points = get_points(argv[1], 10, 0, ',');
    print_head(points, 10);
    printf("\n");
    swap_points(points, points+1);
    print_head(points, 10);
    printf("\n");
    partition(points, 10, 9, 1);
    print_head(points, 10);




    return 0;
}