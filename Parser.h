#pragma once
#include "Token.h"
#include "Expr.h"
#include "Stmt.h"
#include <vector>
#include <stdexcept>
#include <optional>
#include <initializer_list>
#include <string>

using namespace std;

class Parser 
{
private:
    const vector<Token> tokens;
    int current = 0;

    class ParseError : public runtime_error {
    public:
        ParseError() : runtime_error("Parse Error") {}
    };

    Stmt declaration();
    Stmt varDeclaration();
    Stmt statement();
    vector<Stmt> block();
    Stmt printStatement();
    Stmt expressionStatement();
    Stmt ifStatement();
    Stmt whileStatement();
    Stmt forStatement();

    Expr expression();
    Expr assignment();
    Expr equality();
    Expr comparison();
    Expr term();
    Expr factor();
    Expr unary();
    Expr primary();
    Expr logic_or();
    Expr logic_and();

    bool match(initializer_list<TokenType> types);
    bool check(TokenType type);
    Token advance();
    bool isAtEnd();
    Token peek();
    Token previous();
    void synchronize();

    Token consume(TokenType type, const string& message);
    ParseError error(Token token, const string& message);

public:
    Parser(const vector<Token>& tokens);

    vector<Stmt> parse();
};