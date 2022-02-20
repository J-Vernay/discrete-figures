/// \file SimpleMartinAlgo.hpp Straight-forward implementation of the Martin algorithm
/// preserving the mathematical notions presented in the research paper.

#pragma once

#include <array>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include <bitset>
#include <cstring>


// List of all directions.
enum Direction {
    DirE, DirNE, DirN, DirNW, DirW, DirSW, DirS, DirSE
};

// Position of a candidate. Limiting ourself to 2^16 large grid,
// sufficient to handle all figures up to n = 2^15-1 pixels.
struct Coordinate {
    std::int16_t x;
    std::int16_t y;
    // We can apply a direction to obtain a new coordinate.
    Coordinate apply(Direction d) const noexcept {
        switch (d) {
        case DirE:  return { x+1, y   };
        case DirNE: return { x+1, y+1 };
        case DirN : return { x  , y+1 };
        case DirNW: return { x-1, y+1 };
        case DirW : return { x-1, y   };
        case DirSW: return { x-1, y-1 };
        case DirS : return { x  , y-1 };
        case DirSE: return { x+1, y-1 };
        default: return {x,y};
        }
    }
};

// Maximum number of chosen pixels in a figure.
static constexpr int MAX_FIGURE_SIZE = 0x7FFF;

// Allowing hashing of coordinates by composing its two members (x,y) in a 32 bit value which is then hashed. 
template<> struct std::hash<Coordinate> {
    std::size_t operator()(Coordinate c) const noexcept {
        std::uint32_t x = (std::uint16_t)c.x;
        std::uint32_t y = (std::uint16_t)c.y;
        // Inspired from Boost hash_combine()
        return (x << 2) + 0x9e3779b9 + (y << 6) + (y >> 2);
    }
};

// Allowing equality check between coordinates.
inline bool operator==(Coordinate a, Coordinate b) noexcept {
    return a.x == b.x && a.y == b.y;
}

// A candidate is the sum of the following informations:
// 'p' is the 2D coordinate of the candidate: 'x' and 'y'.
// 'k' is the level of appearance as candidate.
// 's' is the state, either Free, Chosen or Prohibited, encoded as in 'State' enum.
// 'i' is the level of the state, i.e. at which level was it Chosen or Prohibited.
// These information are packed in 64 bits per candidate.
// It supports up to n = 32767 (2^15 - 1) pixels in the figure.
// This is a reasonable range, given that we can only enumerate figures
// currently for 56 pixels.
struct alignas(uint64_t) Candidate {
    // Note: 'Unvisited' is only used in Optimized version to provide a default state for pixels.
    enum State : uint8_t { Free = 0, Chosen = 1, Prohibited = 2, Unvisited = 3 };
    // Conversion from state to ASCII letter.
    static constexpr char StateLetter[] = { 'F', 'C', 'P', ' ' };

    Coordinate coordinate;
    uint32_t k : 15, s : 2, i : 15;
};
static_assert(sizeof(Candidate) == 8, "Candidate is exactly 64 bits.");

// The state required to execute the Martin algorithm.
// In theory, only the list of candidates is required to use the
// algorithm. However it would require frequent linear searches
// to e.g. find candidates with a given 'k' or next free candidate.
// In practice, it is faster and more convenient to maintain
// additional data, at the cost of duplicating data.
struct MartinAlgoSimple {
    // List of the candidates, as described in the research paper.
    std::vector<Candidate> candidates;
    // Indice of the next free candidate.
    unsigned next_free;
    // Current level of the algorithm = the number of chosen candidates.
    unsigned level;
    // Indices where candidates have given 'k', i.e. 'k_start[j]'
    // returns the index of the first candidate with 'k == j'.
    std::vector<unsigned> k_start;
    // Indices where candidate have been chosen, i.e. 'chosen[j]'
    // returns the index of the chosen candidate with 'i == j'.
    std::vector<unsigned> chosen;
    // Mapping from coordinate to candidate index.
    // A coordinate is present if it is already a candidate.
    // Prevents candidates being added twice, and allow to
    // access a candidate state from its coordinate.
    std::unordered_map<Coordinate, unsigned> candidate_indices;

    // Initializes the Martin state. 'n' is only used as an hint
    // of the max number of cells, preventing reallocations.
    void Init(int n = -1) {
        if (n > 0) {
            candidates.reserve(4 * (n+1));
            k_start.reserve(n+1);
            chosen.reserve(n);
        }
        candidates = {};
        k_start = { 0 };
        chosen = {};
        next_free = 0;
        level = 0;
        candidate_indices = {};
        AddCandidate({0,0});
    }

    // Add the next candidate, given its index in the 'candidates' list.
    // For enumeration, use 'candidate_id = next_free'.
    // Pushing another candidate is equivalent to skipping push and pop operation
    // until the given candidate would be the next free candidate, meaning all
    // previous candidates are either chosen or prohibited.
    // After pushing, you are responsible of adding the neighbours as candidates
    // by calling 'AddCandidate()'.
    // Returns the coordinate of the newly chosen candidate.
    Coordinate Push(unsigned candidate_id) {
        if (candidate_id < next_free || candidate_id >= candidates.size()) {
            throw std::out_of_range("Invalid candidate index provided.");
        }
        // Prohibiting previous free candidates.
        for (unsigned i = next_free; i < candidate_id; ++i) {
            candidates[i].s = Candidate::Prohibited;
            candidates[i].i = level;
        }

        // Going to the next level.
        ++level;

        // Mark the given candidate as "chosen".
        chosen.push_back(candidate_id);
        candidates[candidate_id].s = Candidate::Chosen;
        candidates[candidate_id].i = level;

        ++next_free;

        return candidates[candidate_id].coordinate;
    }

