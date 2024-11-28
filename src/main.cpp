#include <iostream>
#include <string>
#include "compiler.h"

int main(int argc, char* argv[]) {
    // If no input provided, use a default expression
    std::string input = (argc > 1) ? argv[1] : "5 + 3 * 2";

    try {
        Compiler compiler(input);
        int result = compiler.compile();
        std::cout << "Compilation successful. Result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}