#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include <sstream>


enum class TokenType {
    IDENTIFIER,
    KEYWORD,
    LITERAL_INTEGER,
    LITERAL_STRING,
    OPERATOR,
    PUNCTUATION,
    COMMENT,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

class LeiTokenizer {
private:
    std::vector<std::string> sourceLines;
    std::vector<Token> tokens;

    // Helper method to classify tokens
    TokenType classifyToken(const std::string& token) {
        // Check for keywords
        std::vector<std::string> keywords = {"fn", "let", "if", "else", "return"};
        if (std::find(keywords.begin(), keywords.end(), token) != keywords.end()) {
            return TokenType::KEYWORD;
        }

        // Check for integer literals
        if (std::regex_match(token, std::regex("^-?[0-9]+$"))) {
            return TokenType::LITERAL_INTEGER;
        }

        // Check for string literals
        if (std::regex_match(token, std::regex("^\".*\"$"))) {
            return TokenType::LITERAL_STRING;
        }

        // Check for identifiers
        if (std::regex_match(token, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            return TokenType::IDENTIFIER;
        }

        // Operators and punctuation
        std::vector<std::string> operators = {"+", "-", "*", "/", "=", "==", "!="};
        std::vector<std::string> punctuation = {"{", "}", "(", ")", ";", ":"};
        
        if (std::find(operators.begin(), operators.end(), token) != operators.end()) {
            return TokenType::OPERATOR;
        }

        if (std::find(punctuation.begin(), punctuation.end(), token) != punctuation.end()) {
            return TokenType::PUNCTUATION;
        }

        return TokenType::UNKNOWN;
    }

public:
    LeiTokenizer(const std::vector<std::string>& lines) : sourceLines(lines) {}

    void tokenize() {
        tokens.clear();
        for (int lineNum = 0; lineNum < sourceLines.size(); ++lineNum) {
            std::string line = sourceLines[lineNum];
            std::istringstream iss(line);
            std::string token;
            int column = 0;

            while (iss >> token) {
                // Skip comments
                if (token.find("//")) break;

                Token t;
                t.type = classifyToken(token);
                t.value = token;
                t.line = lineNum + 1;
                t.column = column++;

                tokens.push_back(t);
            }
        }
    }

    void printTokens() {
        for (const auto& token : tokens) {
            std::cout << "Type: " << static_cast<int>(token.type) 
                      << ", Value: " << token.value 
                      << ", Line: " << token.line 
                      << ", Column: " << token.column << std::endl;
        }
    }

    const std::vector<Token>& getTokens() const {
        return tokens;
    }
};

// int main() {
//     // Example usage
//     std::vector<std::string> sourceLines = {
//         "fn main() {",
//         "    let x = 42;",
//         "    return x;",
//         "}"
//     };

//     LeiTokenizer tokenizer(sourceLines);
//     tokenizer.tokenize();
//     tokenizer.printTokens();

//     return 0;
// }