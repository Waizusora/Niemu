#include "niemu/version.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

int main() {
    constexpr std::string_view expected{"0.1.0"};
    const auto actual = niemu::version();

    if (actual != expected) {
        std::cerr << "version mismatch: expected " << expected
                  << ", got " << actual << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Niemu core smoke test passed (version "
              << actual << ")\n";
    return EXIT_SUCCESS;
}
