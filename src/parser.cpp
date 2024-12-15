#include "parser.h"
#include <sstream>


Parser::Parser(const std::vector<Token>& tokens) 
    : tokens(tokens), current(0) {}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == END;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    ErrorHandler::instance().error(
        ErrorLevel::SYNTAX,
        peek().line,
        peek().column,
        message
    );
    
    // Return the current token to allow continued parsing
    return peek();
}

void Parser::synchronize() {
    debugToken("Synchronize Start");
    size_t startPos = current;
    
    // Always advance at least once
    advance();
    
    while (!isAtEnd()) {
        // Stop at statement boundaries
        if (previous().type == SEMICOLON) {
            debugToken("Synchronize Found Semicolon");
            return;
        }
        
        // Stop at statement keywords
        switch (peek().type) {
            case FN:
            case VAR:
            case IF:
            case WHILE:
            case RETURN:
            case RBRACE:  // End of block
                debugToken("Synchronize Found Statement Start");
                return;
        }
        
        advance();
    }
    
    // If we didn't advance, force advance
    if (current == startPos && !isAtEnd()) {
        advance();
        debugToken("Forced Advance in Synchronize");
    }
}

// Program parsing
std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    
    while (!isAtEnd()) {
        try {
            if (match(FN)) {
                functions.push_back(parseFunction());
            } else {
                ErrorHandler::instance().error(
                    ErrorLevel::SYNTAX,
                    peek().line,
                    peek().column,
                    "Expected function declaration"
                );
                synchronize();
            }
        } catch (const std::exception& e) {
            synchronize();
        }
    }
    
    return std::make_unique<Program>(std::move(functions));
}

// Type parsing
Type Parser::parseType() {
    // Parse basic type first
    std::string typeName;
    bool isArray = false;
    int arraySize = 0;
    
    // Get the base type
    if (match(INT)) typeName = "int";
    else if (match(FLOAT_TYPE)) typeName = "float";
    else if (match(BOOL_TYPE)) typeName = "bool";
    else if (match(STRING_TYPE)) typeName = "str";
    else {
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            "Expected type specifier"
        );
        return Type("error");
    }
    
    // Check if it's an array type
    if (match(LBRACKET)) {
        isArray = true;
        
        // Parse array size if specified
        if (match(NUMBER)) {
            arraySize = std::stoi(previous().value);
        } else {
            arraySize = -1; // Dynamic size
        }
        
        consume(RBRACKET, "Expected ']' after array size");
    }
    
    return Type(typeName, isArray, arraySize);
}

// Function parsing
std::unique_ptr<FunctionDecl> Parser::parseFunction() {
    // Parse return type
    Type returnType = parseType();
    
    // Parse function name
    Token name = consume(IDENTIFIER, "Expected function name");
    
    consume(LPAREN, "Expected '(' after function name");
    
    // Parse parameters
    std::vector<Parameter> parameters;
    if (!check(RPAREN)) {
        parameters = parseParameters();
    }
    
    consume(RPAREN, "Expected ')' after parameters");
    
    // Parse function body
    auto body = parseBlock();
    
    return std::make_unique<FunctionDecl>(name, returnType, std::move(parameters), std::move(body));
}

std::vector<Parameter> Parser::parseParameters() {
    std::vector<Parameter> parameters;
    
    do {
        Token name = consume(IDENTIFIER, "Expected parameter name");
        consume(COLON, "Expected ':' after parameter name");
        Type type = parseType();
        
        parameters.emplace_back(name, type);
    } while (match(COMMA));
    
    return parameters;
}

// Statement parsing
std::unique_ptr<BlockStmt> Parser::parseBlock() {
    consume(LBRACE, "Expected '{' before block");
    
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(RBRACE) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }
    
    consume(RBRACE, "Expected '}' after block");
    
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    debugToken("Parse Statement Start");
    
    try {
        // First check for known statement starts
        if (match(VAR)) return parseVarDecl();
        if (match(IF)) return parseIfStmt();
        if (match(WHILE)) return parseWhileStmt();
        if (match(RETURN)) return parseReturnStmt();
        if (match(LBRACE)) return parseBlock();
        
        // If we see a type token, this is likely an error
        if (peek().type == INT || peek().type == FLOAT_TYPE || 
            peek().type == BOOL_TYPE || peek().type == STRING_TYPE) {
            ErrorHandler::instance().error(
                ErrorLevel::SYNTAX,
                peek().line,
                peek().column,
                "Unexpected type name in statement position"
            );
            synchronize();
            return nullptr;
        }
        
        // Try to parse as expression statement
        return parseExprStmt();
    } catch (const std::exception& e) {
        debugToken("Statement Parse Error");
        synchronize();
        return nullptr;
    }
}


