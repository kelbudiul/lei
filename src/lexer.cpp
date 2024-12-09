#include "Lexer.h"
#include <cctype>
#include <stdexcept>

// Token constructor
Token::Token(TokenType t, const std::string& v, int l, int c)
    : type(t), value(v), line(l), column(c) {}

// Lexer constructor
Lexer::Lexer(const std::string& code)
    : input(code), pos(0), line(1), column(1) {}

// Advance position and handle line/column tracking
void Lexer::advance() {
    if (input[pos] == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    pos++;
}

// Tokenize method
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < input.size()) {
        if (std::isspace(input[pos])) {
            advance();
            continue;
        }

        int startLine = line;
        int startColumn = column;

        if (std::isalpha(input[pos])) {
            std::string word;
            while (pos < input.size() && std::isalnum(input[pos])) {
                word += input[pos];
                advance();
            }
            if (word == "fn") tokens.emplace_back(FN, word, startLine, startColumn);
            else if (word == "int") tokens.emplace_back(INT, word, startLine, startColumn);
            else if (word == "return") tokens.emplace_back(RETURN, word, startLine, startColumn);
            else tokens.emplace_back(IDENTIFIER, word, startLine, startColumn);
        } else if (std::isdigit(input[pos])) {
            std::string num;
            while (pos < input.size() && std::isdigit(input[pos])) {
                num += input[pos];
                advance();
            }
            tokens.emplace_back(NUMBER, num, startLine, startColumn);
        } else {
            char currentChar = input[pos];
            advance();
            switch (currentChar) {
                case '{': tokens.emplace_back(LBRACE, "{", startLine, startColumn); break;
                case '}': tokens.emplace_back(RBRACE, "}", startLine, startColumn); break;
                case '(': tokens.emplace_back(LPAREN, "(", startLine, startColumn); break;
                case ')': tokens.emplace_back(RPAREN, ")", startLine, startColumn); break;
                case ';': tokens.emplace_back(SEMICOLON, ";", startLine, startColumn); break;
                default:
                    throw std::runtime_error("Unknown character");
            }
        }
    }

    tokens.emplace_back(END, "", line, column);
    return tokens;
}
