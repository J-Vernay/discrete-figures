/// \file MartinAlgoOpti.hpp Optimized version of the Martin algorithm,
/// with optimizations due to better memory-locality.

#include <utility>
#include <array>
#include <cstring>
#include <vector>
#include <cstdio>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <chrono>
#include <numeric>

/// The grid has three purposes:
/// 1. When adding candidates, it is used to efficiently check if the candidate has already been visited.
/// 2. When using white-connexity, check if the figure breaks white-connexity or not.
/// 3. When calculating graph density, find the edges between neighbour chosen cells.
/// For (1), it is only necessary to store whether a cell is Candidate or not.
/// For (2) and (3), it is also necessary to keep wich cells are Chosen.
/// (2) and (3) incurs a noticeable cost (almost doubling the time taken when only counting),
/// so they can be disabled by passing "GridBehaviour::Minimal".
/// The default behaviour is "GridBehaviour::Accurate", which update which cells are chosen. 
enum class GridBehaviour { Minimal, Accurate };

/// The optimized implementation of Martin algorithm.
/// The first optimization is to use fixed-size containers to prevent allocations.
/// This require to size them in accordance to a maximal figure size.
/// The second optimization is to pass configuration infos in template parameters,
/// so the compiler can remove at build-time unused condition branches.
/// \tparam N_ The max size of a figure.
/// \tparam B_ The black-connexity, either 4 or 8.
/// \tparam W_ The white-connexity, either 4 or 8, or 0 to disable it.
/// \tparam grid_behaviour How the grid should be updated? See GridBehaviour for detials
template<int N_, int B_, int W_, GridBehaviour grid_behaviour = GridBehaviour::Accurate>
struct MartinAlgoOpti {
    static constexpr int N = N_, B = B_, W = W_;
    
    static_assert(B == 4 || B == 8, "Black connexity is either 4 or 8.");
    static_assert(W == 0 || W == 4 || W == 8, "White connexity is either 0, 4, or 8.");
    static_assert(W == 0 || grid_behaviour == GridBehaviour::Accurate,
        "White-connexity will not work without GridBehaviour::Accurate");

    static_assert(!(B == 8 && W == 8), "Connexity (8,8) not supported yet.");

    /// We use a grid to have direct checks when adding candidates and for white-connexity
    /// (see GridBehaviour for details). The grid is preallocated so it can store any figure
    /// of size <= N. We chose WIDTH = 2*N+1, which, with origin being X = N, provides N cells
    /// in both directions, ensuring borders are never reached when considering neighbours.
    /// HEIGHT = N+2, with origin being Y = 1, because due to the origin uniqueness, the figure
    /// cannot grow towards negative Y. Then we have N remaining cells in positive Y direction.
    static constexpr unsigned WIDTH = 2*N+1, HEIGHT = N+2;
    static constexpr unsigned STARTING_POINT = N + WIDTH; ///< (X = N, Y = 1)

    /// "Chosen" is only used with "GridBehaviour::Accurate".
    enum Cell { Unvisited = 0, Candidate = 1, Chosen = 2 };

    /// The actual grid. For a point whose coordinate is (x,y),
    /// its position in the array is grid[x + y * WIDTH].
    std::array<Cell, WIDTH*HEIGHT> grid;

    /// Stores the position of the candidates. Candidate cells are either chosen (at most N),
    /// or neighbours of chosen cells (at most 4*(N+1) when black-connexity is 8).
    std::array<unsigned, N + 4*(N+1)> candidates;
    /// Which level are we, which goes from 0 to N-1 (respecting C++ 0-indexed convention).
    /// At any moment, there are "level + 1" chosen cells.
    unsigned level;
    /// The index in "candidates" array of the chosen cells, up to chosen[level].
    std::array<unsigned, N> chosen;
    /// The index in "candidates" array which, when reached by "chosen", provokes a Pop().
    /// Basically, chosen_last[level] provides "past-the-end" index for chosen[level].
    std::array<unsigned, N> chosen_last;

