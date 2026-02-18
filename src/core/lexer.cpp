#include "lexer.hpp"
#include <cctype>

Lexer::Lexer(const std::string& source) : source_(source), pos_(0), line_(1) {}

char Lexer::peek() {
    return pos_ < source_.size() ? source_[pos_] : '\0';
}

char Lexer::advance() {
    if (pos_ >= source_.size()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') line_++;
    return c;
}

bool Lexer::match(char c) {
    if (peek() == c) { advance(); return true; }
    return false;
}

void Lexer::skipWhitespace() {
    while (isspace(peek())) advance();
}

void Lexer::skipLineComment() {
    while (peek() != '\0' && peek() != '\n') advance();
}

void Lexer::skipBlockComment() {
    advance(); // consume *
    while (peek() != '\0') {
        if (peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            advance();
            advance();
            return;
        }
        advance();
    }
}

Token Lexer::number() {
    int start = line_;
    std::string value;
    while (isdigit(peek()) || peek() == '.') value += advance();
    return Token(TokenType::Number, value, start);
}

Token Lexer::string_() {
    int start = line_;
    advance(); // skip opening "
    std::string value;
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '\\') { advance(); value += advance(); }
        else value += advance();
    }
    advance(); // skip closing "
    return Token(TokenType::String, value, start);
}

Token Lexer::identifier() {
    int start = line_;
    std::string value;
    while (isalnum(peek()) || peek() == '_') value += advance();

    if (value == "print") return Token(TokenType::Print, value, start);
    if (value == "let") return Token(TokenType::Let, value, start);
    if (value == "if") return Token(TokenType::If, value, start);
    if (value == "else") return Token(TokenType::Else, value, start);
    if (value == "for") return Token(TokenType::For, value, start);
    if (value == "while") return Token(TokenType::While, value, start);
    if (value == "class") return Token(TokenType::Class, value, start);
    if (value == "method") return Token(TokenType::Method, value, start);
    if (value == "this") return Token(TokenType::This, value, start);
    if (value == "import") return Token(TokenType::Import, value, start);
    if (value == "return") return Token(TokenType::Return, value, start);
    if (value == "try") return Token(TokenType::Try, value, start);
    if (value == "catch") return Token(TokenType::Catch, value, start);
    if (value == "throw") return Token(TokenType::Throw, value, start);
    if (value == "true") return Token(TokenType::True, value, start);
    if (value == "false") return Token(TokenType::False, value, start);
    if (value == "fn") return Token(TokenType::Fn, value, start);

    return Token(TokenType::Identifier, value, start);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (peek() != '\0') {
        skipWhitespace();
        if (peek() == '\0') break;

        char c = peek();
        int line = line_;

        if (isdigit(c)) { tokens.push_back(number()); continue; }
        if (c == '"') { tokens.push_back(string_()); continue; }
        if (isalpha(c) || c == '_') { tokens.push_back(identifier()); continue; }

        if (c == '+') { advance(); tokens.push_back(Token(TokenType::Plus, "+", line)); continue; }
        if (c == '-') { advance(); tokens.push_back(Token(TokenType::Minus, "-", line)); continue; }
        if (c == '*') { advance(); tokens.push_back(Token(TokenType::Star, "*", line)); continue; }
        if (c == '/') {
            advance();
            if (peek() == '/') { advance(); skipLineComment(); continue; }
            if (peek() == '*') { skipBlockComment(); continue; }
            tokens.push_back(Token(TokenType::Slash, "/", line));
            continue;
        }
        if (c == '(') { advance(); tokens.push_back(Token(TokenType::LParen, "(", line)); continue; }
        if (c == ')') { advance(); tokens.push_back(Token(TokenType::RParen, ")", line)); continue; }
        if (c == '[') { advance(); tokens.push_back(Token(TokenType::LBracket, "[", line)); continue; }
        if (c == ']') { advance(); tokens.push_back(Token(TokenType::RBracket, "]", line)); continue; }
        if (c == '.') { advance(); tokens.push_back(Token(TokenType::Dot, ".", line)); continue; }
        if (c == ',') { advance(); tokens.push_back(Token(TokenType::Comma, ",", line)); continue; }
        if (c == '{') { advance(); tokens.push_back(Token(TokenType::LBrace, "{", line)); continue; }
        if (c == '}') { advance(); tokens.push_back(Token(TokenType::RBrace, "}", line)); continue; }
        if (c == ';') { advance(); tokens.push_back(Token(TokenType::Semicolon, ";", line)); continue; }

        if (c == '=') {
            advance();
            if (match('=')) tokens.push_back(Token(TokenType::Eq, "==", line));
            else tokens.push_back(Token(TokenType::Assign, "=", line));
            continue;
        }
        if (c == '!') {
            advance();
            if (match('=')) tokens.push_back(Token(TokenType::Ne, "!=", line));
            else tokens.push_back(Token(TokenType::Not, "!", line));
            continue;
        }
        if (c == '&' && pos_ < source_.size() && source_[pos_] == '&') {
            advance(); advance();
            tokens.push_back(Token(TokenType::And, "&&", line));
            continue;
        }
        if (c == '|' && pos_ < source_.size() && source_[pos_] == '|') {
            advance(); advance();
            tokens.push_back(Token(TokenType::Or, "||", line));
            continue;
        }
        if (c == '<') {
            advance();
            if (match('=')) tokens.push_back(Token(TokenType::Le, "<=", line));
            else tokens.push_back(Token(TokenType::Lt, "<", line));
            continue;
        }
        if (c == '>') {
            advance();
            if (match('=')) tokens.push_back(Token(TokenType::Ge, ">=", line));
            else tokens.push_back(Token(TokenType::Gt, ">", line));
            continue;
        }

        advance(); // skip unknown char
    }
    tokens.push_back(Token(TokenType::Eof, "", line_));
    return tokens;
}
