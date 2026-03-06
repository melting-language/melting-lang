#pragma once

#include <string>
#include <vector>

enum class TokenType {
    Number, String, True, False, Identifier,
    Plus, Minus, Star, Slash,
    LParen, RParen,
    Assign, Eq, Ne, Lt, Le, Gt, Ge, And, Or, Not, Colon, Arrow,
    Print, Let, If, Else, For, Foreach, In, While, Return, Try, Catch, Throw,
    Import,
    Class, Method, This, Fn,
    Dot, Comma,
    LBracket, RBracket,
    LBrace, RBrace, Semicolon,
    Eof
};

struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType t, const std::string& v, int l) : type(t), value(v), line(l) {}
};

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t pos_;
    int line_;
    char peek();
    char advance();
    bool match(char c);
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token number();
    Token string_();
    Token identifier();
};