    /// Initializes the Martin algorithm by preparing the iteration of all figures of size N.
    /// After Init(), this structure stores the initial figure of size 1.
    void Init() {
        std::fill(grid.begin(), grid.begin() + STARTING_POINT, Cell::Candidate);
        grid[STARTING_POINT] = Cell::Chosen;
        std::fill(grid.begin() + STARTING_POINT + 1, grid.end(), Cell::Unvisited);
        
        candidates[0] = STARTING_POINT;
        level = 0;
        chosen[level] = 0;
        chosen_last[level] = 1;
    }

    /// Adds a candidate to be explored for the next level, or skip it if already candidate.
    /// This cannot be used when level == N, because figures must not grow at this point.
    void AddCandidate(unsigned pos) {
        if (grid[pos] != Cell::Unvisited)
            return; // Already candidate or chosen
        grid[pos] = Cell::Candidate;
        candidates[chosen_last[level+1]++] = pos;
    }

    /// Adds all neighbours of "center" as candidates for the next level.
    /// This cannot be used when level == N, because figures must not grow at this point.
    void AddCandidates(unsigned center) {
        chosen_last[level+1] = chosen_last[level];
        if constexpr (B == 4) {
            AddCandidate(center + 1);
            AddCandidate(center + WIDTH);
            AddCandidate(center - 1);
            AddCandidate(center - WIDTH);
        }
        else { // B == 8
            AddCandidate(center + 1);
            AddCandidate(center + 1 + WIDTH);
            AddCandidate(center     + WIDTH);
            AddCandidate(center - 1 + WIDTH);
            AddCandidate(center - 1);
            AddCandidate(center - 1 - WIDTH);
            AddCandidate(center - WIDTH);
            AddCandidate(center + 1 - WIDTH);
        }
    }

    /// Adds the next candidate to the figure as a chosen cell.
    void Push() {
        // This check is required in case AddCandidates() has provided no candidates.
        if (chosen_last[level+1] == chosen[level])
            return;
        ++level;
        chosen[level] = chosen[level-1] + 1;
    }

    /// Removes the last chosen cell from the figure.
    void Pop() {
        --level;
        // Unvisit points which have been visited due to this cell.
        for (unsigned i = chosen_last[level]; i < chosen_last[level+1]; ++i)
            grid[candidates[i]] = Cell::Unvisited;
        // Unchose the removed cell.
        if constexpr (grid_behaviour == GridBehaviour::Accurate)
            grid[candidates[chosen[level]]] = Cell::Candidate;
    }

    /// Checks whether the last chosen cell has broken the white-connexity or not.
    bool IsValid() const {
        if constexpr (W == 0)
            return true; // Always valid if white-connexity is not considered.
        unsigned pos = candidates[chosen[level]];
        if constexpr (!(B == 8 && W == 8)) {
            // For non-(8,8)-connexity, we can use the local method O(1) to check if
            // the current figure is valid, given the level-1 figure was valid too.
            // We need to know whether the neighbours are chosen or not:
            // a b c
            // d   f  (the center is 'pos')
            // g h i
            bool a = grid[pos - 1 + WIDTH] == Cell::Chosen, b = grid[pos     + WIDTH] == Cell::Chosen,
                 c = grid[pos + 1 + WIDTH] == Cell::Chosen, d = grid[pos - 1        ] == Cell::Chosen,
                 f = grid[pos + 1        ] == Cell::Chosen, g = grid[pos - 1 - WIDTH] == Cell::Chosen,
                 h = grid[pos     - WIDTH] == Cell::Chosen, i = grid[pos + 1 - WIDTH] == Cell::Chosen;
            if constexpr (W == 4) {
                // For each pair (F,C), (C,B), ..., (I,F), we check if it is equal to (Chosen, NotChosen).
                // By counting the number of such pairs, we know the number of white parts after the insertion
                // of 'c'. If it is 0 or 1, there are no white breakage. Else, we would have multiple parts.
                // There is a special case for the corners A, C, G, I:
                // if A is white and B and D are chosen, it means A is already connected "outside" the local context.
                // In this case, we are not breaking "more" the local connexity, so such cases should be removed.
                int count =
                    (f && !c) + (c && !b) + (b && !a) + (a && !d) + (d && !g) + (g && !h) + (h && !i) + (i && !f)
                    - (!a && b && d) - (!c && b && f) - (!g && d && h) - (!i && f && h);
                return count < 2;
            }
            else { // W == 8
                // This is the same principle as WouldBreakWhiteLocal4(), 
                // except the handling of corners: the special case "!A && B && D" is not possible,
                // since the corners are 8-connected to the center.
                // However, the case "A && !B && !D" should not count, as B and D are still connected.
                int count =
                    (f && !c) + (c && !b) + (b && !a) + (a && !d) + (d && !g) + (g && !h) + (h && !i) + (i && !f)
                    - (a && !b && !d) - (c && !b && !f) - (g && !d && !h) - (i && !f && !h);  
                return count < 2;
            }
        }
        else {
            // (8,8)-connexity, not implemented yet...
            return true;
        }
    }
    
