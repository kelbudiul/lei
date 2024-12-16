#include "lexer.h"
#include "source_reader.h"
#include "parser.h"
#include "error_handler.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>
#include <sstream>
#include "ast_printer.h"
#include "semantic_visitor.h"

// Forward declaration of helper function
void printErrorsWithContext(const std::vector<ErrorHandler::Error>& errors, const std::string& sourceCode);

int main(int argc, char* argv[]) {
    // LLVM initialization
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [output_path]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string path = (argc == 3) ? argv[2] : ".";

    try {
        // Read source file
        std::string filename = argv[1];
        std::string code = Lei::SourceReader::readSourceFile(filename);
        
        if (code.empty()) {
            std::cerr << "Error: Unable to read source file: " << filename << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Processing source file: " << filename << std::endl;
        
        // Lexical Analysis
        Lexer lexer(code);
        auto tokens = lexer.tokenize();

        // Check for lexical errors
        if (ErrorHandler::instance().hasErrors(ErrorLevel::LEXICAL)) {
            std::cerr << "\nLexical Analysis Failed\n";
            auto lexicalErrors = ErrorHandler::instance().getErrors(ErrorLevel::LEXICAL);
            std::cerr << "Found " << lexicalErrors.size() << " lexical error(s):\n";
            printErrorsWithContext(lexicalErrors, code);
            return EXIT_FAILURE;
        }

        // Parsing
        Parser parser(tokens);
        auto ast = parser.parse();

        // Check for syntax errors
        if (ErrorHandler::instance().hasErrors(ErrorLevel::SYNTAX)) {
            std::cerr << "\nParsing Failed\n";
            auto syntaxErrors = ErrorHandler::instance().getErrors(ErrorLevel::SYNTAX);
            std::cerr << "Found " << syntaxErrors.size() << " syntax error(s):\n";
            printErrorsWithContext(syntaxErrors, code);
            return EXIT_FAILURE;
        }

        // Only print AST if parsing was successful
        if (ast) {
            ASTPrinter printer;
            std::string astDump = printer.print(ast.get());
            std::cout << "\nAST Structure:\n" << astDump << std::endl;
        }

        SemanticAnalyzer analyzer;
        bool success = analyzer.analyze(ast.get());

        if (!success)
        {
            std::cerr << "\nSemantic Analysis Failed\n";
            auto semanticErrors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
            std::cerr << "Found " << semanticErrors.size() << " syntax error(s):\n";
            printErrorsWithContext(semanticErrors, code);
            return EXIT_FAILURE;
        }
        

        return EXIT_SUCCESS;

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