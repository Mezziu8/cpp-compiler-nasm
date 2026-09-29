#pragma once
#include <memory>
#include <string>
#include <vector>
#include "tokens.hpp"

struct Node
{
    virtual ~Node() = default;
};

struct NumberNode : Node
{
    Token tok;
    explicit NumberNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct IdentifierNode : Node
{
    Token tok;
    explicit IdentifierNode(Token t) : tok(std::move(t)) {}
    std::string getValue() const { return tok.value; }
};

struct BinOpNode : Node
{
    std::shared_ptr<Node> left;
    Token op_tok;
    std::shared_ptr<Node> right;
};


struct Statement : Node
{
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right; // may be null
};

struct StatementList : Node
{
    std::vector<std::shared_ptr<Statement>> statements;

};

struct Program : Node
{
    std::shared_ptr<StatementList> body;
};

struct Condition : Node
{
    std::shared_ptr<Node> left;
    Token op_tok;
    std::shared_ptr<Node> right;
};

struct IfStatement : Statement
{
    std::shared_ptr<Condition> condition;
    std::shared_ptr<StatementList> thenBlock;
    std::shared_ptr<StatementList> elseBlock; 
};

struct WhileStatement : Statement
{
    std::shared_ptr<Condition> condition;
    std::shared_ptr<StatementList> body;
};

struct PrintStatement : Statement
{
    bool isString = false;
    std::shared_ptr<Node> expression;
    std::string stringValue;
};

struct Assignment : Statement
{
    Token identifier;
    std::shared_ptr<Node> expression;
};

struct Declaration : Statement
{
    std::vector<Token> identifiers;
};