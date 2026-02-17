#include "parser.hpp"
#include <stdexcept>
#include <cstdlib>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), pos_(0) {}

const Token& Parser::peek() {
    return pos_ < tokens_.size() ? tokens_[pos_] : tokens_.back();
}

const Token& Parser::advance() {
    if (pos_ < tokens_.size()) return tokens_[pos_++];
    return tokens_.back();
}

bool Parser::check(TokenType type) {
    return peek().type != TokenType::Eof && peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (peek().type != TokenType::Eof) {
        statements.push_back(statement());
    }
    return statements;
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::Print)) return printStatement();
    if (match(TokenType::Let)) return letStatement();
    if (match(TokenType::Import)) return importStatement();
    if (match(TokenType::Class)) return classStatement();
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::Try)) return tryStatement();
    if (match(TokenType::Throw)) return throwStatement();
    if (match(TokenType::LBrace)) return blockStatement();
    return exprStatement();
}

std::unique_ptr<Stmt> Parser::printStatement() {
    auto expr = expression();
    match(TokenType::Semicolon);
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Stmt> Parser::letStatement() {
    if (peek().type != TokenType::Identifier)
        throw std::runtime_error("Expected variable name");
    std::string name = advance().value;
    if (!match(TokenType::Assign))
        throw std::runtime_error("Expected '=' after variable name");
    auto expr = expression();
    match(TokenType::Semicolon);
    return std::make_unique<LetStmt>(name, std::move(expr));
}

std::unique_ptr<Stmt> Parser::importStatement() {
    if (peek().type != TokenType::String)
        throw std::runtime_error("Expected string path after 'import'");
    std::string path = advance().value;
    if (!match(TokenType::Semicolon))
        throw std::runtime_error("Expected ';' after import path");
    return std::make_unique<ImportStmt>(std::move(path));
}

std::unique_ptr<Stmt> Parser::classStatement() {
    if (peek().type != TokenType::Identifier)
        throw std::runtime_error("Expected class name");
    std::string name = advance().value;
    if (!match(TokenType::LBrace))
        throw std::runtime_error("Expected '{' after class name");
    std::vector<MethodDecl> methods;
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        methods.push_back(methodDecl());
    }
    if (!match(TokenType::RBrace))
        throw std::runtime_error("Expected '}'");
    return std::make_unique<ClassDeclStmt>(name, std::move(methods));
}

MethodDecl Parser::methodDecl() {
    if (!match(TokenType::Method))
        throw std::runtime_error("Expected 'method'");
    if (peek().type != TokenType::Identifier)
        throw std::runtime_error("Expected method name");
    std::string name = advance().value;
    if (!match(TokenType::LParen))
        throw std::runtime_error("Expected '(' after method name");
    std::vector<std::string> params;
    while (peek().type == TokenType::Identifier) {
        params.push_back(advance().value);
        if (!match(TokenType::Comma)) break;
    }
    if (!match(TokenType::RParen))
        throw std::runtime_error("Expected ')'");
    if (!match(TokenType::LBrace))
        throw std::runtime_error("Expected '{' for method body");
    auto body = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        body->statements.push_back(statement());
    }
    if (!match(TokenType::RBrace))
        throw std::runtime_error("Expected '}'");
    MethodDecl m;
    m.name = name;
    m.params = std::move(params);
    m.body = std::move(body);
    return m;
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    if (!match(TokenType::LParen))
        throw std::runtime_error("Expected '(' after 'if'");
    auto condition = expression();
    if (!match(TokenType::RParen))
        throw std::runtime_error("Expected ')' after condition");
    auto thenBranch = statement();
    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else)) elseBranch = statement();
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    if (!match(TokenType::LParen))
        throw std::runtime_error("Expected '(' after 'while'");
    auto condition = expression();
    if (!match(TokenType::RParen))
        throw std::runtime_error("Expected ')' after condition");
    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    std::unique_ptr<Expr> value;
    if (!check(TokenType::Semicolon) && peek().type != TokenType::Eof)
        value = expression();
    match(TokenType::Semicolon);
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::tryStatement() {
    if (!match(TokenType::LBrace))
        throw std::runtime_error("Expected '{' after 'try'");
    auto tryBody = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof)
        tryBody->statements.push_back(statement());
    if (!match(TokenType::RBrace))
        throw std::runtime_error("Expected '}' after try block");
    if (!match(TokenType::Catch))
        throw std::runtime_error("Expected 'catch' after try block");
    if (!match(TokenType::LParen))
        throw std::runtime_error("Expected '(' after 'catch'");
    if (peek().type != TokenType::Identifier)
        throw std::runtime_error("Expected variable name in catch");
    std::string catchVar = advance().value;
    if (!match(TokenType::RParen))
        throw std::runtime_error("Expected ')' after catch variable");
    if (!match(TokenType::LBrace))
        throw std::runtime_error("Expected '{' for catch block");
    auto catchBody = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof)
        catchBody->statements.push_back(statement());
    if (!match(TokenType::RBrace))
        throw std::runtime_error("Expected '}' after catch block");
    return std::make_unique<TryCatchStmt>(std::move(tryBody), std::move(catchVar), std::move(catchBody));
}

std::unique_ptr<Stmt> Parser::throwStatement() {
    auto value = expression();
    match(TokenType::Semicolon);
    return std::make_unique<ThrowStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        block->statements.push_back(statement());
    }
    if (!match(TokenType::RBrace))
        throw std::runtime_error("Expected '}'");
    return block;
}

