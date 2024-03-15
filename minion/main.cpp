// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// This is a minimal 'main', which can be easily removed to make Minion a library

int minion_main(int argc, char** argv);

#include <iostream>
#include <ostream>

int main(int argc, char** argv) {
    if(sizeof(void*) == 4) {
        std::cerr << "Minion does not support 32-bit builds\n";
        exit(1);
    }
    minion_main(argc, argv);
}