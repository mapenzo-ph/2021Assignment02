#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <vector>
#include <chrono>

#ifdef _OPENMP
#include <omp.h>
#endif

// ==================================================================
// kdtree class
// ==================================================================
template <typename T, std::size_t NDIM>
class kdtree
{
    using value_type = T;
    using kdpoint = std::array<value_type, NDIM>;

    // basic struct for a node
    struct kdnode
    {
        int ax;
        kdpoint split;
        std::size_t left, right;

        // overload operator to print node
        friend std::ostream& operator<<(std::ostream& os, const kdnode& node)
        {
            os.precision(3);
            os << "axis: " << node.ax << ", split point: [ ";
            for (auto x: node.split)
                os << std::setw(10) << std::fixed << x << " ";
            os << "], children: " << node.left << " " << node.right;
            return os; 
        }

        // define constructor
        explicit kdnode(const kdpoint& point) :
            ax{},
            split{point},
            left{},
            right{}
        {
            // std::cout << "Using node copy ctor" << std::endl;
        }

        explicit kdnode(kdpoint&& point) :
            ax{},
            split{std::forward<kdpoint>(point)},
            left{},
            right{}
        {
            // std::cout << "Using node move ctor" << std::endl;
        }

    };

    // vector containing all nodes
    std::vector<kdnode> tree;

    // ==============================================================
    // private helper functions
    // ==============================================================
    void swap_points(size_t i, size_t j)
    {
        /*
         * Swaps the split point of nodes i,j
         */

        tree.at(i).split.swap(tree.at(j).split);
    }

public:

    explicit kdtree() :
        tree{}
    {}

    void read_file(std::string filename, char sep)
    {
        /*
        * Imports points in the tree vector
        *
        * @param std::string filename: path of the dataset file
        * @param char sep: separator for dataset entries
        */

        using std::chrono::duration;
        using std::chrono::high_resolution_clock;

        std::ifstream infile(filename, std::ios::in);
        // check file opened correctly
        if (!infile.is_open())
        {
            std::cerr << "Problem opening the input file!\n" << std::endl;
            exit(-1);
        }

        kdpoint point;
        size_t idx;
        std::string line;
        std::string cell;
        std::istringstream lstream;
        std::istringstream cstream;

        auto start = high_resolution_clock::now();
        while (std::getline(infile, line))
        {
            lstream = std::istringstream(line);
            idx = 0;
            while((std::getline(lstream, cell, sep)))
            {
                cstream = std::istringstream(cell);
                cstream >> point.at(idx);
                ++idx;
            }
            tree.push_back(kdnode(std::move(point)));
        }
        auto end = high_resolution_clock::now();
        duration<double, std::milli> ms_double = end - start;
        std::cout << "Read " << tree.size() << " points in "
                  << ms_double.count() << "ms." << std::endl; 
    }

    void print(size_t start, size_t stop) noexcept
    {
        /*
         * Prints all the nodes between start and stop (excluded)
         */

        for (size_t i = start; i < stop; ++i)
        {
            std::cout << tree.at(i) << std::endl;
        }
    }
  
    value_type get_value(size_t node, size_t axis)
    {
        /*
        * Returns the value of @param node at coord 
        * @param axis
        */
        return tree.at(node).split.at(axis);
    }

    size_t size() const noexcept 
    {
        return tree.size();
    }

    size_t partition(size_t start, size_t len, size_t axis) noexcept
    {
        /*
        * Takes the @param len points beginning from @param start
        * and performs a partial ordering along @param axis with 
        * respect to the value at @param pivot.
        * 
        * Returns the pivot element splitting 
        */

        if (start >= tree.size() || start+len > tree.size())
        {
            std::cerr << "Parameter out of range!\n" << std::endl;
            exit(-1);
        }

        if (len <= 1) return start; // single node case

        long int i = start-1;
        size_t end = start + len-1;

        value_type pivot = get_value(end, axis);
        // std::cout << "Pivot: " << pivot << std::endl;
        
        for (size_t j = start; j <= end; ++j)
        {
            if (get_value(j, axis) < pivot)
            {
                ++i;
                swap_points(i, j);
            }
        }
        swap_points(i+1, end);
        return i+1;
    }

    void omp_qsort(size_t start, size_t len, size_t axis)
    {
        /*
        * Performs a quicksort along @param axis on the @param points 
        * starting from index @param start
        */
        if (axis >= NDIM)
        {
            std::cerr << "Axis is out of range.\n";
            exit(-1);
        }

        if (len <= 1 ) return; // single node case
    
        size_t pivot = partition(start, len, axis);
        size_t len_lower = pivot-start;
        size_t len_upper = len - len_lower - 1;

        #pragma omp task
        omp_qsort(start, len_lower, axis);
        
        #pragma omp task
        omp_qsort(pivot+1, len_upper, axis); 
    }

    void qsort(size_t start, size_t len, size_t axis)
    {
        /*
        * Wrapper for omp_qsort, opens parallel region
        * then runs omp version
        */
        #pragma omp parallel shared(start)
        {
            #pragma omp task
            omp_qsort(start, len, axis);
        }
    }

    size_t omp_build_subtree(size_t start, size_t len, size_t axis)
    {
        /*
        * Builds subtree using the quicksort and openMP tasks
        * (to be used inside a parallel region)
        */
    
        if (len <= 1){
            tree.at(start).ax = axis;
            tree.at(start).left = nullptr;
            tree.at(start).right = nullptr;
            return start;
        }

        size_t median = len / 2;
        size_t len_left = median-start;
        size_t len_right = len - len_left - 1;
        size_t new_axis = (axis + 1) % NDIM;

        // sort the relevant portion of the array
        omp_qsort(start, len, axis);
        // populate the median node
        tree.at(median).ax = axis;
        
        #pragma omp task
        tree.at(median).left = omp_build_subtree(start, len_left, new_axis);

        #pragma omp task
        tree.at(median).right = omp_build_subtree(median+1, len_right, new_axis);

        // return the relevant node
        return median;
    }

    size_t build_subtree(size_t start, size_t len, size_t axis)
    {
        /*
        * Wrapper for omp_build_subtree, opens parallel region, 
        * then runs omp version
        */
        #pragma omp parallel shared(start)
        {
            omp_qsort(start, len, axis);
        }
    }

};


// ==================================================================
// Main program
// ==================================================================
int main(int argc, char **argv)
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration;

    kdtree<double, 3> mytree;
    mytree.read_file("test_data.csv", ',');
    mytree.print(0, mytree.size());
    
    std::cout << "" << std::endl;
    auto t1 = high_resolution_clock::now();
    mytree.omp_qsort(0, mytree.size(), 0);
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2-t1;
    std::cout << "Ordered in " << ms_double.count() << "ms" << std::endl;
    mytree.print(0,mytree.size());


    return 0;
}