
#include "FigureGenerator.hpp"
#include "BS_thread_pool.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>


#ifndef NMAX
#define NMAX 20
#endif

using ullong = unsigned long long;

struct Result
{
	bool done = false;
	int a, b;
	ullong counts[NMAX];
	ullong time_ms;
	ullong state_bytesize;
	FigureGeneratorStats stats;
};

/// Function to dispatch to MainFunc_Xxxxx
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc(uint32_t n, bool bAlternative, bool bMultithreaded);

/// Implementation using FigureGenerator::generate().
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc_Simple(uint32_t n);

/// Implementation using FigureGenerator::nextStep().
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc_Alternative(uint32_t n);

/// Implementation using FigureGenerator::nextStep() and multithreading.
template<uint32_t A, uint32_t B>
Result MainFunc_Multithreaded(uint32_t n);


int main(int argc, char** argv)
{
	// printf("sizeof = %zu (nmax = %d)\n", sizeof(FigureGenerator<NMAX, 8, 8>), NMAX);
	// return 0;

	enum { AB40 = 1, AB48 = 2, AB44 = 4, AB80 = 8, AB88 = 16, AB84 = 32 };
	unsigned ab = 0;
	int n = 0;
	bool stat = false;
	bool alt = false;
	bool mt = false;

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
		else if (strcmp(p, "--alt") == 0)
			alt = true;
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
		printf(" --alt  : alternative single thread implementation: nextStep()\n");
		printf(" --mt   : enable multithreaded implementation\n");
		return 1;
	}
	if (mt && stat) {
		printf("Multithreadeding not compatible with statistics.\n");
		return 1;
	}
	if (mt && alt) {
		printf("Multithreading not compatible with alternative implementation.\n");
		return 1;
	}


	Result res40{}, res44{}, res48{}, res80{}, res84{}, res88{};

	if (stat) {
		if (ab & AB40) res40 = MainFunc<4, 0, true>(n, alt, mt);
		if (ab & AB48) res48 = MainFunc<4, 8, true>(n, alt, mt);
		if (ab & AB44) res44 = MainFunc<4, 4, true>(n, alt, mt);
		if (ab & AB80) res80 = MainFunc<8, 0, true>(n, alt, mt);
		if (ab & AB88) res88 = MainFunc<8, 8, true>(n, alt, mt);
		if (ab & AB84) res84 = MainFunc<8, 4, true>(n, alt, mt);
	}
	else {
		if (ab & AB40) res40 = MainFunc<4, 0, false>(n, alt, mt);
		if (ab & AB48) res48 = MainFunc<4, 8, false>(n, alt, mt);
		if (ab & AB44) res44 = MainFunc<4, 4, false>(n, alt, mt);
		if (ab & AB80) res80 = MainFunc<8, 0, false>(n, alt, mt);
		if (ab & AB88) res88 = MainFunc<8, 8, false>(n, alt, mt);
		if (ab & AB84) res84 = MainFunc<8, 4, false>(n, alt, mt);
	}

	for (Result const & res : { res40, res48, res44, res80, res88, res84 }) {
		if (not res.done)
			continue;

		printf("[n%d_a%d_b%d%s%s%s]\n", n, res.a, res.b,
			(stat ? "_stats" : ""), (alt ? "_alt" : ""), (mt ? "_mt" : ""));
		printf("time_seconds     = %f\n", res.time_ms / 1000.0);
		printf("state_bytesize   = %llu\n", res.state_bytesize);
		ullong total_count = 0;
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


/// Function to dispatch to MainFunc_Xxxxx
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc(uint32_t n, bool bAlternative, bool bMultithreaded)
{
	if (bAlternative)
		return MainFunc_Alternative<A, B, bStats>(n);
	else if (bMultithreaded)
		return MainFunc_Multithreaded<A, B>(n);
	else
		return MainFunc_Simple<A, B, bStats>(n);
}


/// Implementation using FigureGenerator::generate().
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc_Simple(uint32_t n)
{
	Result res{};
	FigureGenerator<NMAX, A, B> generator;

	BS::timer timer;
	timer.start();

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
		res.stats = (FigureGeneratorStats&)generator.stats;

	return res;
}

/// Implementation using FigureGenerator::nextStep().
template<uint32_t A, uint32_t B, bool bStats>
Result MainFunc_Alternative(uint32_t n)
{
	Result res{};
	FigureGenerator<NMAX, A, B> generator;

	BS::timer timer;
	timer.start();

	generator.init();
	do {
		++res.counts[generator.level];
	}
	while (generator.nextStep(n));

	timer.stop();
	res.time_ms = timer.ms();
	res.done = true;
	res.a = A;
	res.b = B;
	res.state_bytesize = sizeof(generator);

	if constexpr (bStats)
		res.stats = (FigureGeneratorStats&)generator.stats;

	return res;
}

/// Implementation using FigureGenerator::nextStep() and multithreading.
template<uint32_t A, uint32_t B>
Result MainFunc_Multithreaded(uint32_t n)
{
	using FigGenerator = FigureGenerator<NMAX, A, B>;

	Result res{};
	FigureGenerator<NMAX, A, B> generator;
	BS::thread_pool pool;
	std::mutex resMutex;
	std::vector<FigGenerator> tasks;
	std::atomic<size_t> tasksProgress{};
	BS::synced_stream tasksOutput;
	tasks.reserve(40000);

	constexpr uint32_t InitialDepth = (A == 4 ? 8 : 6);

	BS::timer timer;
	timer.start();

	generator.init();
	do {
		++res.counts[generator.level];
		if (generator.level == InitialDepth - 1)
			tasks.emplace_back(generator);
	}
	while (generator.nextStep(InitialDepth));

	pool.push_loop(0, tasks.size(), [&] (size_t imin, size_t imax)
	{
        // We are given a block of indices to process for this thread.
        ullong my_counts[NMAX]{};

        for (size_t i = imin; i < imax; ++i) {
            auto& generator = tasks[i];
			while (generator.nextStep(n)) {
				if (generator.level <= InitialDepth - 1)
					break;
				++my_counts[generator.level];
			}
			char buffer[100];
			snprintf(buffer, 100, "\r%4zu / %zu", ++tasksProgress, tasks.size());
			tasksOutput.print(buffer);
        }
        // Merge "my_counts" into "res.counts"
        resMutex.lock();
        for (uint32_t level = 0; level < n; ++level)
			res.counts[level] += my_counts[level];
        resMutex.unlock();
	});
	pool.wait_for_tasks();
	tasksOutput.println();

	timer.stop();
	res.time_ms = timer.ms();
	res.done = true;
	res.a = A;
	res.b = B;
	res.state_bytesize = sizeof(generator) * (1 + tasks.size());

	return res;
}










