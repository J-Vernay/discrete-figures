#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

// Helper to disable state storage when not needed.
template<bool Condition, typename T>
struct StoreIf : T {};

template<typename T>
struct StoreIf<false, T> {};

struct FigureGeneratorStats {
	uint64_t nonLeaf;  // Number of figures with children.
	uint64_t leaf;     // Number of figures without children.
	uint64_t rejected; // Number of rejections by the validity check.
};

/// @tparam Nmax Maximum size of generated figures
/// @tparam A Connectivity of chosen pixels: 4 or 8.
/// @tparam B Connectivity of non-chosen pixels: 4, 8 or 0 to disable check.
/// @tparam bStats Whether to measure some statistics, with worse performances.
template<uint32_t Nmax, uint32_t A, uint32_t B, bool bStats = false>
struct FigureGenerator
{
	static_assert(A == 4 || A == 8);
	static_assert(B == 0 || B == 4 || B == 8);

	// ============================================================
	// Constants definitions related to the grid where figures will be generated.
	// Width and Height have margins such that we do not require bound-checking.

	static constexpr int32_t Width = 2 * Nmax + 3;
	static constexpr int32_t Height = Nmax + 4;
	static constexpr int32_t GridSize = Width * Height;

	static_assert(GridSize <= INT16_MAX, "Positions can be stored in int16_t");
	using Pos = int16_t;

	static constexpr Pos PosOrigin = (Width / 2) + 2 * Width;

	// ============================================================
	// Directions are defined as position offsets.
	// Convention: bottom-left is (0,0)

	enum Dir : Pos
	{
		DirRight = +1,
		DirUp    = +Width,
		DirLeft  = -1,
		DirDown  = -Width,

		DirUpLeft    = DirUp   + DirLeft,
		DirUpRight   = DirUp   + DirRight,
		DirDownLeft  = DirDown + DirLeft,
		DirDownRight = DirDown + DirRight,
	};

	// ============================================================
	// Bit grid used for O(1) candidates and chosen pixel presence.

	struct BitGrid
	{
		static constexpr uint32_t U64size = (GridSize + 63) / 64;
		uint64_t u64[U64size] {};

		constexpr bool get(Pos pos)
		{
			return (u64[pos / 64] >> (pos % 64)) & 1;
		}

		constexpr void set(Pos pos)
		{
			u64[pos / 64] |= (uint64_t)1 << (pos % 64);
		}

		constexpr void reset(Pos pos)
		{
			u64[pos / 64] &= ~((uint64_t)1 << (pos % 64));
		}
	};

	// ============================================================
	// State of the algorithm.

	uint32_t count;
	uint32_t level;
	uint32_t candidateCounts[Nmax];
	uint32_t chosenIndices[Nmax];
	Pos candidates[5 * Nmax];

	BitGrid gridCandidates;

	// Only needed for white connectivity check.
	StoreIf<B != 0, BitGrid> gridChosen;

	// ============================================================
	// Validity check state.

	struct ValidityLookup { bool table[256]; };
	StoreIf<B != 0, ValidityLookup> validityLookup;

	/// Only used for connectivity (8,8)
	struct Visit {
		BitGrid grid;
		Pos queue[5 * Nmax + Width + 1];
		uint32_t count;
	};
	StoreIf<A == 8 && B == 8, Visit> visit;

	// ============================================================
	// Some optional statistics, measured if bStats is true.

	StoreIf<bStats, FigureGeneratorStats> stats;

	// ============================================================
	// Algorithm core.

	void init()
	{
		*this = {};
		initLookupTableValidity();
		candidates[0] = PosOrigin;
		count = 1;
		candidateCounts[0] = 1;
		for (Pos pos = 0; pos <= PosOrigin; ++pos)
			gridCandidates.set(pos);
		chosenIndices[0] = 0;
		if constexpr (B != 0)
			gridChosen.set(PosOrigin);
		level = 0;
	}

	/// @param callbackNewFigure Called once per figure.
	/// @param nmax Maximum size to iterate.
	template <typename Func>
	void generate(Func&& callbackNewFigure, uint32_t nmax = Nmax)
	{
		if (nmax > Nmax)
			nmax = Nmax;
		uint32_t maxLevel = nmax - 1;

		while (true) {
			while (checkValidity()) {
				callbackNewFigure();
				// extra logic to stop iterating before.
				if (level >= maxLevel) {
					if constexpr (bStats)
						++stats.nonLeaf;
					break;
				}
				else if (not firstChild()) {
					break;
				}
			}
			while (not nextSibling()) {
				if (level == 0)
					return;
				parent();
			}
		}
	}

	/// Rewording of the generate() function, but a single invocation
	/// corresponds to the code executed between two valid figures.
	/// @retval true if current figure is valid and iteration can continue.
	/// @retval false if we have reached end of iteration.
	bool nextStep(uint32_t nmax = Nmax)
	{
		if (level < nmax - 1)
			if (firstChild())
				if (checkValidity())
					return true;
		do {
			while (not nextSibling()) {
				if (level == 0)
					return false;
				parent();
			}
		} while (not checkValidity());
		return true;
	}

