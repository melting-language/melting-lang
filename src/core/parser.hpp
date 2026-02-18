#pragma once

#include "lexer.hpp"
#include "ast.hpp"
#include <vector>
#include <memory>

class Parser {
public:
    Parser(std::vector<Token> tokens);
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_;

    const Token& peek();
    const Token& advance();
    bool check(TokenType type);
    bool match(TokenType type);

    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> letStatement();
    std::unique_ptr<Stmt> importStatement();
    std::unique_ptr<Stmt> classStatement();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> forStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> tryStatement();
    std::unique_ptr<Stmt> throwStatement();
    std::unique_ptr<Stmt> blockStatement();
    std::unique_ptr<Stmt> exprStatement();

    MethodDecl methodDecl();

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> logicalNot();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> postfix(std::unique_ptr<Expr> expr);
    std::vector<std::unique_ptr<Expr>> exprList();
};