std::unique_ptr<Stmt> Parser::exprStatement() {
    if (peek().type == TokenType::Identifier && pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == TokenType::Assign) {
        std::string name = advance().value;
        advance(); // =
        auto expr = expression();
        match(TokenType::Semicolon);
        return std::make_unique<AssignStmt>(name, std::move(expr));
    }
    auto expr = expression();
    if (match(TokenType::Assign)) {
        if (IndexExpr* idx = dynamic_cast<IndexExpr*>(expr.get())) {
            std::unique_ptr<Expr> arr = std::move(idx->array);
            std::unique_ptr<Expr> indexExpr = std::move(idx->index);
            auto val = expression();
            match(TokenType::Semicolon);
            return std::make_unique<SetIndexStmt>(std::move(arr), std::move(indexExpr), std::move(val));
        }
        GetExpr* g = dynamic_cast<GetExpr*>(expr.get());
        if (g) {
            std::unique_ptr<Expr> obj = std::move(g->object);
            std::string propName = g->name;
            auto val = expression();
            match(TokenType::Semicolon);
            return std::make_unique<SetPropertyStmt>(std::move(obj), propName, std::move(val));
        }
        throw std::runtime_error("Invalid assignment target");
    }
    match(TokenType::Semicolon);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expr> Parser::expression() {
    return logicalOr();
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match(TokenType::Or)) {
        expr = std::make_unique<BinaryExpr>("||", std::move(expr), logicalAnd());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = logicalNot();
    while (match(TokenType::And)) {
        expr = std::make_unique<BinaryExpr>("&&", std::move(expr), logicalNot());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalNot() {
    if (match(TokenType::Not)) {
        return std::make_unique<UnaryExpr>("!", logicalNot());
    }
    return comparison();
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();
    for (;;) {
        if (match(TokenType::Eq))
            expr = std::make_unique<BinaryExpr>("==", std::move(expr), term());
        else if (match(TokenType::Ne))
            expr = std::make_unique<BinaryExpr>("!=", std::move(expr), term());
        else if (match(TokenType::Lt))
            expr = std::make_unique<BinaryExpr>("<", std::move(expr), term());
        else if (match(TokenType::Le))
            expr = std::make_unique<BinaryExpr>("<=", std::move(expr), term());
        else if (match(TokenType::Gt))
            expr = std::make_unique<BinaryExpr>(">", std::move(expr), term());
        else if (match(TokenType::Ge))
            expr = std::make_unique<BinaryExpr>(">=", std::move(expr), term());
        else break;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    for (;;) {
        if (match(TokenType::Plus))
            expr = std::make_unique<BinaryExpr>("+", std::move(expr), factor());
        else if (match(TokenType::Minus))
            expr = std::make_unique<BinaryExpr>("-", std::move(expr), factor());
        else break;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = primary();
    for (;;) {
        if (match(TokenType::Star))
            expr = std::make_unique<BinaryExpr>("*", std::move(expr), primary());
        else if (match(TokenType::Slash))
            expr = std::make_unique<BinaryExpr>("/", std::move(expr), primary());
        else break;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::primary() {
    std::unique_ptr<Expr> expr;
    if (match(TokenType::Number)) {
        double v = std::stod(tokens_[pos_ - 1].value);
        expr = std::make_unique<NumberExpr>(v);
    } else if (match(TokenType::String)) {
        expr = std::make_unique<StringExpr>(tokens_[pos_ - 1].value);
    } else if (match(TokenType::True)) {
        expr = std::make_unique<BoolExpr>(true);
    } else if (match(TokenType::False)) {
        expr = std::make_unique<BoolExpr>(false);
    } else if (match(TokenType::Identifier)) {
        expr = std::make_unique<VarExpr>(tokens_[pos_ - 1].value);
    } else if (match(TokenType::This)) {
        expr = std::make_unique<ThisExpr>();
    } else if (match(TokenType::LParen)) {
        expr = expression();
        if (!match(TokenType::RParen))
            throw std::runtime_error("Expected ')'");
    } else if (match(TokenType::LBracket)) {
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RBracket)) {
            elements.push_back(expression());
            while (match(TokenType::Comma)) elements.push_back(expression());
        }
        if (!match(TokenType::RBracket))
            throw std::runtime_error("Expected ']'");
        expr = std::make_unique<ArrayExpr>(std::move(elements));
    } else {
        throw std::runtime_error("Unexpected token");
    }
    return postfix(std::move(expr));
}

std::unique_ptr<Expr> Parser::postfix(std::unique_ptr<Expr> expr) {
    for (;;) {
        if (match(TokenType::Dot)) {
            if (peek().type != TokenType::Identifier)
                throw std::runtime_error("Expected property name");
            std::string name = advance().value;
            expr = std::make_unique<GetExpr>(std::move(expr), name);
        } else if (match(TokenType::LParen)) {
            auto args = exprList();
            if (!match(TokenType::RParen))
                throw std::runtime_error("Expected ')'");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::LBracket)) {
            auto indexExpr = expression();
            if (!match(TokenType::RBracket))
                throw std::runtime_error("Expected ']'");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(indexExpr));
        } else {
            break;
        }
    }
    return expr;
}

std::vector<std::unique_ptr<Expr>> Parser::exprList() {
    std::vector<std::unique_ptr<Expr>> args;
    if (check(TokenType::RParen)) return args;
    args.push_back(expression());
    while (match(TokenType::Comma)) {
        args.push_back(expression());
    }
    return args;
}
