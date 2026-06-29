#pragma once
#include "Expr.h"
#include "Token.h"
#include "Stmt.h"
#include "RuntimeError.h"
#include "Environment.h"
#include <variant>
#include <type_traits>
#include <iostream>
#include <vector>
#include <memory>

using namespace std;

class Interpreter 
{
private:
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();

    void checkNumberOperand(const Token& op, const Literal& operand) {
        if (holds_alternative<double>(operand)) return;
        throw RuntimeError(op, "Operand must be a number.");
    }

    void checkNumberOperands(const Token& op, const Literal& left, const Literal& right) {
        if (holds_alternative<double>(left) && holds_alternative<double>(right)) return;
        throw RuntimeError(op, "Operands must be numbers.");
    }

    bool isTruthy(const Literal& object) 
    {
        if (holds_alternative<nullptr_t>(object)) return false;
        if (holds_alternative<bool>(object)) return get<bool>(object);
        return true;
    }
    
    bool isEqual(const Literal& a, const Literal& b) 
    {
        return a == b; 
    }
    
    string stringify(const Literal& value) 
    {
        return visit([](const auto& val) -> string {
            using ValT = decay_t<decltype(val)>;
            
            if constexpr (is_same_v<ValT, nullptr_t>) return "nil";
            else if constexpr (is_same_v<ValT, bool>) return val ? "true" : "false";
            else if constexpr (is_same_v<ValT, string>) return val;
            else if constexpr (is_same_v<ValT, double>) 
            {
                string text = to_string(val);
                text.erase(text.find_last_not_of('0') + 1, string::npos);
                if (text.back() == '.') text.pop_back();
                return text;
            }
            return "";
        }, value);
    }

    void execute(const Stmt& stmt) 
    {
        visit([this](const auto& node) {
            using T = decay_t<decltype(*node)>;

            if constexpr (is_same_v<T, PrintStmt>) {
                Literal value = evaluate(node->expression);
                cout << stringify(value) << "\n";
            } 
            else if constexpr (is_same_v<T, ExpressionStmt>) {
                evaluate(node->expression); 
            }
            else if constexpr (is_same_v<T, VarStmt>) {
                Literal value = nullptr;
                if (node->initializer.has_value()) {
                    value = evaluate(node->initializer.value());
                }
                environment->define(node->name.lexeme, value);
            }
            else if constexpr (is_same_v<T, Block>) 
            {
                executeBlock(node->statements, std::make_shared<Environment>(environment));
            }
            else if constexpr (is_same_v<T, IfStmt>) 
            {
                Literal conditionResult = evaluate(node->condition);

                if (isTruthy(conditionResult)) 
                {
                    execute(node->thenBranch);
                } 
                else if (node->elseBranch.has_value()) 
                {
                    execute(node->elseBranch.value());
                }
            }
            else if constexpr (is_same_v<T, WhileStmt>) 
            {
                while (isTruthy(evaluate(node->condition))) 
                {
                    execute(node->body);
                }
            }
        }, stmt);
    }

    void executeBlock(const vector<Stmt>& statements, std::shared_ptr<Environment> environment) {
        std::shared_ptr<Environment> previous = this->environment;

        try {
            this->environment = environment;

            for (const Stmt& statement : statements) {
                execute(statement);
            }
        } catch (...) {
            this->environment = previous;
            throw;
        }

        this->environment = previous;
    }

public:
    Literal evaluate(const Expr& expr) 
    {
        return visit([this](const auto& node) -> Literal 
        {
            using T = decay_t<decltype(*node)>;

            if constexpr (is_same_v<T, LiteralExpr>) 
            {
                return node->value;
            } 
            else if constexpr (is_same_v<T, Grouping>) 
            {
                return evaluate(node->expression);
            } 
            else if constexpr (is_same_v<T, Unary>) 
            {
                Literal right = evaluate(node->right);

                switch (node->op.type) 
                {
                    case TokenType::BANG:
                        return !isTruthy(right);
                    case TokenType::MINUS:
                        checkNumberOperand(node->op, right);
                        return -get<double>(right);
                    default:
                        return nullptr;
                }
            }
            else if constexpr (is_same_v<T, Binary>) 
            {
                Literal left = evaluate(node->left);
                Literal right = evaluate(node->right);

                switch (node->op.type) 
                {
                    case TokenType::MINUS:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) - get<double>(right);
                    case TokenType::SLASH:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) / get<double>(right);
                    case TokenType::STAR:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) * get<double>(right);
                    case TokenType::PLUS:
                        if (holds_alternative<double>(left) && holds_alternative<double>(right)) 
                        {
                            return get<double>(left) + get<double>(right);
                        }
                        if (holds_alternative<string>(left) && holds_alternative<string>(right)) 
                        {
                            return get<string>(left) + get<string>(right);
                        }
                        throw RuntimeError(node->op, "Operands must be two numbers or two strings.");
                    case TokenType::GREATER:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) > get<double>(right);
                    case TokenType::GREATER_EQUAL:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) >= get<double>(right);
                    case TokenType::LESS:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) < get<double>(right);
                    case TokenType::LESS_EQUAL:
                        checkNumberOperands(node->op, left, right);
                        return get<double>(left) <= get<double>(right);
                    case TokenType::BANG_EQUAL:
                        return !isEqual(left, right);
                    case TokenType::EQUAL_EQUAL:
                        return isEqual(left, right);
                    default:
                        return nullptr;
                }
            }
            else if constexpr (is_same_v<T, Variable>) 
            {
                return environment->get(node->name);
            }
            else if constexpr (is_same_v<T, Assign>) 
            {
                Literal value = evaluate(node->value);
                environment->assign(node->name, value);
                return value;
            }
            else if constexpr (is_same_v<T, Logical>) 
            {
                Literal left = evaluate(node->left);

                if (node->op.type == TokenType::OR) {
                    if (isTruthy(left)) return left;
                }
                else { 
                    if (!isTruthy(left)) return left;
                }

                return evaluate(node->right);
            }
        }, expr);
    }
    
    void interpret(const vector<Stmt>& statements) 
    {
        try {
            for (const Stmt& statement : statements) {
                execute(statement);
            }
        } catch (const RuntimeError& error) {
            cerr << error.what() << "\n[line " << error.token.line << "]\n";
        }
    }
};