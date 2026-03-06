#pragma once

#include <string>
#include <vector>
#include <memory>

struct BlockStmt;
struct Stmt;
struct Expr;

struct Expr {
    virtual ~Expr() = default;
};

struct NumberExpr : Expr {
    double value;
    NumberExpr(double v) : value(v) {}
};

struct StringExpr : Expr {
    std::string value;
    StringExpr(const std::string& v) : value(v) {}
};

struct BoolExpr : Expr {
    bool value;
    explicit BoolExpr(bool v) : value(v) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(const std::string& n) : name(n) {}
};

struct AssignExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(const std::string& n, std::unique_ptr<Expr> v) : name(n), value(std::move(v)) {}
};

struct BinaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinaryExpr(const std::string& o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> operand;
    UnaryExpr(std::string o, std::unique_ptr<Expr> e) : op(std::move(o)), operand(std::move(e)) {}
};

struct ThisExpr : Expr {};

struct GetExpr : Expr {
    std::unique_ptr<Expr> object;
    std::string name;
    GetExpr(std::unique_ptr<Expr> o, const std::string& n) : object(std::move(o)), name(n) {}
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(std::unique_ptr<Expr> c, std::vector<std::unique_ptr<Expr>> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

struct ArrayExpr : Expr {
    std::vector<std::unique_ptr<Expr>> elements;
    ArrayExpr() = default;
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> e) : elements(std::move(e)) {}
};

// PHP-style map literal: [ "key" :=> value, ... ] -> MeltObject
struct MapExpr : Expr {
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
    MapExpr() = default;
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> a, std::unique_ptr<Expr> i) : array(std::move(a)), index(std::move(i)) {}
};

struct BlockStmt;
struct LambdaExpr : Expr {
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    LambdaExpr(std::vector<std::string> p, std::unique_ptr<BlockStmt> b) : params(std::move(p)), body(std::move(b)) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct BlockStmt;

struct MethodDecl {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
};

struct ClassDeclStmt : Stmt {
    std::string name;
    std::vector<MethodDecl> methods;
    ClassDeclStmt(std::string n, std::vector<MethodDecl> m) : name(std::move(n)), methods(std::move(m)) {}
};

struct SetPropertyStmt : Stmt {
    std::unique_ptr<Expr> object;
    std::string name;
    std::unique_ptr<Expr> value;
    SetPropertyStmt(std::unique_ptr<Expr> o, const std::string& n, std::unique_ptr<Expr> v)
        : object(std::move(o)), name(n), value(std::move(v)) {}
};

struct SetIndexStmt : Stmt {
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
    SetIndexStmt(std::unique_ptr<Expr> a, std::unique_ptr<Expr> i, std::unique_ptr<Expr> v)
        : array(std::move(a)), index(std::move(i)), value(std::move(v)) {}
};

struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

struct ImportStmt : Stmt {
    std::string path;
    std::string asName;  // if non-empty: bind module namespace to this variable (import "x.melt" as M;)
    ImportStmt(std::string p, std::string as = std::string()) : path(std::move(p)), asName(std::move(as)) {}
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
    PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

struct LetStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> expr;
    LetStmt(const std::string& n, std::unique_ptr<Expr> e) : name(n), expr(std::move(e)) {}
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> expr;
    AssignStmt(const std::string& n, std::unique_ptr<Expr> e) : name(n), expr(std::move(e)) {}
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> e)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct ForStmt : Stmt {
    std::unique_ptr<Stmt> init;       // optional: let i = 0 or expr;
    std::unique_ptr<Expr> condition;  // optional: empty means true
    std::unique_ptr<Expr> update;     // optional
    std::unique_ptr<Stmt> body;
    ForStmt(std::unique_ptr<Stmt> i, std::unique_ptr<Expr> c, std::unique_ptr<Expr> u, std::unique_ptr<Stmt> b)
        : init(std::move(i)), condition(std::move(c)), update(std::move(u)), body(std::move(b)) {}
};

struct ForeachStmt : Stmt {
    std::string firstVar;            // value (array) OR key/value first binding
    std::string secondVar;           // optional: value for object, value for array when firstVar is index
    bool hasSecondVar = false;
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<Stmt> body;
    ForeachStmt(std::string a, std::string b, bool hasB, std::unique_ptr<Expr> it, std::unique_ptr<Stmt> bd)
        : firstVar(std::move(a)), secondVar(std::move(b)), hasSecondVar(hasB), iterable(std::move(it)), body(std::move(bd)) {}
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : condition(std::move(c)), body(std::move(b)) {}
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;  // optional: null means "return;" with no value
    explicit ReturnStmt(std::unique_ptr<Expr> v = nullptr) : value(std::move(v)) {}
};

struct TryCatchStmt : Stmt {
    std::unique_ptr<BlockStmt> tryBody;
    std::string catchVar;  // variable name to bind thrown value in catch block
    std::unique_ptr<BlockStmt> catchBody;
    TryCatchStmt(std::unique_ptr<BlockStmt> tryB, std::string catchV, std::unique_ptr<BlockStmt> catchB)
        : tryBody(std::move(tryB)), catchVar(std::move(catchV)), catchBody(std::move(catchB)) {}
};

struct ThrowStmt : Stmt {
    std::unique_ptr<Expr> value;  // required: value to throw
    explicit ThrowStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
};
