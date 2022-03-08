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

    //static_assert(!(B == 8 && W == 8), "Connexity (8,8) not supported yet.");

    /// The grid requires some margin: for (4,4), (4,8) and (8,4), we require
    /// an extra margin so that for each chosen point, we can inspect its 8 neighbours.
    /// For (8,8), we require a double margin so we can inspect the neighbours of
    /// the white neighbours of the figure.
    static constexpr unsigned MARGIN =
        (B == 8 && W == 8) ? 2 : (W != 0) ? 1 : 0;
    /// We use a grid to have direct checks when adding candidates and for white-connexity
    /// (see GridBehaviour for details). The grid is preallocated so it can store any figure
    /// of size <= N. We chose WIDTH = 2*N-1, which, provides N-1 cells in both direction,
    /// plus the extra margin required to prevent out-of-bounds when accessing neighbours.
    /// HEIGHT = N+1, with origin being Y = 1, because due to the origin uniqueness, the figure
    /// cannot grow towards negative Y. Then we have N-1 remaining cells in positive Y direction.
    static constexpr unsigned WIDTH  = 2 * N - 1 + 2 * MARGIN;
    static constexpr unsigned HEIGHT = N + 1 + MARGIN;
    static constexpr unsigned STARTING_POINT = WIDTH + WIDTH / 2; ///< (X = N, Y = 1) (+ margin)

    /// "Chosen" is only used with "GridBehaviour::Accurate".
    enum Cell { Unvisited = 0, Candidate = 1, Chosen = 2, INTERNAL = 3 /**< Only used internally */ };

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
    /// The logical size of given "level" corresponds to the actual candidates from this level
    /// are candidates[0 <= i < logical_sizes[level]]. This means that we must have
    /// chosen[level] < logical_sizes[level], else we would choose a candidate which has
    /// not be visited at this level. So instead, when chosen[level] would reach logical_sizes[level],
    /// a Pop() is required.
    std::array<unsigned, N> logical_sizes;

    /// Initializes the Martin algorithm by preparing the iteration of all figures of size N.
    /// After Init(), this structure stores the initial figure of size 1.
    void Init() {
        // Make sure the first cells are not used.
        std::fill(grid.begin(), grid.begin() + STARTING_POINT, Cell::INTERNAL);
        grid[STARTING_POINT] = Cell::Chosen;
        std::fill(grid.begin() + STARTING_POINT + 1, grid.end(), Cell::Unvisited);
        
        // Simulating an initial push.
        candidates[0] = STARTING_POINT;
        level = 0;
        chosen[level] = 0;
        logical_sizes[level] = 1;
    }

    /// Adds a candidate to be explored for the next level, or skip it if already candidate.
    /// This cannot be used when level == N, because figures must not grow at this point.
    void AddCandidate(unsigned pos) {
        if (grid[pos] != Cell::Unvisited)
            return; // Already candidate or chosen
        grid[pos] = Cell::Candidate;
        candidates[logical_sizes[level+1]++] = pos;
    }

    /// Adds all neighbours of "center" as candidates for the next level.
    /// This cannot be used when level == N, because figures must not grow at this point.
    void AddCandidates(unsigned center) {
        logical_sizes[level+1] = logical_sizes[level];
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
        if (logical_sizes[level+1] == chosen[level])
            return;
        ++level;
        chosen[level] = chosen[level-1] + 1;
    }

    /// Removes the last chosen cell from the figure.
    void Pop() {
        --level;
        // Unvisit points which have been visited due to this cell.
        for (unsigned i = logical_sizes[level]; i < logical_sizes[level+1]; ++i)
            grid[candidates[i]] = Cell::Unvisited;
        // Unchose the removed cell.
        if constexpr (grid_behaviour == GridBehaviour::Accurate)
            grid[candidates[chosen[level]]] = Cell::Candidate;
    }

    static std::array<bool, 256> generateLocalLookup() {
        std::array<bool, 256> result;
        for (unsigned compact = 0; compact < 256; ++compact) {
            bool a = compact & 128, b = compact & 64, c = compact & 32, d = compact & 16;
            bool f = compact & 8, g = compact & 4, h = compact & 2, i = compact & 1;
            int count;
            if constexpr (W == 4)
                count =
                    (f & !c) + (c & !b) + (b & !a) + (a & !d) + (d & !g) + (g & !h) + (h & !i) + (i & !f)
                    - (!a & b & d) - (!c & b & f) - (!g & d & h) - (!i & f & h);
            else
                count =
                    (f && !c) + (c && !b) + (b && !a) + (a && !d) + (d && !g) + (g && !h) + (h && !i) + (i && !f)
                    - (a && !b && !d) - (c && !b && !f) - (g && !d && !h) - (i && !f && !h);
            result[compact] = (count < 2);
        }
        return result;
    }
    
    static inline auto lookupTable = generateLocalLookup();

    /// Checks whether the last chosen cell has broken the white-connexity or not.
    /// This method is not const for (8,8) connexity which uses the grid for
    /// internal processing (even if the grid is restored before the function returns).
    bool IsValid() {
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

            // The different cases were already computed in "generateLocalLookup".
            unsigned compact = a << 7 | b << 6 | c << 5 | d << 4 | f << 3 | g << 2 | h << 1 | i;
            return lookupTable[compact];
        }
        else {
            // (8,8)-connexity
            // The objective is to start with a white neighbour of the figure, and navigate
            // its own white neighbours until no more white neighbour is visitable.
            // If we have navigated over every white neighbours, then they are all connected
            // and the white-connexity is preserved. Else, it is broken.
            grid[pos] = Cell::Chosen;
            // This vector will be populated by the visited white neighbours.
            // We use Cell::Visited88 to mark white neighbours already added to this vector.
            static thread_local std::vector<unsigned> white_neighbours;
            white_neighbours.resize(0);
            // We first visit the bottom neighbours to populate the white neighbours.
            // Those are the white cells just above the initially prohibited cells
            // due to the unicity of the starting point.
            for (unsigned pos = STARTING_POINT+1; pos < 2*WIDTH-1; ++pos) { // "-1" to keep the margin
                // Fortunately, in (8,8), the white-neighbours are also the candidates
                // to be chosen next. So we only need to check whether it is a candidate
                // (!= Cell::Unvisited), not chosen (!= Cell::Chosen), and not already
                // visited in this function (!= Cell::Visited88), which is equivalent
                // to check if it is equal to Cell::Candidate.
                if (grid[pos] == Cell::Candidate) {
                    white_neighbours.push_back(pos);
                    grid[pos] = Cell::INTERNAL;
                }
            }
            for (unsigned pos = 2*WIDTH+1; pos < 2*WIDTH+WIDTH/2+1; ++pos) {
                if (grid[pos] == Cell::Candidate) {
                    white_neighbours.push_back(pos);
                    grid[pos] = Cell::INTERNAL;
                }
            }

            // Visit all white_neighbours. Note that white_neighbours will be populated inside the loop too.
            static constexpr int neighbour_offsets[8] = {
                (int)-WIDTH-1, (int)-WIDTH, (int)-WIDTH+1, (int)-1, (int)+1, (int)+WIDTH-1, (int)+WIDTH, (int)+WIDTH+1
            };
            for (unsigned i = 0; i < white_neighbours.size(); ++i) {
                unsigned pos = white_neighbours[i];
                for (int offset : neighbour_offsets) {
                    if (grid[pos+offset] == Cell::Candidate) {
                        white_neighbours.push_back(pos+offset);
                        grid[pos+offset] = Cell::INTERNAL;
                    }
                }
            }
            unsigned nb_visited_white_neighbours = white_neighbours.size();
            for (unsigned pos : white_neighbours)
                grid[pos] = Cell::Candidate; // Restore state: remove Visited88.
            
            // The number of white neighbours should be the number of candidates
            // minus the number of chosen cells. If it is not, it means we have
            // missed some white neighbours, meaning the white-connexity is broken.
            // Note that because we do "AddCandidates()" just before "Push()",
            // we actually are only visiting candidates of the previous level.
            // This should not be an issue, because every candidate is accessible
            // by two paths: "left" and "right". Chosing a point which break one
            // of the two, will still make every candidate visited.
            // However, if you are chosing a cell which isolate a candidate,
            // it will still be detected.
            grid[pos] = Cell::Candidate;
            return nb_visited_white_neighbours >= logical_sizes[level] - (level + 1);
        }
    }
    
    /// Advances the algorithm to the next figure. Figures of size < N are also outputted.
    /// \tparam MaxSize It is possible to iterate only on smaller figures too.
    template<int MaxSize = N>
    void NextStep() {
        static constexpr int MaxLevel = std::min(MaxSize, N)-1;
        if (level == MaxLevel) {
            if constexpr (grid_behaviour == GridBehaviour::Accurate)
                grid[candidates[chosen[level]]] = Cell::Candidate;
            ++chosen[level];
        }
        else {
            AddCandidates(candidates[chosen[level]]);
            Push();
        }
        for (;;) {
            while (chosen[level] >= logical_sizes[level]) {
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
    static constexpr unsigned REPR_SIZE = (WIDTH + 1) * HEIGHT + 1;
    void GetRepr(char* repr) const {
        int i = 0;
        for (int y = HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < WIDTH; ++x)
                repr[i++] = grid[x + y * WIDTH] == Cell::Chosen ? 'X' : ' ';
            repr[i++] = '\n';
        }
        repr[i] = '\0';
    }
};