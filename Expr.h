#pragma once
#include "Token.h"
#include <memory>
#include <vector>

using namespace std;

struct Binary;
struct Grouping;
struct LiteralExpr;
struct Logical;
struct Unary;
struct Variable;
struct Assign;


using Expr = variant<
    unique_ptr<Assign>,
    unique_ptr<Binary>,
    unique_ptr<Grouping>,
    unique_ptr<LiteralExpr>,
    std::unique_ptr<Logical>,
    unique_ptr<Unary>,
    unique_ptr<Variable>
>;


template <typename T, typename... Args>
Expr makeExpr(Args&&... args)
{
    return make_unique<T>(forward<Args>(args)...);
}


struct Binary
{
    Expr left;
    Token op;
    Expr right;

    Binary(Expr left, Token op, Expr right)
        : left(move(left)), op(move(op)), right(move(right)) {}
};

struct Grouping
{
    Expr expression;

    Grouping(Expr expression)
        : expression(move(expression)) {}
};

struct LiteralExpr
{
    Literal value;

    LiteralExpr(Literal value)
        : value(move(value)) {}
};

struct Unary
{
    Token op;
    Expr right;

    Unary(Token op, Expr right)
        : op(move(op)), right(move(right)) {}
};

struct Variable 
{
    Token name;

    Variable(Token name) : name(std::move(name)) {}
};

struct Assign 
{
    Token name;
    Expr value;

    Assign(Token name, Expr value)
        : name(std::move(name)), value(std::move(value)) {}
};

struct Logical {
    Expr left;
    Token op;
    Expr right;

    Logical(Expr left, Token op, Expr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};