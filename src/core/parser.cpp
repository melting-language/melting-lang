#include "parser.hpp"
#include <stdexcept>
#include <cstdlib>
#include <string>

Parser::Parser(std::vector<Token> tokens, std::string sourceName)
    : tokens_(std::move(tokens)), pos_(0), sourceName_(std::move(sourceName)) {}

void Parser::parseError(const std::string& msg) {
    int line = peek().line;
    std::string out;
    if (!sourceName_.empty()) out += sourceName_ + ": ";
    out += "line " + std::to_string(line) + ": " + msg;
    throw std::runtime_error(out);
}

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
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::Foreach)) return foreachStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Break)) return breakStatement();
    if (match(TokenType::Continue)) return continueStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::Try)) return tryStatement();
    if (match(TokenType::Throw)) return throwStatement();
    if (match(TokenType::LBrace)) return blockStatement();
    return exprStatement();
}

std::unique_ptr<Stmt> Parser::printStatement() {
    int line = peek().line;
    auto expr = expression();
    match(TokenType::Semicolon);
    auto s = std::make_unique<PrintStmt>(std::move(expr));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::letStatement() {
    int line = peek().line;
    if (peek().type != TokenType::Identifier)
        parseError("Expected variable name");
    std::string name = advance().value;
    if (!match(TokenType::Assign))
        parseError("Expected '=' after variable name");
    auto expr = expression();
    match(TokenType::Semicolon);
    auto s = std::make_unique<LetStmt>(name, std::move(expr));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::importStatement() {
    int line = peek().line;
    if (peek().type != TokenType::String)
        parseError("Expected string path after 'import'");
    std::string path = advance().value;
    std::string asName;
    if (check(TokenType::Identifier) && peek().value == "as") {
        advance();
        if (peek().type != TokenType::Identifier)
            parseError("Expected variable name after 'as'");
        asName = advance().value;
    }
    if (!match(TokenType::Semicolon))
        parseError("Expected ';' after import path");
    auto s = std::make_unique<ImportStmt>(std::move(path), std::move(asName));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::classStatement() {
    int line = peek().line;
    if (peek().type != TokenType::Identifier)
        parseError("Expected class name");
    std::string name = advance().value;
    if (!match(TokenType::LBrace))
        parseError("Expected '{' after class name");
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> staticFields;
    std::vector<MethodDecl> methods;
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        if (match(TokenType::Static)) {
            if (peek().type != TokenType::Identifier)
                parseError("Expected static variable name");
            std::string varName = advance().value;
            if (!match(TokenType::Assign))
                parseError("Expected '=' after static variable name");
            staticFields.push_back({std::move(varName), expression()});
            if (!match(TokenType::Semicolon))
                parseError("Expected ';' after static initializer");
        } else {
            methods.push_back(methodDecl());
        }
    }
    if (!match(TokenType::RBrace))
        parseError("Expected '}'");
    auto s = std::make_unique<ClassDeclStmt>(name, std::move(staticFields), std::move(methods));
    s->line = line;
    return s;
}

MethodDecl Parser::methodDecl() {
    if (!match(TokenType::Method))
        parseError("Expected 'method'");
    if (peek().type != TokenType::Identifier)
        parseError("Expected method name");
    std::string name = advance().value;
    if (!match(TokenType::LParen))
        parseError("Expected '(' after method name");
    std::vector<std::string> params;
    while (peek().type == TokenType::Identifier) {
        params.push_back(advance().value);
        if (!match(TokenType::Comma)) break;
    }
    if (!match(TokenType::RParen))
        parseError("Expected ')'");
    if (!match(TokenType::LBrace))
        parseError("Expected '{' for method body");
    int savedLoopDepth = loopDepth_;
    loopDepth_ = 0;
    auto body = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        body->statements.push_back(statement());
    }
    loopDepth_ = savedLoopDepth;
    if (!match(TokenType::RBrace))
        parseError("Expected '}'");
    MethodDecl m;
    m.name = name;
    m.params = std::move(params);
    m.body = std::move(body);
    return m;
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    int line = peek().line;
    if (!match(TokenType::LParen))
        parseError("Expected '(' after 'if'");
    auto condition = expression();
    if (!match(TokenType::RParen))
        parseError("Expected ')' after condition");
    auto thenBranch = statement();
    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else)) elseBranch = statement();
    auto s = std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::forStatement() {
    int line = peek().line;
    if (!match(TokenType::LParen))
        parseError("Expected '(' after 'for'");
    std::unique_ptr<Stmt> init;
    if (match(TokenType::Semicolon)) {
        // no init
    } else if (match(TokenType::Let)) {
        init = letStatement();
    } else {
        init = std::make_unique<ExprStmt>(expression());
        init->line = line;
        if (!match(TokenType::Semicolon))
            parseError("Expected ';' after for init");
    }
    std::unique_ptr<Expr> condition;
    if (check(TokenType::Semicolon)) {
        advance();
    } else {
        condition = expression();
        if (!match(TokenType::Semicolon))
            parseError("Expected ';' after for condition");
    }
    std::unique_ptr<Expr> update;
    if (check(TokenType::RParen)) {
        advance();
    } else {
        update = expression();
        if (!match(TokenType::RParen))
            parseError("Expected ')' after for update");
    }
    ++loopDepth_;
    auto body = statement();
    --loopDepth_;
    auto s = std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(update), std::move(body));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::foreachStatement() {
    int line = peek().line;
    if (!match(TokenType::LParen))
        parseError("Expected '(' after 'foreach'");
    if (peek().type != TokenType::Identifier)
        parseError("Expected variable name in foreach");
    std::string firstVar = advance().value;
    std::string secondVar = "";
    bool hasSecondVar = false;
    if (match(TokenType::Comma)) {
        if (peek().type != TokenType::Identifier)
            parseError("Expected second variable name in foreach");
        secondVar = advance().value;
        hasSecondVar = true;
    }
    if (!match(TokenType::In))
        parseError("Expected 'in' in foreach");
    auto iterable = expression();
    if (!match(TokenType::RParen))
        parseError("Expected ')' after foreach iterable");
    ++loopDepth_;
    auto body = statement();
    --loopDepth_;
    auto s = std::make_unique<ForeachStmt>(std::move(firstVar), std::move(secondVar), hasSecondVar, std::move(iterable), std::move(body));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    int line = peek().line;
    if (!match(TokenType::LParen))
        parseError("Expected '(' after 'while'");
    auto condition = expression();
    if (!match(TokenType::RParen))
        parseError("Expected ')' after condition");
    ++loopDepth_;
    auto body = statement();
    --loopDepth_;
    auto s = std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::breakStatement() {
    int line = peek().line;
    if (loopDepth_ == 0)
        parseError("'break' used outside of loop");
    if (!match(TokenType::Semicolon))
        parseError("Expected ';' after 'break'");
    auto s = std::make_unique<BreakStmt>();
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::continueStatement() {
    int line = peek().line;
    if (loopDepth_ == 0)
        parseError("'continue' used outside of loop");
    if (!match(TokenType::Semicolon))
        parseError("Expected ';' after 'continue'");
    auto s = std::make_unique<ContinueStmt>();
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    int line = peek().line;
    std::unique_ptr<Expr> value;
    if (!check(TokenType::Semicolon) && peek().type != TokenType::Eof)
        value = expression();
    match(TokenType::Semicolon);
    auto s = std::make_unique<ReturnStmt>(std::move(value));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::tryStatement() {
    int line = peek().line;
    if (!match(TokenType::LBrace))
        parseError("Expected '{' after 'try'");
    auto tryBody = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof)
        tryBody->statements.push_back(statement());
    if (!match(TokenType::RBrace))
        parseError("Expected '}' after try block");
    if (!match(TokenType::Catch))
        parseError("Expected 'catch' after try block");
    if (!match(TokenType::LParen))
        parseError("Expected '(' after 'catch'");
    if (peek().type != TokenType::Identifier)
        parseError("Expected variable name in catch");
    std::string catchVar = advance().value;
    if (!match(TokenType::RParen))
        parseError("Expected ')' after catch variable");
    if (!match(TokenType::LBrace))
        parseError("Expected '{' for catch block");
    auto catchBody = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof)
        catchBody->statements.push_back(statement());
    if (!match(TokenType::RBrace))
        parseError("Expected '}' after catch block");
    auto s = std::make_unique<TryCatchStmt>(std::move(tryBody), std::move(catchVar), std::move(catchBody));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::throwStatement() {
    int line = peek().line;
    auto value = expression();
    match(TokenType::Semicolon);
    auto s = std::make_unique<ThrowStmt>(std::move(value));
    s->line = line;
    return s;
}

std::unique_ptr<Stmt> Parser::blockStatement() {
    int line = peek().line;
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBrace) && peek().type != TokenType::Eof) {
        block->statements.push_back(statement());
    }
    if (!match(TokenType::RBrace))
        parseError("Expected '}'");
    block->line = line;
    return block;
}

