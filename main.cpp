
#include "FigureGenerator.hpp"
#include "BS_thread_pool.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef NMAX
#define NMAX 20
#endif

struct Result
{
	unsigned long long counts[NMAX];
	unsigned long long time_ms;
	FigureGeneratorStats stats;
};

template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc(uint32_t n)
{
	Result res{};

	BS::timer timer;
	timer.start();

	FigureGenerator<NMAX, A, B, bStats> generator;
	generator.init();

	generator.generate([&] {
		++res.counts[generator.level];
	}, n);

	timer.stop();
	res.time_ms = timer.ms();

	if constexpr (bStats)
		res.stats = generator.stats;

	return res;
};


int main(int argc, char** argv)
{
	uint32_t n = 0;
	uint32_t a = 0;
	uint32_t b = 0;
	bool stat = false;
	bool mt = false;
	bool csv = false;

	for (int i = 1; i < argc; ++i) {
		char const* p = argv[i];
		if (p[0] != '-')
			continue;
		if (p[1] == 'a')
			a = atoi(p + 2);
		else if (p[1] == 'b')
			b = atoi(p + 2);
		else if (p[1] == 'n')
			n = atoi(p + 2);
		else if (strcmp(p + 1, "stat") == 0)
			stat = true;
		else if (strcmp(p + 1, "mt") == 0)
			mt = true;
		else if (strcmp(p + 1, "csv") == 0)
			csv = true;
	}

	if (n == 0 || n > NMAX || (a != 4 && a != 8) || (b != 0 && b != 4 && b != 8)) {
		printf("Usage: %s -n12 -a4 -b4 [-stat] [-mt]\n", argv[0]);
		printf(" -n    : max size of figure, between 1 and %d\n", NMAX);
		printf("         (for bigger figures, recompile and change NMAX)\n");
		printf(" -a    : black connectivity, either 4 or 8\n");
		printf(" -b    : white connectivity, either 4, 8, or 0 (disabled)\n");
		printf(" -stat : enable various statistics, lower performances\n");
		printf(" -mt   : enable multithreaded implementation\n");
		printf(" -csv  : print a single line: config, time_ms, total_count\n");
		return 1;
	}

	Result res{};
	if (stat) {
		if (a == 4 && b == 0) res = MainFunc<4, 0, true>(n);
		if (a == 4 && b == 4) res = MainFunc<4, 4, true>(n);
		if (a == 4 && b == 8) res = MainFunc<4, 8, true>(n);
		if (a == 8 && b == 0) res = MainFunc<8, 0, true>(n);
		if (a == 8 && b == 4) res = MainFunc<8, 4, true>(n);
		if (a == 8 && b == 8) res = MainFunc<8, 8, true>(n);
	}
	else {
		if (a == 4 && b == 0) res = MainFunc<4, 0, false>(n);
		if (a == 4 && b == 4) res = MainFunc<4, 4, false>(n);
		if (a == 4 && b == 8) res = MainFunc<4, 8, false>(n);
		if (a == 8 && b == 0) res = MainFunc<8, 0, false>(n);
		if (a == 8 && b == 4) res = MainFunc<8, 4, false>(n);
		if (a == 8 && b == 8) res = MainFunc<8, 8, false>(n);
	}

	unsigned long long total_count = 0;
	for (int level = 0; level < NMAX; ++level) {
		total_count += res.counts[level];
	}

	if (csv) {
		printf("a=%d_b=%d_n=%d, %llu, %llu\n", a, b, n, res.time_ms, total_count);
	}
	else {
		printf("a=%d, b=%d, n=%d\n", a, b, n);
		printf("time = %f seconds\n", res.time_ms / 1000.0);
		printf("total_count = %llu\n", total_count);
		for (int level = 0; level < NMAX; ++level) {
			printf("%8d : %20llu\n", level + 1, res.counts[level]);
		}
	}

	return 0;
}
