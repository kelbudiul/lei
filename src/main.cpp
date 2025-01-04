#include "compiler.h"
#include "source_reader.h"
#include <llvm/Support/TargetSelect.h>
#include <iostream>
#include <sstream>  // Added for istringstream
#include "CLI11.hpp"

// Forward declaration of helper function
void printErrorsWithContext(const std::vector<ErrorHandler::Error>& errors, const std::string& sourceCode);

int main(int argc, char* argv[]) {
    // LLVM initialization
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    CLI::App app{"Lei Compiler"};

    std::string inputPath;
    app.add_option("input", inputPath, "Input source file")
       ->required()
       ->check(CLI::ExistingFile);

    std::string outputPath = "output.ll";
    app.add_option("-o,--output", outputPath, "Output path for generated LLVM IR");

    bool execute = false;
    app.add_flag("-e,--execute", execute, "Directly execute the generated LLVM IR");

    bool printAST = false;
    app.add_flag("--print-ast", printAST, "Print the abstract syntax tree (AST)");

    bool printSymbolTable = false; 
    app.add_flag("--print-sp", printSymbolTable, "Print the symbol table");

    bool printIR = false;
    app.add_flag("--print-ir", printIR, "Print the LLVM IR");

    CLI11_PARSE(app, argc, argv);
    
    
    try {
        // Read source file
        std::string sourceCode = Lei::SourceReader::readSourceFile(inputPath);
        if (sourceCode.empty()) {
            std::cerr << "Error: Unable to read source file: " << inputPath << std::endl;
            return EXIT_FAILURE;
        }

        // Create compiler and compile
        Compiler compiler;
        
        if (execute) {
            if (!compiler.execute(sourceCode, printAST, printSymbolTable, printIR)) {
                if (compiler.errorHandler.hasErrors(ErrorLevel::CODEGEN)) {
                    printErrorsWithContext(compiler.errorHandler.getErrors(ErrorLevel::CODEGEN), sourceCode);
                }
                return EXIT_FAILURE;
            }
        } else {
            if (!compiler.compile(sourceCode, outputPath, printAST, printSymbolTable, printIR)) {
                if (compiler.errorHandler.hasErrors(ErrorLevel::LEXICAL)) {
                    std::cerr << "\nLexical Analysis Failed\n";
                    printErrorsWithContext(compiler.errorHandler.getErrors(ErrorLevel::LEXICAL), sourceCode);
                }
                if (compiler.errorHandler.hasErrors(ErrorLevel::SYNTAX)) {
                    std::cerr << "\nParsing Failed\n";
                    printErrorsWithContext(compiler.errorHandler.getErrors(ErrorLevel::SYNTAX), sourceCode);
                }
                if (compiler.errorHandler.hasErrors(ErrorLevel::SEMANTIC)) {
                    std::cerr << "\nSemantic Analysis Failed\n";
                    printErrorsWithContext(compiler.errorHandler.getErrors(ErrorLevel::SEMANTIC), sourceCode);
                }
                if (compiler.errorHandler.hasErrors(ErrorLevel::CODEGEN)) {
                    std::cerr << "\nCode Generation Failed\n";
                    printErrorsWithContext(compiler.errorHandler.getErrors(ErrorLevel::CODEGEN), sourceCode);
                }
                return EXIT_FAILURE;
            }
            std::cout << "Compilation successful. Output written to: " << outputPath << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

void printErrorsWithContext(const std::vector<ErrorHandler::Error>& errors, const std::string& sourceCode) {
    std::istringstream sourceStream(sourceCode);
    std::vector<std::string> lines;
    std::string line;
    
    // Read all lines into vector for easier access
    while (std::getline(sourceStream, line)) {
        lines.push_back(line);
    }
    
    for (const auto& error : errors) {
        std::cerr << "\n" << ErrorHandler::getLevelString(error.level)
                  << " at line " << error.line << ", column " << error.column << ":\n";
        
        // Print the line where error occurred
        if (error.line > 0 && error.line <= lines.size()) {
            std::cerr << lines[error.line - 1] << "\n";
            // Print caret pointing to error position
            std::cerr << std::string(error.column - 1, ' ') << "^\n";
        }
        
        std::cerr << error.message << "\n";
        
        // Print context if available
        if (!error.sourceSnippet.empty()) {
            std::cerr << "Context:\n" << error.sourceSnippet << "\n";
        }
    }
}