std::unique_ptr<Stmt> Parser::exprStatement() {
    int line = peek().line;
    if (peek().type == TokenType::Identifier && pos_ + 1 < tokens_.size() && tokens_[pos_ + 1].type == TokenType::Assign) {
        std::string name = advance().value;
        advance(); // =
        auto expr = expression();
        match(TokenType::Semicolon);
        auto s = std::make_unique<AssignStmt>(name, std::move(expr));
        s->line = line;
        return s;
    }
    auto expr = expression();
    if (match(TokenType::Assign)) {
        if (IndexExpr* idx = dynamic_cast<IndexExpr*>(expr.get())) {
            std::unique_ptr<Expr> arr = std::move(idx->array);
            std::unique_ptr<Expr> indexExpr = std::move(idx->index);
            auto val = expression();
            match(TokenType::Semicolon);
            auto s = std::make_unique<SetIndexStmt>(std::move(arr), std::move(indexExpr), std::move(val));
            s->line = line;
            return s;
        }
        GetExpr* g = dynamic_cast<GetExpr*>(expr.get());
        if (g) {
            std::unique_ptr<Expr> obj = std::move(g->object);
            std::string propName = g->name;
            auto val = expression();
            match(TokenType::Semicolon);
            auto s = std::make_unique<SetPropertyStmt>(std::move(obj), propName, std::move(val));
            s->line = line;
            return s;
        }
        parseError("Invalid assignment target");
    }
    match(TokenType::Semicolon);
    auto s = std::make_unique<ExprStmt>(std::move(expr));
    s->line = line;
    return s;
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = logicalOr();
    if (expr && peek().type == TokenType::Assign) {
        if (VarExpr* v = dynamic_cast<VarExpr*>(expr.get())) {
            std::string name = v->name;
            advance(); // =
            auto value = assignment();
            return std::make_unique<AssignExpr>(name, std::move(value));
        }
        // GetExpr/IndexExpr handled as SetPropertyStmt/SetIndexStmt in exprStatement
        return expr;
    }
    return expr;
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
    auto expr = unaryMinus();
    for (;;) {
        if (match(TokenType::Star))
            expr = std::make_unique<BinaryExpr>("*", std::move(expr), unaryMinus());
        else if (match(TokenType::Slash))
            expr = std::make_unique<BinaryExpr>("/", std::move(expr), unaryMinus());
        else break;
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unaryMinus() {
    if (match(TokenType::Minus)) {
        return std::make_unique<UnaryExpr>("-", unaryMinus());
    }
    return primary();
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
            parseError("Expected ')'");
    } else if (match(TokenType::LBracket)) {
        if (check(TokenType::RBracket)) {
            advance();
            expr = std::make_unique<ArrayExpr>(std::vector<std::unique_ptr<Expr>>{});
        } else {
            std::unique_ptr<Expr> first = expression();
            if (match(TokenType::Arrow)) {
                // Map literal: [ "key" => value, ... ]
                auto mapExpr = std::make_unique<MapExpr>();
                mapExpr->entries.push_back({std::move(first), expression()});
                while (match(TokenType::Comma)) {
                    std::unique_ptr<Expr> key = expression();
                    if (!match(TokenType::Arrow))
                        parseError("Expected ':=>' in map literal");
                    mapExpr->entries.push_back({std::move(key), expression()});
                }
                if (!match(TokenType::RBracket))
                    parseError("Expected ']'");
                expr = std::move(mapExpr);
            } else {
                // Array literal: [ a, b, c ]
                std::vector<std::unique_ptr<Expr>> elements;
                elements.push_back(std::move(first));
                while (match(TokenType::Comma)) elements.push_back(expression());
                if (!match(TokenType::RBracket))
                    parseError("Expected ']'");
                expr = std::make_unique<ArrayExpr>(std::move(elements));
            }
        }
    } else if (match(TokenType::Fn)) {
        if (!match(TokenType::LParen))
            parseError("Expected '(' after 'fn'");
        std::vector<std::string> params;
        if (!check(TokenType::RParen)) {
            if (peek().type != TokenType::Identifier)
                parseError("Expected parameter name");
            params.push_back(advance().value);
            while (match(TokenType::Comma)) {
                if (peek().type != TokenType::Identifier)
                    parseError("Expected parameter name");
                params.push_back(advance().value);
            }
        }
        if (!match(TokenType::RParen))
            parseError("Expected ')'");
        if (!match(TokenType::LBrace))
            parseError("Expected '{' for lambda body");
        int savedLoopDepth = loopDepth_;
        loopDepth_ = 0;
        auto blockStmt = blockStatement();
        loopDepth_ = savedLoopDepth;
        BlockStmt* bs = dynamic_cast<BlockStmt*>(blockStmt.get());
        if (!bs)
            parseError("Expected block");
        auto body = std::make_unique<BlockStmt>();
        body->statements = std::move(bs->statements);
        expr = std::make_unique<LambdaExpr>(std::move(params), std::move(body));
    } else {
        parseError("Unexpected token");
    }
    return postfix(std::move(expr));
}

std::unique_ptr<Expr> Parser::postfix(std::unique_ptr<Expr> expr) {
    for (;;) {
        if (match(TokenType::Dot)) {
            if (peek().type != TokenType::Identifier)
                parseError("Expected property name");
            std::string name = advance().value;
            expr = std::make_unique<GetExpr>(std::move(expr), name);
        } else if (match(TokenType::LParen)) {
            auto args = exprList();
            if (!match(TokenType::RParen))
                parseError("Expected ')'");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::LBracket)) {
            auto indexExpr = expression();
            if (!match(TokenType::RBracket))
                parseError("Expected ']'");
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
