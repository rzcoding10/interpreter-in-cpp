#include "Parser.h"
#include "Lox.h"

Parser::Parser(const vector<Token>& tokens) : tokens(tokens) 
{
}

Expr Parser::expression()
{
    return assignment();
}

Expr Parser::assignment() 
{
        Expr expr = logic_or(); 

        if (match({TokenType::EQUAL})) 
        {
            Token equals = previous();
            Expr value = assignment(); 
            if (holds_alternative<unique_ptr<Variable>>(expr)) 
            {
                Token name = get<unique_ptr<Variable>>(expr)->name;
                return makeExpr<Assign>(name, std::move(value));
            }
            error(equals, "Invalid assignment target."); 
        }

        return expr;
}

Expr Parser::logic_or() 
{
    Expr expr = logic_and();

    while (match({TokenType::OR})) 
    {
        Token op = previous();
        Expr right = logic_and();
        expr = std::make_unique<Logical>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

Expr Parser::logic_and() 
{
    Expr expr = equality();

    while (match({TokenType::AND})) 
    {
        Token op = previous();
        Expr right = equality();
        expr = std::make_unique<Logical>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

Expr Parser::equality()
{
    Expr expr = comparison();

    while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) 
    {
        Token op = previous();
        Expr right = comparison();
        
       
        expr = makeExpr<Binary>(move(expr), op, move(right));
    }

    return expr;
}

Expr Parser::comparison() 
{
    Expr expr = term();

    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) 
    {
        Token op = previous();
        Expr right = term();
        expr = makeExpr<Binary>(move(expr), op, move(right));
    }

    return expr;
}

Expr Parser::term() 
{
    Expr expr = factor();

    while (match({TokenType::MINUS, TokenType::PLUS})) 
    {
        Token op = previous();
        Expr right = factor();
        expr = makeExpr<Binary>(move(expr), op, move(right));
    }

    return expr;
}

Expr Parser::factor() 
{
    Expr expr = unary();

    while (match({TokenType::SLASH, TokenType::STAR})) 
    {
        Token op = previous();
        Expr right = unary();
        expr = makeExpr<Binary>(move(expr), op, move(right));
    }

    return expr;
}

Expr Parser::unary() 
{
    if (match({TokenType::BANG, TokenType::MINUS})) 
    {
        Token op = previous();
        Expr right = unary();
        return makeExpr<Unary>(op, move(right));
    }

    return primary();
}

Expr Parser::primary() 
{
    if (match({TokenType::FALSE})) return makeExpr<LiteralExpr>(false);
    if (match({TokenType::TRUE})) return makeExpr<LiteralExpr>(true);
    if (match({TokenType::NIL})) return makeExpr<LiteralExpr>(nullptr);

    if (match({TokenType::NUMBER, TokenType::STRING})) 
    {
        return makeExpr<LiteralExpr>(previous().literal);
    }

    if (match({TokenType::LEFT_PAREN})) 
    {
        Expr expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return makeExpr<Grouping>(move(expr));
    }

    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<Variable>(previous());
    }

    throw error(peek(), "Expect expression.");
}

Token Parser::consume(TokenType type, const string& message) 
{
    if (check(type)) return advance();

    throw error(peek(), message);
}

Parser::ParseError Parser::error(Token token, const string& message) 
{
    Lox::error(token, message);
    return ParseError();
}

vector<Stmt> Parser::parse() 
{
    vector<Stmt> statements;
    while (!isAtEnd()) {
        try {
            statements.push_back(declaration());
        } catch (const ParseError& error) {
            synchronize(); 
        }
    }
    return statements;
}

Stmt Parser::declaration() {
    if (match({TokenType::VAR})) return varDeclaration();
    return statement();
}

Stmt Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

    std::optional<Expr> initializer = std::nullopt;
    if (match({TokenType::EQUAL})) {
        initializer = expression();
    }

    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(std::move(name), std::move(initializer));
}

Stmt Parser::statement() 
{
    if (match({TokenType::FOR})) 
    {    
        return forStatement();
    }

    if (match({TokenType::IF})) 
    {
        return ifStatement();
    }

    if (match({TokenType::PRINT})) 
    {
        return printStatement();
    }

    if (match({TokenType::WHILE})) {
        return whileStatement();
    }

    if (match({TokenType::LEFT_BRACE})) 
    {
        return make_unique<Block>(block());
    }

    return expressionStatement();
}

vector<Stmt> Parser::block() 
{
    vector<Stmt> statements;

    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return statements;
}

Stmt Parser::printStatement() 
{
    Expr value = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after value.");
    return make_unique<PrintStmt>(std::move(value));
}

Stmt Parser::expressionStatement() 
{
    Expr expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return make_unique<ExpressionStmt>(std::move(expr));
}

Stmt Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    Expr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
    Stmt thenBranch = statement();

    std::optional<Stmt> elseBranch = std::nullopt;
    if (match({TokenType::ELSE})) 
    {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(
        std::move(condition), 
        std::move(thenBranch), 
        std::move(elseBranch)
    );
}

Stmt Parser::whileStatement() 
{
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    Expr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    Stmt body = statement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

Stmt Parser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

    std::optional<Stmt> initializer;
    if (match({TokenType::SEMICOLON})) {
        initializer = std::nullopt;
    } else if (match({TokenType::VAR})) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }

    std::optional<Expr> condition = std::nullopt;
    if (!check(TokenType::SEMICOLON)) 
    {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

    std::optional<Expr> increment = std::nullopt;
    if (!check(TokenType::RIGHT_PAREN)) 
    {
        increment = expression();
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

    Stmt body = statement();

    if (increment.has_value()) 
    {
        std::vector<Stmt> stmts;
        stmts.push_back(std::move(body));
        stmts.push_back(std::make_unique<ExpressionStmt>(std::move(increment.value())));
        body = std::make_unique<Block>(std::move(stmts));
    }

    
    if (!condition.has_value()) 
    {
        condition = std::make_unique<LiteralExpr>(true); 
    }
    body = std::make_unique<WhileStmt>(std::move(condition.value()), std::move(body));

    if (initializer.has_value()) {
        std::vector<Stmt> stmts;
        stmts.push_back(std::move(initializer.value()));
        stmts.push_back(std::move(body));
        body = std::make_unique<Block>(std::move(stmts));
    }

    return body;
}

void Parser::synchronize() 
{
    advance();

    while (!isAtEnd()) 
    {
        if (previous().type == TokenType::SEMICOLON) return;

        switch (peek().type) 
        {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::VAR:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
            default:
                break; 
        }

        advance();
    }
}

bool Parser::match(initializer_list<TokenType> types) 
{
    for (TokenType type : types) 
    {
        if (check(type)) 
        {
            advance();
            return true;
        }
    }

    return false;
}

bool Parser::check(TokenType type) 
{
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() 
{
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() 
{
    return peek().type == TokenType::EOF_TOKEN; 
}

Token Parser::peek() 
{
    return tokens[current];
}

Token Parser::previous() 
{
    return tokens[current - 1];
}