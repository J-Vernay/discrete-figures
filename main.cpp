
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
	bool done = false;
	int a, b;
	unsigned long long counts[NMAX];
	unsigned long long time_ms;
	unsigned long long state_bytesize;
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
	res.done = true;
	res.a = A;
	res.b = B;
	res.state_bytesize = sizeof(generator);

	if constexpr (bStats)
		res.stats = generator.stats;

	return res;
};


int main(int argc, char** argv)
{
	enum { AB40 = 1, AB48 = 2, AB44 = 4, AB80 = 8, AB88 = 16, AB84 = 32 };
	unsigned ab = 0;
	int n = 0;
	bool stat = false;
	bool mt = false;
	bool csv = false;

	for (int i = 1; i < argc; ++i) {
		char const* p = argv[i];
		if (p[0] == '-' && p[1] == 'n')
			n = atoi(p + 2);
		else if (strcmp(p, "40") == 0)
			ab |= AB40;
		else if (strcmp(p, "48") == 0)
			ab |= AB48;
		else if (strcmp(p, "44") == 0)
			ab |= AB44;
		else if (strcmp(p, "80") == 0)
			ab |= AB80;
		else if (strcmp(p, "88") == 0)
			ab |= AB88;
		else if (strcmp(p, "84") == 0)
			ab |= AB84;
		else if (strcmp(p, "--stat") == 0)
			stat = true;
		else if (strcmp(p, "--mt") == 0)
			mt = true;
		else if (strcmp(p, "--csv") == 0)
			csv = true;
		else {
			printf("Unrecognized argument: %s\n", p);
			return 1;
		}
	}

	if (n == 0 || n > NMAX || ab == 0) {
		printf("Usage: %s <conn...> -n8 [--stat] [--mt]\n", argv[0]);
		printf(" conn...: either 40, 44, 48, 80, 84 or 88\n");
		printf(" -n     : max size of figure, between 1 and %d\n", NMAX);
		printf("          (for bigger figures, recompile and change NMAX)\n");
		printf(" --stat : enable various statistics, lower performances\n");
		printf(" --mt   : enable multithreaded implementation\n");
		return 1;
	}

	Result res40{}, res44{}, res48{}, res80{}, res84{}, res88{};

	if (stat) {
		if (ab & AB40) res40 = MainFunc<4, 0, true>(n);
		if (ab & AB48) res48 = MainFunc<4, 8, true>(n);
		if (ab & AB44) res44 = MainFunc<4, 4, true>(n);
		if (ab & AB80) res80 = MainFunc<8, 0, true>(n);
		if (ab & AB88) res88 = MainFunc<8, 8, true>(n);
		if (ab & AB84) res84 = MainFunc<8, 4, true>(n);
	}
	else {
		if (ab & AB40) res40 = MainFunc<4, 0, false>(n);
		if (ab & AB48) res48 = MainFunc<4, 8, false>(n);
		if (ab & AB44) res44 = MainFunc<4, 4, false>(n);
		if (ab & AB80) res80 = MainFunc<8, 0, false>(n);
		if (ab & AB88) res88 = MainFunc<8, 8, false>(n);
		if (ab & AB84) res84 = MainFunc<8, 4, false>(n);
	}

	for (Result const & res : { res40, res48, res44, res80, res88, res84 }) {
		if (not res.done)
			continue;
		printf("[n%d_a%d_b%d%s]\n", n, res.a, res.b, (stat ? "_stats" : ""));
		printf("time_seconds     = %f\n", res.time_ms / 1000.0);
		printf("state_bytesize   = %llu\n", res.state_bytesize);
		unsigned long long total_count = 0;
		for (int level = 0; level < n; ++level) {
			total_count += res.counts[level];
			printf("count_%-10d = %20llu\n", level + 1, res.counts[level]);
		}
		printf("total_count      = %llu\n", total_count);
		printf("millions_per_sec = %f\n", (total_count / 1000'000.0) / (res.time_ms / 1000.0));
		if (stat) {
			printf("stat_non_leaf    = %llu\n", res.stats.nonLeaf);
			printf("stat_leaf        = %llu\n", res.stats.leaf);
			printf("stat_rejected	 = %llu\n", res.stats.rejected);
			printf("ratio_non_leaf_valid = %5.2f # percent\n", res.stats.nonLeaf * 100.0 / total_count);
			printf("ratio_leaf_valid     = %5.2f # percent\n", res.stats.leaf * 100.0 / total_count);
			printf("ratio_rejected_valid = %5.2f # percent\n", res.stats.rejected * 100.0 / total_count);
		}
		printf("\n");
	}
	return 0;
}