	bool firstChild()
	{
		uint32_t idx = chosenIndices[level];
		Pos pos = candidates[idx];

		// Add neighbours of 'pos' as candidates.

		auto funcAddCandidate = [this] (Pos pos) {
			if (not gridCandidates.get(pos)) {
				gridCandidates.set(pos);
				candidates[count] = pos;
				++count;
			}
		};

		if constexpr (A == 4) {
			funcAddCandidate(pos + DirRight);
			funcAddCandidate(pos + DirUp);
			funcAddCandidate(pos + DirLeft);
			funcAddCandidate(pos + DirDown);
		}
		else {
			funcAddCandidate(pos + DirRight);
			funcAddCandidate(pos + DirUpRight);
			funcAddCandidate(pos + DirUp);
			funcAddCandidate(pos + DirUpLeft);
			funcAddCandidate(pos + DirLeft);
			funcAddCandidate(pos + DirDownLeft);
			funcAddCandidate(pos + DirDown);
			funcAddCandidate(pos + DirDownRight);
		}
		if (idx + 1 == count) {
			if constexpr (bStats)
				++stats.nonLeaf;
			return false;
		}

		++level;
		candidateCounts[level] = count;
		chosenIndices[level] = idx + 1;
		if constexpr (B != 0)
			gridChosen.set(candidates[idx + 1]);
		if constexpr (bStats)
			++stats.leaf;
		return true;
	}

	bool nextSibling()
	{
		uint32_t idx = chosenIndices[level];
		if (idx + 1 < count) {
			if constexpr (B != 0) {
				gridChosen.reset(candidates[idx]);
				gridChosen.set(candidates[idx + 1]);
			}
			chosenIndices[level] = idx + 1;
			return true;
		}
		else {
			return false;
		}
	}

	void parent()
	{
		if constexpr (B != 0)
			gridChosen.reset(candidates[chosenIndices[level]]);
		--level;
		for (uint32_t idx = candidateCounts[level]; idx < count; ++idx)
			gridCandidates.reset(candidates[idx]);
		count = candidateCounts[level];
	}

	// ============================================================
	// Validity check.

	bool checkValidity()
	{
		bool bResult = false;
		if constexpr (B == 0) {
			bResult = true;
		}
		else {
			// Get neighbourhood.
			Pos pos = candidates[chosenIndices[level]];
			bool a = gridChosen.get(pos + DirUpLeft);
			bool b = gridChosen.get(pos + DirUp);
			bool c = gridChosen.get(pos + DirUpRight);
			bool d = gridChosen.get(pos + DirLeft);
			bool f = gridChosen.get(pos + DirRight);
			bool g = gridChosen.get(pos + DirDownLeft);
			bool h = gridChosen.get(pos + DirDown);
			bool i = gridChosen.get(pos + DirDownRight);
			uint8_t neighbourhood = (a << 0) | (b << 1) | (c << 2) | (d << 3)
			                      | (f << 4) | (g << 5) | (h << 6) | (i << 7);

			if constexpr (A != 8 || B != 8) {
				bResult = validityLookup.table[neighbourhood];
			}
			else {
				if (validityLookup.table[neighbourhood]) {
					bResult = true;
				}
				else {
					// For (8,8), we cannot reject for sure with the neighbourhood.
					// We need to do a proper graph traversal among the white pixels.

					// White neighbours are candidates, except chosen pixels.
					for (uint32_t k = 0; k < BitGrid::U64size; ++k)
						visit.grid.u64[k] = gridCandidates.u64[k] & ~gridChosen.u64[k];

					// In gridCandidates, we also have all pos before PosOrigin.
					// Among them, all pos before (PosOrigin + DirDownLeft) are
					//_not necessary to visit.
					constexpr Pos FirstVisitPos = (PosOrigin + DirDownLeft);
					for (uint32_t k = 0; k < FirstVisitPos / 64; ++k)
						visit.grid.u64[k] = 0;
					for (Pos p = ((FirstVisitPos / 64) * 64); p <= FirstVisitPos; ++p)
						visit.grid.reset(p);

					visit.count = 1;
					visit.queue[0] = FirstVisitPos;

					auto funcVisit = [this](Pos pos) {
						if (visit.grid.get(pos)) {
							visit.grid.reset(pos);
							visit.queue[visit.count] = pos;
							++visit.count;
						}
						};

					while (visit.count > 0) {
						--visit.count;
						Pos p = visit.queue[visit.count];
						funcVisit(p + DirRight);
						funcVisit(p + DirUpRight);
						funcVisit(p + DirUp);
						funcVisit(p + DirUpLeft);
						funcVisit(p + DirLeft);
						funcVisit(p + DirDownLeft);
						funcVisit(p + DirDown);
						funcVisit(p + DirDownRight);
					}
					// If there is any gridVisit pixel not visited, reject.
					bResult = true;
					for (uint32_t k = 0; k < BitGrid::U64size; ++k) {
						if (visit.grid.u64[k]) {
							bResult = false;
							break;
						}
					}
				}
			}
		}
		if constexpr (bStats)
			stats.rejected += !bResult;
		return bResult;
	}

	void initLookupTableValidity()
	{
		if constexpr (B != 0) {
			for (uint32_t n = 0; n < 256; ++n) {
				// a b c
				// d   f
				// g h i
				bool a = (n & 1), b = (n & 2), c = (n & 4), d = (n & 8),
					 f = (n & 16), g = (n & 32), h = (n & 64), i = (n & 128);

				// Number of components in the loop (fcbadghif).
				int nb = (f & !c) + (c & !b) + (b & !a) + (a & !d)
					   + (d & !g) + (g & !h) + (h & !i) + (i & !f);

				if constexpr (B == 8) {
					// Remove false positives due to black (acgi) with white neighbours.
					nb -= (a & !b & !d) + (c & !b & !f)
						+ (g & !d & !h) + (i & !f & !h);
				}
				if constexpr (A == 8 && B == 4) {
					// In (8,4), if (acgi) is white with black neighbours, it means
					// (acgi) is connected to white pixels through an external white path.
					// Thus, these disconnected cases do not invalidate the figure.
					nb -= (!a & b & d) + (!c & b & f) + (!g & d & h) + (!i & f & h);
				}

				validityLookup.table[n] = (nb <= 1);
			}
		}
	}
};

