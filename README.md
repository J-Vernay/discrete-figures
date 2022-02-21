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

Finally, an optimized AND multithreaded vrsion is available (on a 12-cores machine,
the measure was 8 times faster). The arguments are also passed at compile-time,
with the extra macro `T` which is the threshold level used to split the work among threads.
It also provides an approximate remaining time, because the "splitting" is used to approximate
the amount of work to be done. Because outputting the progress takes some time, it must be enabled
with the macro `SHOW_PROGRESS`.

NOTE: The multithreaded version is based on the "thread_pool" library proposed by [Barak Shoshany](https://github.com/bshoshany/thread-pool) in the paper [arXiv:2105.00613](https://arxiv.org/abs/2105.00613).

```
$ g++ src/opti_mt_enumeration.cpp -o opti_mt_enumeration -O3 -DB=4 -DW=4 -DT=9 -DN=20 -DSHOW_PROGRESS
$ ./opti_mt_enumeration
2.65615 % (256 / 9638)
5.31231 % (512 / 9638)
...
98.2776 % (9472 / 9638)
Generation of (4,4)-connected figures (24681869833 in total) in 44.706667 s
        (avg: 552.08 * 10^6 figures/s).
NOTE: "expected" is the OEIS data, lacking white-connexity. "0" is used when unknown.
              n,          result,        expected
              1,               1,               1
              2,               2,               2
              3,               6,               6
             ...           ...             ...
             19,      4796310672,      5940738676
             20,     18155586993,     22964779660
```

You can also visualise which figures are rejected due to the white-connexity. The code is based on the optimized version, so arguments are passed at compile-time too.

```
$ g++ src/white_rejected.cpp -o white_rejected -O3 -DN=7 -DB=4 -DW=4
$ ./white_rejected
1. (size=7)
       XX      
       X X     
       XXX     
2. (size=7)
        XX     
       X X     
       XXX     
3. (size=7)
       XXX     
       X X     
       XX      
4. (size=7)
      XXX      
      X X      
       XX      
Total 4-black-connected figures (size <= 7) due to 4-white-connexity: 4
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
