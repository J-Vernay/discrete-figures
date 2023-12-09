
#include "FigureGenerator.hpp"

#include <stdio.h>

int main()
{
	unsigned long long figureCounts[15] {};

	FigureGenerator<10, 8, 4> generator;

	generator.init();

	generator.generate([&] {
		++figureCounts[generator.level];
	});

	for (int i = 0; i < 15; ++i)
		printf("n = %2d : %20llu\n", i + 1, figureCounts[i]);

	return 0;
}