std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
    Token name = consume(IDENTIFIER, "Expected variable name");
    consume(COLON, "Expected ':' after variable name");
    
    Type type = parseType();
    
    std::unique_ptr<Expr> initializer = nullptr;
    if (match(EQUALS)) {
        initializer = parseExpression();
    }
    
    consume(SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VarDeclStmt>(name, type, std::move(initializer));
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    auto condition = parseExpression();
    auto thenBranch = parseBlock();
    
    std::unique_ptr<Stmt> elseBranch = nullptr;
    if (match(ELSE)) {
        if (match(IF)) {
            elseBranch = parseIfStmt();
        } else {
            elseBranch = parseBlock();
        }
    }
    
    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    auto condition = parseExpression();
    auto body = parseBlock();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    Token keyword = previous();
    std::unique_ptr<Expr> value = nullptr;
    
    if (!check(SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(SEMICOLON, "Expected ';' after return statement");
    
    return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

std::unique_ptr<ExprStmt> Parser::parseExprStmt() {
    debugToken("Parse Expression Statement Start");
    
    auto expr = parseExpression();
    if (!expr) {
        debugToken("Expression Parse Failed");
        synchronize();
        return nullptr;
    }
    
    if (!match(SEMICOLON)) {
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            "Expected ';' after expression"
        );
        synchronize();
    }
    
    debugToken("Parse Expression Statement End");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// Expression parsing
std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    
    if (match(EQUALS) || match(PLUS_EQUALS) || match(MINUS_EQUALS) ||
        match(STAR_EQUALS) || match(SLASH_EQUALS)) {
        Token op = previous();
        auto value = parseAssignment();
        
        // Verify LHS is a valid assignment target
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(std::move(expr), op, std::move(value));
        } else if (auto* arrayAccess = dynamic_cast<ArrayAccessExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(std::move(expr), op, std::move(value));
        }
        
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            op.line,
            op.column,
            "Invalid assignment target"
        );
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match(OR)) {
        Token op = previous();
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match(AND)) {
        Token op = previous();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseComparison();
    
    while (match(EQUALS_EQUALS) || match(NOT_EQUALS)) {
        Token op = previous();
        auto right = parseComparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseTerm();
    
    while (match(LESS) || match(LESS_EQUAL) ||
           match(GREATER) || match(GREATER_EQUAL)) {
        Token op = previous();
        auto right = parseTerm();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseFactor();
    
    while (match(PLUS) || match(MINUS)) {
        Token op = previous();
        auto right = parseFactor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor() {
    auto expr = parseUnary();
    
    while (match(STAR) || match(SLASH)) {
        Token op = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(NOT) || match(MINUS)) {
        Token op = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    
    return parseCall();
}

std::unique_ptr<Expr> Parser::parseCall() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(LPAREN)) {
            auto arguments = parseArguments();
            consume(RPAREN, "Expected ')' after arguments");
            
            if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
                expr = std::make_unique<CallExpr>(var->name, std::move(arguments));
            } else {
                ErrorHandler::instance().error(
                    ErrorLevel::SYNTAX,
                    previous().line,
                    previous().column,
                    "Expected function name before '('"
                );
            }
        } else if (match(LBRACKET)) {
            auto index = parseExpression();
            consume(RBRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    try {
        if (match(NUMBER)) {
            return std::make_unique<NumberExpr>(previous(), false);
        }
        if (match(FLOAT_LITERAL)) {
            return std::make_unique<NumberExpr>(previous(), true);
        }
        if (match(STRING_LITERAL)) {
            return std::make_unique<StringExpr>(previous());
        }
        if (match(BOOL_LITERAL)) {
            return std::make_unique<BoolExpr>(previous(), previous().value == "true");
        }
        if (match(IDENTIFIER)) {
            return std::make_unique<VariableExpr>(previous());
        }
        // Add handling for type names in expressions
        if (match(INT) || match(FLOAT_TYPE) || match(BOOL_TYPE) || match(STRING_TYPE)) {
            return parseTypeExpression();
        }
        if (match(LPAREN)) {
            auto expr = parseExpression();
            if (!expr) return nullptr;
            consume(RPAREN, "Expected ')' after expression");
            return expr;
        }
        if (match(LBRACE)) {
            return parseArrayInitializer();
        }
        
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            "Expected expression"
        );
        
        return nullptr;
    } catch (const std::exception& e) {
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            std::string("Error parsing expression: ") + e.what()
        );
        return nullptr;
    }
}

std::unique_ptr<ArrayInitExpr> Parser::parseArrayInitializer() {
    std::vector<std::unique_ptr<Expr>> elements;
    
    if (!check(RBRACE)) {
        do {
            elements.push_back(parseExpression());
        } while (match(COMMA));
    }
    
    consume(RBRACE, "Expected '}' after array elements");
    
    return std::make_unique<ArrayInitExpr>(std::move(elements));
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> arguments;
    
    if (!check(RPAREN)) {
        do {
            arguments.push_back(parseExpression());
        } while (match(COMMA));
    }
    
    return arguments;
}

std::unique_ptr<Expr> Parser::parseTypeExpression() {
    Token typeToken = previous();
    bool isArray = false;
    
    // Check for array type
    if (match(LBRACKET)) {
        isArray = true;
        consume(RBRACKET, "Expected ']' after array type");
    }
    
    // Create a TypeExpr node (you'll need to add this to AST)
    return std::make_unique<TypeExpr>(Type(typeToken.value, isArray));
}
