
# Generation of polyominos

This repository contains code to generate all polyominos of a given size, for different connectivities.

Pixels are called "black" if they are inside the polyomino, and "white" if they are part of the background.

Connectivity (a, b) mean:

- a = 4 : Black pixels must be adjacent, diagonals are NOT allowed : each pixel has 4 neighbours.
- a = 8 : Black pixels must be adjacent, diagonals are allowed : each pixel has 8 neighbours.

- b = 0 : We accept holes in polyominos.
- b = 8 : Polyominos with holes are rejected, when two white pixels cannot be joined by a path, diagonals allowed.
- b = 4 : Polyominos with holes are rejected, when two white pixels cannot be joined by a path, diagonals NOT allowed.

# Compilation

If you know Meson build system, you can simply build it the usual way.

Else, simply compiling `main.cpp` is sufficient. NMAX, the maximum size generable, defaults to 20 if not specified.

```
gcc main.cpp -o main -O2 -DNMAX=20

cl.exe main.cpp /O2 /DNMAX=20
```

# Command line usage

To generate figures of size 13 or less, for all connectivities, using the multithreaded implementation:

```
./main 40 48 44 80 88 84 -n13 --mt
```

Complete usage:

```
Usage: ./main <conn...> -n8 [--stat] [--mt]
 conn...: either 40, 44, 48, 80, 84 or 88
 -n     : max size of figure, between 1 and 20
          (for bigger figures, recompile and change NMAX)
 --stat : enable various statistics, lower performances
 --alt  : alternative single thread implementation: nextStep()
 --mt   : enable multithreaded implementation
```


