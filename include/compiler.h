#pragma once

#include <string>
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "ast.h"
//include "code_generator.h"

class Compiler {
private:
    std::string input;
    Lexer lexer;

public:
    // Constructor
    Compiler(const std::string& sourceCode);

    // Compile method with stages
    int compile();

    // Separate compilation stages for flexibility
    std::vector<Token> lexicalAnalysis();
    std::unique_ptr<ASTNode> syntaxAnalysis(const std::vector<Token>& tokens);
    void semanticAnalysis(const ASTNode* ast);
    void generateCode(const ASTNode* ast);

    // Error handling and reporting
    void reportError(const std::string& stage, const std::string& message);
};
