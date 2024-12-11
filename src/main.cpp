#include "lexer.h"
#include "source_reader.h"
//#include "parser.h"
//#include "semantic_visitor.h"
//#include "symbol_table.h"
//#include "codegen_visitor.h"


#include "error_handler.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) {
    // LLVM initialization
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    // Check for sufficient command-line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [output_path]" << std::endl;
        return EXIT_FAILURE;
    }

    // Determine output path
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
            
            // Print the source code context for each error
            for (const auto& error : lexicalErrors) {
                // Get the line of code where the error occurred
                std::istringstream codeStream(code);
                std::string line;
                int currentLine = 1;
                while (std::getline(codeStream, line) && currentLine < error.line) {
                    currentLine++;
                }

                std::cerr << "\nError at line " << error.line << ", column " << error.column << ":\n";
                std::cerr << line << "\n";
                // Print the error pointer
                std::cerr << std::string(error.column - 1, ' ') << "^\n";
                std::cerr << error.message << "\n";
            }
            return EXIT_FAILURE;
        }

        // // Debug: Print tokens if needed
        // #ifdef DEBUG_MODE
        // std::cout << "\nTokens:\n";
        // for (const auto& token : tokens) {
        //     std::cout << "Type: " << static_cast<int>(token.type) 
        //               << ", Value: '" << token.value 
        //               << "', Line: " << token.line 
        //               << ", Column: " << token.column << "\n";
        // }
        // #endif

        // Parsing
        // Parser parser(tokens);
        // auto ast = parser.parse();

        // if (ErrorHandler::instance().hasErrors(ErrorLevel::SYNTAX)) {
        //     std::cerr << "\nParsing Failed\n";
        //     auto syntaxErrors = ErrorHandler::instance().getErrors(ErrorLevel::SYNTAX);
        //     // Handle syntax errors similarly to lexical errors...
        //     return EXIT_FAILURE;
        // }

        // // Semantic Analysis
        // SymbolTable symbolTable;
        // SemanticVisitor semanticVisitor(symbolTable);
        // ast->accept(&semanticVisitor);

        // if (ErrorHandler::instance().hasErrors(ErrorLevel::SEMANTIC)) {
        //     std::cerr << "\nSemantic Analysis Failed\n";
        //     auto semanticErrors = ErrorHandler::instance().getErrors(ErrorLevel::SEMANTIC);
        //     // Handle semantic errors...
        //     return EXIT_FAILURE;
        // }

        // // Code Generation
        // llvm::LLVMContext context;
        // llvm::IRBuilder<> builder(context);
        // llvm::Module module("my_module", context);

        // CodeGenerationVisitor codeGenVisitor(context, builder, module);
        // ast->accept(&codeGenVisitor);

        // if (ErrorHandler::instance().hasErrors(ErrorLevel::CODEGEN)) {
        //     std::cerr << "\nCode Generation Failed\n";
        //     auto codegenErrors = ErrorHandler::instance().getErrors(ErrorLevel::CODEGEN);
        //     // Handle codegen errors...
        //     return EXIT_FAILURE;
        // }

        // std::cout << "\nCompilation successful!\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}