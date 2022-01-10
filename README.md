# discrete-figures
Generation of discrete figures, also known as polyominos, animals, self-avoiding polygons.

This app was made by Julien Vernay to accompany a research paper cowritten with Hugo Tremblay on the generation of discrete figures.
It contains an illustration of Martin algorithm, along with random variants of the algorithm and an empirical analysis.

# Installation
First, you must have a C++ compiler and CMake on your machine.

Then, you must install `raylib` and its dependencies, according to the instructions specified on their official wiki.
[Windows](https://github.com/raysan5/raylib/wiki/Working-on-Windows), [Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux).

Then, compile this project:
```sh
mkdir build
cd build
cmake ..
cmake --build .
./discrete-figures
```