    // Add candidate at the current level, preventing duplicates.
    // Should be called just after Push() to add neighbours.
    void AddCandidate(Coordinate coordinate) {
        // Ensuring we are not adding a point which would replace the origin.
        // Otherwise, we would break the precondition that (0,0) is the
        // origin as being the bottom-row left-most point of the figure.
        if (coordinate.y > 0 || coordinate.y == 0 && coordinate.x >= 0) {
            auto [it, inserted] = candidate_indices.insert({coordinate, candidates.size()});
            if (inserted) { // It was not itn 'candidate_indices' before.
                candidates.push_back(
                    Candidate{ coordinate, level, Candidate::Free, 0 }
                );
            }
        }
    }

    // Add the 4-connected candidates.
    void AddCandidates4(Coordinate coordinate) {
        for (Direction d : { DirE, DirN, DirW, DirS })
            AddCandidate(coordinate.apply(d));
    }
    
    // Add the 8-connected candidates.
    void AddCandidates8(Coordinate coordinate) {
        for (Direction d : { DirE, DirNE, DirN, DirNW, DirW, DirSW, DirS, DirSE })
            AddCandidate(coordinate.apply(d));
    }

    // Removes the last level, prohibiting the last chosen candidate,
    // and removing all candidates added since last Push(), then return true.
    // If there are no more Push() to undo, return false and do nothing.
    bool Pop() {
        if (level <= 0) {
            return false; // Do nothing.
        }
        // Remove all candidates added since last Push(): 'k == level'.
        while (candidates.size() > 0 && candidates.back().k == level) {
            candidate_indices.erase(candidates.back().coordinate);
            candidates.pop_back();
        }
        // Free all candidates prohibited in the current level.
        for (int i = chosen.back() + 1; i < candidates.size(); ++i) {
            if (candidates[i].s == Candidate::Prohibited && candidates[i].i == level) {
                candidates[i].s = Candidate::Free;
            } else {
                // Prohibited candidates are contiguous, so we know
                // we have reached the end of prohibited candidates.
                break;
            }
        }

        // Going back to previous level.
        --level;
        int last_chosen = chosen.back();
        chosen.pop_back();

        // Prohibiting last chosen candidate.
        candidates[last_chosen].s = Candidate::Prohibited;
        candidates[last_chosen].i = level;

        next_free = last_chosen + 1;
        return true;
    }

    // Easy access to candidate given its coordinate.
    bool IsChosen(Coordinate c) const {
        auto it = candidate_indices.find(c);
        if (it == candidate_indices.end())
            return false;
        return candidates[it->second].s == Candidate::Chosen;
    }

    // Check whether the local white connexity is respected after the inclusion of 'c'.
    bool WouldBreakWhiteLocal4(Coordinate c) const {
        bool A = IsChosen(c.apply(DirNW)), B = IsChosen(c.apply(DirN)), C = IsChosen(c.apply(DirNE)),
             D = IsChosen(c.apply(DirW)),  F = IsChosen(c.apply(DirE)),
             G = IsChosen(c.apply(DirSW)), H = IsChosen(c.apply(DirS)), I = IsChosen(c.apply(DirSE));

        // Given the following situation:
        // A B C
        // D c F  (c is the given center)
        // G H I
        // For each pair (F,C), (C,B), ..., (I,F), we check if it is equal to (Chosen, NotChosen).
        // By counting the number of such pairs, we know the number of white parts after the insertion
        // of 'c'. If it is 0 or 1, there are no white breakage. Else, we would have multiple parts.
        // There is a special case for the corners A, C, G, I:
        // if A is white and B and D are chosen, it means A is already connected "outside" the local context.
        // In this case, we are not breaking "more" the local connexity, so such cases should be removed.

        int count = (F && !C) + (C && !B) + (B && !A) + (A && !D) + (D && !G) + (G && !H) + (H && !I) + (I && !F)
            - (!A && B && D) - (!C && B && F) - (!G && D && H) - (!I && F && H);
        
        return count >= 2;
    }

    bool WouldBreakWhiteLocal8(Coordinate c) const {
        bool A = IsChosen(c.apply(DirNW)), B = IsChosen(c.apply(DirN)), C = IsChosen(c.apply(DirNE)),
             D = IsChosen(c.apply(DirW)),  F = IsChosen(c.apply(DirE)),
             G = IsChosen(c.apply(DirSW)), H = IsChosen(c.apply(DirS)), I = IsChosen(c.apply(DirSE));
        // This is the simple principle as WouldBreakWhiteLocal4(), 
        // except the handling of corners: the special case "!A && B && D" is not possible,
        // since the corners are 8-connected to the center.
        // However, the case "A && !B && !D" should not count, as B and D are still connected.

        int count = (F && !C) + (C && !B) + (B && !A) + (A && !D) + (D && !G) + (G && !H) + (H && !I) + (I && !F)
            - (A && !B && !D) - (C && !B && !F) - (G && !D && !H) - (I && !F && !H);
        
        return count >= 2;
    }
};