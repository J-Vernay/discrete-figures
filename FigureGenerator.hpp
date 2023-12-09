#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

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
	//_Bit grid used for O(1) candidates and chosen pixel presence.

	struct BitGrid
	{
		uint64_t storage[(GridSize + 63) / 64] {};

		constexpr bool get(Pos pos)
		{
			return (storage[pos / 64] >> (pos % 64)) & 1;
		}

		constexpr void set(Pos pos)
		{
			storage[pos / 64] |= (uint64_t)1 << (pos % 64);
		}

		constexpr void reset(Pos pos)
		{
			storage[pos / 64] &= ~((uint64_t)1 << (pos % 64));
		}
	};

	// ============================================================
	// State of the algorithm.

	/// In case we want to iterate less deep than Nmax.
	/// Needed for multithreaded implementation.
	uint32_t maxLevel;

	uint32_t count;
	uint32_t level;
	uint32_t candidateCounts[Nmax];
	uint32_t chosenIndices[Nmax];
	Pos candidates[5 * Nmax];

	BitGrid gridCandidates;
	BitGrid gridChosen;

	bool lookupTableValidity[256]; ///< Used for fast white connectivity check.

	// ============================================================
	// Some optional statistics, measured if bStats is true.

	uint64_t statNonLeaf;  ///< Number of valid figures which have children.
	uint64_t statLeaf;     ///<_Number of valid figures without children.
	uint64_t statRejected; ///< Number of figures rejected by validity check.

	// ============================================================
	//_Algorithm core.

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
		gridChosen.set(PosOrigin);
		level = 0;
	}

	/// @param callbackNewFigure Called once per figure.
	/// @param nmax Maximum size to iterate.
	template <typename Func>
	void generate(Func&& callbackNewFigure, uint32_t nmax = Nmax)
	{
		maxLevel = nmax - 1;

		while (true) {
			while (checkValidity()) {
				callbackNewFigure();
				if (not firstChild())
					break;
			}
			while (not nextSibling()) {
				if (level == 0)
					return;
				parent();
			}
		}
	}

	bool firstChild()
	{
		if (level >= maxLevel)
			return false;
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
		if (idx + 1 == count)
			return false; // No neighbours added.

		++level;
		candidateCounts[level] = count;
		chosenIndices[level] = idx + 1;
		gridChosen.set(candidates[idx + 1]);
		return true;
	}

	bool nextSibling()
	{
		uint32_t idx = chosenIndices[level];
		if (idx + 1 < count) {
			gridChosen.reset(candidates[idx]);
			gridChosen.set(candidates[idx + 1]);
			chosenIndices[level] = idx + 1;
			return true;
		}
		else {
			return false;
		}
	}

	void parent()
	{
		gridChosen.reset(candidates[chosenIndices[level]]);
		--level;
		for (uint32_t idx = candidateCounts[level]; idx < count; ++idx)
			gridCandidates.reset(candidates[idx]);
		count = candidateCounts[level];
	}

	// ============================================================
	//_Validity check.

	bool checkValidity()
	{
		if constexpr (B == 0) {
			return true;
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

			if constexpr (A != 8 && B != 8) {
				return lookupTableValidity[neighbourhood];
			}
			else {
				if (lookupTableValidity[neighbourhood])
					return true;
				// For (8,8), we cannot reject for sure with the neighbourhood.
				// We need to do a proper graph traversal among the white pixels.
				return false;
			}
		}
	}

	void initLookupTableValidity()
	{
		for (uint32_t n = 0; n < 256; ++n) {
			// a b c
			// d   f
			//_g h i
			bool a = (n & 1), b = (n & 2), c = (n & 4), d = (n & 8),
			     f = (n & 16), g = (n & 32), h = (n & 64), i = (n & 128);

			// Number of components in the loop (abcfihgda).
			int nb = (a & !b) + (b & !c) + (c & !f) + (f & !i)
			       + (i & !h) + (h & !g) + (g & !d) + (d & !a);

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

			lookupTableValidity[n] = (nb <= 1);
		}
	}
};

