#pragma once
#include <memory>
#include <variant>
#include <vector>
#include<optional>
#include "Expr.h"
#include "Token.h"

using namespace std;

struct Block;
struct ExpressionStmt;
struct IfStmt;
struct PrintStmt;
struct VarStmt;
struct WhileStmt;


using Stmt = variant<
    unique_ptr<Block>,
    unique_ptr<ExpressionStmt>,
    unique_ptr<IfStmt>,
    unique_ptr<PrintStmt>,
    unique_ptr<VarStmt>,
    unique_ptr<WhileStmt>
>;


struct Block {
    vector<Stmt> statements;

    Block(vector<Stmt> statements)
        : statements(std::move(statements)) {}
};

struct ExpressionStmt {
    Expr expression;

    ExpressionStmt(Expr expression)
        : expression(std::move(expression)) {}
};

struct PrintStmt {
    Expr expression;

    PrintStmt(Expr expression)
        : expression(std::move(expression)) {}
};

struct VarStmt {
    Token name;
    std::optional<Expr> initializer;

    VarStmt(Token name, std::optional<Expr> initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

struct IfStmt {
    Expr condition;
    Stmt thenBranch;
    std::optional<Stmt> elseBranch;

    IfStmt(Expr condition, Stmt thenBranch, std::optional<Stmt> elseBranch)
        : condition(std::move(condition)), 
          thenBranch(std::move(thenBranch)), 
          elseBranch(std::move(elseBranch)) {}
};

struct WhileStmt {
    Expr condition;
    Stmt body;

    WhileStmt(Expr condition, Stmt body)
        : condition(std::move(condition)), body(std::move(body)) {}
};