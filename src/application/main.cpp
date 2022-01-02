#include <stdexcept>
#include <cstdio>
#include <cstdlib>

#include "Application.hpp"

int main() {
    try {
        Application().Run();
        return EXIT_SUCCESS;
    } catch (std::exception const& e) {
        fputs(e.what(), stderr);
        return EXIT_FAILURE;
    }
}