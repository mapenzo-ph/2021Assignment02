
#include <iostream>
#include <array>
#include <vector>
// #include <mpi.h>

template <typename T, std::size_t NDIM>
class kdtree // kdtree node class
{
    // basic type aliases
    using value_t = T;
    using point_t = std::array<value_t, NDIM>;

    struct node_t // basic node structure
    {
        std::size_t axis;       // axis for split
        point_t split;          // splitting point
        node_t *left, *right;   // pointers to children
    
        node_t() noexcept = default;
        node_t(const std::size_t& ax, point_t&& p, node_t* l, node_t* r) noexcept :
            axis{ax},
            split{std::move(p)},
            left{l},
            right{r}
        {}
    };

    std::vector<node_t> tree;   // tree with all unordered nodes
    
public: // interface functions

    
};

// ===== MAIN PROGRAM ===============================================
int main(int argc, char** argv)
{
    //MPI_Init(&argc, &argv);


    //MPI_Finalize();
    return 0;
}



