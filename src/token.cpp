#include "token.h"

Token::Token(TokenType t, const std::string& v, int l, int c)
    : type(t), value(v), line(l), column(c) {}