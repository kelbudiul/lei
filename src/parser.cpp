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
    advance();
    
    while (!isAtEnd()) {
        // Return on statement boundaries
        if (previous().type == SEMICOLON) return;
        if (previous().type == RBRACE) return;
        
        switch (peek().type) {
            case FN:
            case VAR:
            case IF:
            case WHILE:
            case RETURN:
            case LBRACE:  // Add LBRACE as a synchronization point
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<Program> Parser::parse() {
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    Token startToken = peek();
    
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
    
    return std::make_unique<Program>(std::move(functions), startToken);
}

Type Parser::parseType() {
    std::string typeName;
    bool isArray = false;
    int arraySize = -1;
    
    if (match(INT)) typeName = "int";
    else if (match(FLOAT_TYPE)) typeName = "float";
    else if (match(BOOL_TYPE)) typeName = "bool";
    else if (match(STRING_TYPE)) typeName = "str";
    else if (match(VOID)) typeName = "void";
    else {
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            "Expected type specifier"
        );
        return Type("error");
    }
    
    if (match(LBRACKET)) {
        isArray = true;
        if (match(NUMBER)) {
            arraySize = std::stoi(previous().value);
        }
        consume(RBRACKET, "Expected ']' after array size");
    }
    
    return Type(typeName, isArray, arraySize);
}

std::unique_ptr<FunctionDecl> Parser::parseFunction() {
    Token fnToken = previous();
    Type returnType = parseType();
    Token name = consume(IDENTIFIER, "Expected function name");
    
    consume(LPAREN, "Expected '(' after function name");
    
    std::vector<Parameter> parameters;
    if (!check(RPAREN)) {
        parameters = parseParameters();
    }
    
    consume(RPAREN, "Expected ')' after parameters");
    
    // Parse the function body as a block
    auto body = parseBlock();
    if (!body) {
        ErrorHandler::instance().error(
            ErrorLevel::SYNTAX,
            peek().line,
            peek().column,
            "Invalid function body"
        );
        return nullptr;
    }
    
    return std::make_unique<FunctionDecl>(name, returnType, std::move(parameters),
                                        std::move(body), fnToken);
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

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    Token braceToken = consume(LBRACE, "Expected '{' before block");
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(RBRACE) && !isAtEnd()) {
        if (auto stmt = parseStatement()) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(RBRACE, "Expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements), braceToken);
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    try {
        if (match(VAR)) return parseVarDecl();
        if (match(IF)) return parseIfStmt();
        if (match(WHILE)) return parseWhileStmt();
        if (match(RETURN)) return parseReturnStmt();
        if (match(LBRACE)) {
            // Create a block statement directly from a brace
            Token braceToken = previous();
            std::vector<std::unique_ptr<Stmt>> statements;
            
            while (!check(RBRACE) && !isAtEnd()) {
                if (auto stmt = parseStatement()) {
                    statements.push_back(std::move(stmt));
                }
            }
            
            consume(RBRACE, "Expected '}' after block");
            return std::make_unique<BlockStmt>(std::move(statements), braceToken);
        }
        
        // Handle type declarations that shouldn't appear here
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
        
        return parseExprStmt();
    } catch (const std::exception& e) {
        synchronize();
        return nullptr;
    }
}


std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
    Token varToken = previous();
    Token name = consume(IDENTIFIER, "Expected variable name");
    consume(COLON, "Expected ':' after variable name");
    
    Type type = parseType();
    
    if (type.name == "void") {
    ErrorHandler::instance().error(
        ErrorLevel::SYNTAX,
        name.line,
        name.column,
        "Variables cannot have 'void' type"
    );
    return nullptr;
}
    std::unique_ptr<Expr> initializer = nullptr;
    
    if (match(EQUALS)) {
        initializer = parseExpression();
    }
    
    consume(SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclStmt>(name, type, std::move(initializer), varToken);
}

std::unique_ptr<IfStmt> Parser::parseIfStmt() {
    Token ifToken = previous();
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
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                   std::move(elseBranch), ifToken);
}

std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
    Token whileToken = previous();
    auto condition = parseExpression();
    auto body = parseBlock();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), whileToken);
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
    Token returnToken = previous();
    std::unique_ptr<Expr> value = nullptr;
    
    if (!check(SEMICOLON)) {
        value = parseExpression();
    }
    
    consume(SEMICOLON, "Expected ';' after return statement");
    return std::make_unique<ReturnStmt>(returnToken, std::move(value));
}