    /// Advances the algorithm to the next figure. Figures of size < N are also outputted.
    /// \tparam MaxSize It is possible to iterate only on smaller figures too.
    template<int MaxSize = N>
    void NextStep() {
        if (level == MaxSize-1) {
            if constexpr (grid_behaviour == GridBehaviour::Accurate)
                grid[candidates[chosen[level]]] = Cell::Candidate;
            ++chosen[level];
        }
        else {
            AddCandidates(candidates[chosen[level]]);
            Push();
        }
        for (;;) {
            while (chosen[level] >= chosen_last[level]) {
                if (level == 0)
                    return;
                Pop();
                ++chosen[level];
            }
            if (IsValid()) {
                if constexpr (grid_behaviour == GridBehaviour::Accurate)
                    grid[candidates[chosen[level]]] = Cell::Chosen;
                return;
            }
            ++chosen[level];
        }
    }

    /// Calculates the graph density, which is:
    /// (2 * nb_undirected_edges) / (nb_vertices * (nb_vertices - 1))
    double Density() const {
        if constexpr (grid_behaviour == GridBehaviour::Minimal)
            throw std::logic_error("Density() requires GridBehaviour::Accurate.");
        
        unsigned nb_vertices = level+1;
        unsigned nb_edges = 0;
        for (int j = 0; j <= level; ++j) {
            unsigned pos = candidates[chosen[j]];
            bool a = grid[pos - 1 + WIDTH] == Cell::Chosen, b = grid[pos     + WIDTH] == Cell::Chosen,
                 c = grid[pos + 1 + WIDTH] == Cell::Chosen, d = grid[pos - 1        ] == Cell::Chosen,
                 f = grid[pos + 1        ] == Cell::Chosen, g = grid[pos - 1 - WIDTH] == Cell::Chosen,
                 h = grid[pos     - WIDTH] == Cell::Chosen, i = grid[pos + 1 - WIDTH] == Cell::Chosen;
            if constexpr (B == 4) {
                nb_edges += b + d + f + h;
            }
            else { // B == 8
                nb_edges += a + b + c + d + f + g + h + i;
            }
        }
        // Note: 'nb_edges' contains the number of directed edges:
        // Both edges (A->B) and (B->A) are counted, meaning we already have 2 * undirected edges.
        // The density is then (directed edges) / (nb_vertices * (nb_vertices - 1))
        return (double)(nb_edges) / (nb_vertices * (nb_vertices - 1));
    }

    /// Overrides "repr" with the figure string representation:
    /// 'X' for chosen cells and ' ' for others, '\n' for line delimiters.
    /// The string is null-terminated. "repr" must have at least ReprSize bytes.
    static constexpr unsigned ReprSize = (WIDTH + 1) * HEIGHT + 1;
    void GetRepr(char const* msg, char* repr) const {
        int i = 0;
        for (int y = HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < WIDTH; ++x)
                repr[i++] = grid[x + y * WIDTH] == Cell::Chosen ? 'X' : ' ';
            repr[i++] = '\n';
        }
        repr[i] = '\0';
    }
};