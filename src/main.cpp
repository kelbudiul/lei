#include "../include/common.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_visitor.h"
#include "symbol_table.h"
#include "codegen_visitor.h"

#include "llvm/Support/TargetSelect.h"
#include "source_reader.h"

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

        std::cout << "Source file contents:\n" << code << std::endl;

        // Lexing and Parsing
        Lexer lexer(code);
        Parser parser(lexer.tokenize());
        auto ast = parser.parse();

        // Semantic Analysis
        SymbolTable symbolTable;
        SemanticVisitor semanticVisitor(symbolTable);
        ast->accept(&semanticVisitor);

        if (semanticVisitor.hasErrors()) {
            for (const auto& error : semanticVisitor.getErrors()) {
                std::cerr << error << std::endl;
            }
            return EXIT_FAILURE;
        }

        // Code Generation
        llvm::LLVMContext context;
        llvm::IRBuilder<> builder(context);
        llvm::Module module("my_module", context);

        CodeGenerationVisitor codeGenVisitor(context, builder, module);
        ast->accept(&codeGenVisitor);

        // Successful completion
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}