#include <iostream>
#include <string>
#include "include/source_reader.h"
#include "include/compiler.h"

int main(int argc, char* argv[]) {
    
    std::string path = "."; // Default value for path to save resulting artifacts.

    if (argc == 1) 
    {
        std::cerr << "Fatal error: no input files, process terminated" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (argc == 3)
    {
        path = argv[2];
    }
    

    std::string filename = argv[1];
    std::string sourceCode = Lei::SourceReader::readSourceFile(filename);
    if (!sourceCode.empty()) {
        std::cout << "File contents:\n" << sourceCode << std::endl;
    }
    

    try {
        Lei::Lexer LX = Lei::Lexer(sourceCode);
        std::vector<Lei::Token> tokens = LX.tokenize();

    } catch (const std::exception& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return 1;
    }

    return EXIT_SUCCESS;
}