std::unique_ptr<ExprStmt> Parser::parseExprStmt() {
    Token startToken = peek();
    auto expr = parseExpression();
    
    if (!expr) {
        synchronize();
        return nullptr;
    }
    
    consume(SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr), startToken);
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    
    if (match(EQUALS) || match(PLUS_EQUALS) || match(MINUS_EQUALS) ||
        match(STAR_EQUALS) || match(SLASH_EQUALS)) {
        Token op = previous();
        auto value = parseAssignment();
        
        if (dynamic_cast<VariableExpr*>(expr.get()) ||
            dynamic_cast<ArrayAccessExpr*>(expr.get())) {
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
            if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
                auto arguments = parseArguments();
                consume(RPAREN, "Expected ')' after arguments");
                expr = std::make_unique<CallExpr>(var->name, std::move(arguments));
            } else {
                ErrorHandler::instance().error(
                    ErrorLevel::SYNTAX,
                    previous().line,
                    previous().column,
                    "Expected function name before '('"
                );
                break;
            }
        } else if (match(LBRACKET)) {
            Token bracketToken = previous();
            auto index = parseExpression();
            consume(RBRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(index), bracketToken);
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
        if (match(INT) || match(FLOAT_TYPE) || match(BOOL_TYPE) || match(STRING_TYPE)) {
            return parseTypeExpression();
        }
        if (match(LPAREN)) {
            auto expr = parseExpression();
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
    Token braceToken = previous();
    std::vector<std::unique_ptr<Expr>> elements;
    
    if (!check(RBRACE)) {
        do {
            if (auto element = parseExpression()) {
                elements.push_back(std::move(element));
            }
        } while (match(COMMA));
    }
    
    consume(RBRACE, "Expected '}' after array elements");
    return std::make_unique<ArrayInitExpr>(std::move(elements), braceToken);
}

std::vector<std::unique_ptr<Expr>> Parser::parseArguments() {
    std::vector<std::unique_ptr<Expr>> arguments;
    
    if (!check(RPAREN)) {
        do {
            if (auto arg = parseExpression()) {
                arguments.push_back(std::move(arg));
            }
        } while (match(COMMA));
    }
    
    return arguments;
    }

std::unique_ptr<ArrayAllocExpr> Parser::parseArrayAllocation(const Type& elementType) {
    Token newToken = previous();
    auto size = parseExpression();
    return std::make_unique<ArrayAllocExpr>(elementType, std::move(size), newToken);
}

std::unique_ptr<Expr> Parser::parseTypeExpression() {
    Token typeToken = previous();
    bool isArray = false;
    
    if (match(LBRACKET)) {
        isArray = true;
        consume(RBRACKET, "Expected ']' after array type");
    }
    
    return std::make_unique<TypeExpr>(Type(typeToken.value, isArray), typeToken);
}

std::unique_ptr<Expr> Parser::createBinaryExpr(std::unique_ptr<Expr> left, 
                                             const Token& op,
                                             std::unique_ptr<Expr> right) {
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
}

std::unique_ptr<Expr> Parser::createCallExpr(const Token& name,
                                           std::vector<std::unique_ptr<Expr>> args) {
    return std::make_unique<CallExpr>(name, std::move(args));
}

std::unique_ptr<Expr> Parser::createAssignExpr(std::unique_ptr<Expr> target,
                                             const Token& op,
                                             std::unique_ptr<Expr> value) {
    return std::make_unique<AssignExpr>(std::move(target), op, std::move(value));
}

// Helper method to report parsing errors with more context
void Parser::error(const std::string& message) {
    ErrorHandler::instance().error(
        ErrorLevel::SYNTAX,
        peek().line,
        peek().column,
        message
    );
}

// Helper method to report errors at a specific token
void Parser::errorAt(const Token& token, const std::string& message) {
    ErrorHandler::instance().error(
        ErrorLevel::SYNTAX,
        token.line,
        token.column,
        message
    );
}

// Helper method to check for end of expression
bool Parser::isAtExpressionEnd() const {
    return check(SEMICOLON) || check(COMMA) || check(RPAREN) || 
           check(RBRACE) || check(RBRACKET) || isAtEnd();
}

// Helper method to check if current token could start an expression
bool Parser::isExpressionStart() const {
    switch (peek().type) {
        case IDENTIFIER:
        case NUMBER:
        case FLOAT_LITERAL:
        case STRING_LITERAL:
        case BOOL_LITERAL:
        case LPAREN:
        case LBRACE:
        case NOT:
        case MINUS:
            return true;
        default:
            return false;
    }
}

// Helper method for validating statement terminators
void Parser::expectStatementEnd(const std::string& context) {
    if (!check(SEMICOLON) && !check(RBRACE)) {
        std::string message = "Expected ';' or '}' after " + context;
        error(message);
    }
}

// Helper method for checking balanced delimiters
void Parser::checkBalancedDelimiter(TokenType opening, TokenType closing,
                                  const std::string& context) {
    int depth = 1;
    size_t startPos = current;
    
    while (!isAtEnd() && depth > 0) {
        if (peek().type == opening) depth++;
        if (peek().type == closing) depth--;
        advance();
    }
    
    if (depth > 0) {
        current = startPos;
        std::string message = "Unmatched " + context;
        error(message);
    }
}

// Helper method to parse a sequence of items separated by commas
template<typename T>
std::vector<T> Parser::parseCommaSequence(std::function<T()> parseItem,
                                        TokenType endToken,
                                        const std::string& endTokenStr) {
    std::vector<T> items;
    
    if (!check(endToken)) {
        do {
            if (auto item = parseItem()) {
                items.push_back(std::move(item));
            }
        } while (match(COMMA));
    }
    
    consume(endToken, "Expected '" + endTokenStr + "'");
    return items;
}