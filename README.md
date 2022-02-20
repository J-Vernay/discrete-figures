# discrete-figures
Generation of discrete figures, also known as polyominos, animals, self-avoiding polygons.

This app was made by Julien Vernay to accompany a research paper cowritten with Hugo Tremblay on the generation of discrete figures.
It contains an illustration of Martin algorithm, along with random variants of the algorithm and an empirical analysis.

# Installation

To enumerate figures, you only need a C++17 compiler.
You can use the simple enumeration by compiling `src/opti_enumeration.cpp`.
Then, launch the executable with arguments N (size), B (black-connexity) and W (white-connexity).

```
$ g++ src/simple_enumeration.cpp -o simple_enumeration -O3
$ ./simple_enumeration 8 4 4
Generation of (4,4)-connected figures (3784 in total) in 0.001181 s
        (avg: 3.20 * 10^6 figures/s).
NOTE: "expected" is the OEIS data, lacking white-connexity. "0" is used when unknown.
              n,          result,        expected
              1,               1,               1
              2,               2,               2
              3,               6,               6
              4,              19,              19
              5,              63,              63
              6,             216,             216
              7,             756,             760
              8,            2684,            2725
```

For the optimized version, which is way faster (17 times faster on my machine),
the arguments are passed at **compile-time** with macro definitions:
```
$ g++ src/opti_enumeration.cpp -o opti_enumeration -O3 -DN=8 -DB=4 -DW=4
$ ./opti_enumeration
Generation of (4,4)-connected figures (3747 in total) in 0.000078 s
        (avg: 48.12 * 10^6 figures/s).
NOTE: "expected" is the OEIS data, lacking white-connexity. "0" is used when unknown.
              n,          result,        expected
              1,               1,               1
              2,               2,               2
              3,               6,               6
              4,              19,              19
              5,              63,              63
              6,             216,             216
              7,             756,             760
              8,            2684,            2725
```

If you want to build the application used to show a graphical representation of the algorithm steps,
you must install `raylib` and its dependencies, according to the instructions specified on their official wiki:
[Windows](https://github.com/raysan5/raylib/wiki/Working-on-Windows), [Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux).

Then, compile this project:
```sh
mkdir build
cd build
cmake ../app
cmake --build .
./discrete-figures
```
