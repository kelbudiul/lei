#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

Token Parser::currentToken() {
    return tokens[index];
}

Token Parser::peek() {
    if (index + 1 >= tokens.size()) return tokens.back();
    return tokens[index + 1];
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        index++;
        return true;
    }
    return false;
}

void Parser::consume(TokenType type) {
    if (!check(type)) {
        throw std::runtime_error("Expected token type " + std::to_string(type) + 
                               " but got " + std::to_string(currentToken().type));
    }
    index++;
}

bool Parser::check(TokenType type) {
    return currentToken().type == type;
}

std::unique_ptr<FunctionAST> Parser::parse() {
    consume(FN);
    auto returnType = currentToken().value;
    consume(IDENTIFIER);
    auto name = currentToken().value;
    consume(IDENTIFIER);
    
    consume(LPAREN);
    auto params = parseParameters();
    consume(RPAREN);
    
    auto body = parseBlock();
    
    return std::make_unique<FunctionAST>(name, returnType, std::move(params), std::move(body));
}

std::vector<std::pair<std::string, std::string>> Parser::parseParameters() {
    std::vector<std::pair<std::string, std::string>> params;
    
    if (!check(RPAREN)) {
        do {
            auto paramName = currentToken().value;
            consume(IDENTIFIER);
            consume(COLON);
            auto paramType = currentToken().value;
            consume(IDENTIFIER);
            
            params.emplace_back(paramName, paramType);
        } while (match(COMMA));
    }
    
    return params;
}

std::unique_ptr<BlockAST> Parser::parseBlock() {
    consume(LBRACE);
    auto block = std::make_unique<BlockAST>();
    
    while (!check(RBRACE) && !check(END)) {
        block->statements.push_back(parseStatement());
    }
    
    consume(RBRACE);
    return block;
}

std::unique_ptr<StatementAST> Parser::parseStatement() {
    if (check(IF)) return parseIf();
    if (check(WHILE)) return parseWhile();
    if (check(RETURN)) return parseReturn();
    if (check(VAR)) return parseVarDecl();
    
    // Expression statement
    auto expr = parseExpression();
    consume(SEMICOLON);
    return std::make_unique<ExpressionStatementAST>(std::move(expr));
}

std::unique_ptr<ExpressionAST> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<ExpressionAST> Parser::parseAssignment() {
    auto expr = parseLogicOr();
    
    if (match(EQUALS) || match(PLUS_EQUALS) || match(MINUS_EQUALS) || 
        match(STAR_EQUALS) || match(SLASH_EQUALS)) {
        auto op = tokens[index - 1].value;
        auto value = parseAssignment();
        auto varExpr = dynamic_cast<VariableExprAST*>(expr.get());
        
        if (!varExpr) {
            throw std::runtime_error("Invalid assignment target");
        }
        
        return std::make_unique<AssignmentExprAST>(varExpr->name, std::move(value), op);
    }
    
    return expr;
}