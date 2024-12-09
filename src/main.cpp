#include "../include/common.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "symbol_table.h"
#include "code_generator.h"

#include "llvm/Support/TargetSelect.h"
#include "source_reader.h"

int main(int argc, char* argv[]) {
    // LLVM initialization
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

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
    std::string code = Lei::SourceReader::readSourceFile(filename);
    if (!code.empty()) {
        std::cout << "Source file contents:\n" << sourceCode << std::endl;
    }
    


    try {
            // Lexical analysis
            Lexer lexer(code);
            auto tokens = lexer.tokenize();

            // Parsing
            Parser parser(tokens);
            auto functionAST = parser.parseFunction();

            // Semantic analysis
            SymbolTable symbols;
            SemanticAnalyzer analyzer(symbols);
            analyzer.checkFunction(*functionAST);

            // Code generation and execution
            CodeGenerator::generateAndRun(functionAST);
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }