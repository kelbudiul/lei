#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <stdexcept>
#include <unordered_map>

// Common types and enums used across the project
enum TokenType {
    FN, INT, IDENTIFIER, RETURN, NUMBER, 
    LBRACE, RBRACE, LPAREN, RPAREN, SEMICOLON, END
};

// Variant type for return values
using ReturnValue = std::variant<int, float, std::string>;


#endif // COMMON_H