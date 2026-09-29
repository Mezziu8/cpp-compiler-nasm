#include "parser.hpp"
#include <stdexcept>

static bool is_value(const Token &t, const char *v) { return t.value == v; }

Parser::Parser(TokenArray tokens) : tokens(std::move(tokens))
{
    this->tokens.appendEndIfMissing();
}

void Parser::read_token_pass(const std::string &expected, const std::string &message)
{
    const Token &t = tokens.current();
    if (t.value != expected)
        throw std::runtime_error(message + " in line " + std::to_string(t.line));
    tokens.next();
}

std::shared_ptr<Node> Parser::factor()
{
    const Token &tok = tokens.current();
    if (tok.type == TokenType::IntLit)
    {
        tokens.next();
        return std::make_shared<NumberNode>(tok);
    }
    if (tok.type == TokenType::Var)
    {
        tokens.next();
        return std::make_shared<IdentifierNode>(tok);
    }
    if (is_value(tok, "("))
    {
        tokens.next();
        auto e = expr();
        if (!is_value(tokens.current(), ")"))
            throw std::runtime_error("Expected ) in line " + std::to_string(tokens.current().line));
        tokens.next();
        return e;
    }
    throw std::runtime_error("Syntax Error: Expected Integer in line " + std::to_string(tok.line));
}

std::shared_ptr<Node> Parser::term()
{
    auto left = factor();
    
    while (tokens.current().value == "*" || tokens.current().value == "/")
    {
        Token op = tokens.current();
        tokens.next();

        auto right = factor();

        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;

        left = bin;
    }

    return left;
}

std::shared_ptr<Node> Parser::expr()
{
    auto left = term();
    while (tokens.current().value == "+" || tokens.current().value == "-")
    {
        Token op = tokens.current();
        tokens.next();
        auto right = term();
        auto bin = std::make_shared<BinOpNode>();
        bin->left = left;
        bin->op_tok = op;
        bin->right = right;
        left = bin;
    }
    return left;
}

std::shared_ptr<Condition> Parser::condition()
{
    auto leftExpr = expr();

    Token op = tokens.current();
    if (op.type != TokenType::Comparison)
        throw std::runtime_error("Expected comparison operator in line " + std::to_string(op.line));
    tokens.next();

    auto rightExpr = expr();

    auto cond = std::make_shared<Condition>();
    cond->left = leftExpr;
    cond->op_tok = op;
    cond->right = rightExpr;
    return cond;
}

std::shared_ptr<Node> Parser::if_statement()
{
    tokens.next();

    if (tokens.current().value != "(")
        throw std::runtime_error("Expected '(' after if in line " + std::to_string(tokens.current().line));
    tokens.next();

    auto cond = condition();

    if (tokens.current().value != ")")
        throw std::runtime_error("Expected ')' in line " + std::to_string(tokens.current().line));
    tokens.next();

    if (tokens.current().value != "{")
        throw std::runtime_error("Expected '{' in line " + std::to_string(tokens.current().line));
    tokens.next();

    auto thenBlock = std::dynamic_pointer_cast<StatementList>(statements());

    if (tokens.current().value != "}")
        throw std::runtime_error("Expected '}' in then-block in line " + std::to_string(tokens.current().line));
    tokens.next();

    std::shared_ptr<StatementList> elseBlock = nullptr;

    if (tokens.current().value == "else")
    {
        tokens.next();
        if (tokens.current().value != "{")
            throw std::runtime_error("Expected '{' after else in line " + std::to_string(tokens.current().line));
        tokens.next();

        elseBlock = std::dynamic_pointer_cast<StatementList>(statements());

        if (tokens.current().value != "}")
            throw std::runtime_error("Expected '}' in else-block in line " + std::to_string(tokens.current().line));
        tokens.next();
    }

    auto node = std::make_shared<IfStatement>();
    node->condition = cond;
    node->thenBlock = thenBlock;
    node->elseBlock = elseBlock;

    return node;
}

std::shared_ptr<Node> Parser::declarations()
{
    tokens.next();
    
    auto decl = std::make_shared<Declaration>();

    if (tokens.current().type != TokenType::Var)
        throw std::runtime_error("Expected identifier after 'int' in line " + std::to_string(tokens.current().line));

    decl->identifiers.push_back(tokens.current());
    tokens.next();

    while (tokens.current().value == ",")
    {
        tokens.next();

        if (tokens.current().type != TokenType::Var)
            throw std::runtime_error("Expected identifier after ',' in line " + std::to_string(tokens.current().line));

        decl->identifiers.push_back(tokens.current());
        tokens.next();
    }

    if (tokens.current().value != ";")
        throw std::runtime_error("Expected ';' after declaration in line " + std::to_string(tokens.current().line));

    tokens.next();
    return decl;
}

std::shared_ptr<Node> Parser::assignment()
{
    Token identifier = tokens.current();
    tokens.next();
    read_token_pass("=", "Missing =");
    auto expression = expr();
    read_token_pass(";", "Missing ;");
    auto a = std::make_shared<Assignment>();
    a->identifier = identifier;
    a->expression = expression;
    return a;
}

std::shared_ptr<Node> Parser::while_statement()
{
    tokens.next();

    if (tokens.current().value != "(")
        throw std::runtime_error("Expected '(' after 'while' in line " + std::to_string(tokens.current().line));
    tokens.next();
    
    auto cond = condition();

    if (tokens.current().value != ")")
        throw std::runtime_error("Expected ')' after condition in line " + std::to_string(tokens.current().line));
    tokens.next();

     if (tokens.current().value != "{")
        throw std::runtime_error("Expected '{' in while-statement in line " + std::to_string(tokens.current().line));
    tokens.next();

    auto body = std::dynamic_pointer_cast<StatementList>(statements());

    if (tokens.current().value != "}")
        throw std::runtime_error("Expected '}' at the end of while-block in line " + std::to_string(tokens.current().line));
    tokens.next(); 

    auto node = std::make_shared<WhileStatement>();
    node->condition = cond;
    node->body = body;

    return node;
}

std::shared_ptr<Node> Parser::printing(const std::string &type)
{
    tokens.next();

    if (tokens.current().value != "(")
        throw std::runtime_error("Expected '(' after print in line " + std::to_string(tokens.current().line));
    tokens.next();

    auto node = std::make_shared<PrintStatement>();

    if (type == "print")
    {
        node->isString = false;
        node->expression = expr();
    }
    else
    {
        if (tokens.current().type != TokenType::String)
            throw std::runtime_error("Expected string literal in prints in line " + std::to_string(tokens.current().line));
        
        node->isString = true;
        node->stringValue = tokens.current().value;
        tokens.next();
    }

    if (tokens.current().value != ")")
        throw std::runtime_error("Expected ')' after print argument in line " + std::to_string(tokens.current().line));
    tokens.next(); 

    if (tokens.current().value != ";")
        throw std::runtime_error("Expected ';' after print in line " + std::to_string(tokens.current().line));
    tokens.next();

    return node;
}

std::shared_ptr<Node> Parser::statements()
{
    auto list = std::make_shared<StatementList>();

    Token t = tokens.current();

    while (t.value != "END" && t.value != "}")
    {
        std::shared_ptr<Statement> stmt;

        if (t.type == TokenType::If) {
            stmt = std::dynamic_pointer_cast<Statement>(if_statement());
        }
        else if (t.type == TokenType::While) {
            stmt = std::dynamic_pointer_cast<Statement>(while_statement());
        }
        else if (t.type == TokenType::Int) {
            stmt = std::dynamic_pointer_cast<Statement>(declarations());
        }
        else if (t.type == TokenType::Print) {
            stmt = std::dynamic_pointer_cast<Statement>(printing("print"));
        }
        else if (t.type == TokenType::Prints) {
            stmt = std::dynamic_pointer_cast<Statement>(printing("prints"));
        }
        else if (t.type == TokenType::Var) {
            stmt = std::dynamic_pointer_cast<Statement>(assignment());
        }
        else {
            throw std::runtime_error("Unexpected token in statement list at line " + std::to_string(t.line));
        }

        list->statements.push_back(stmt);

        t = tokens.current();
    }

    return list;
}

std::shared_ptr<Node> Parser::get_root()
{
    auto program = std::make_shared<Program>();
    program->body = std::dynamic_pointer_cast<StatementList>(statements());

    if (tokens.current().type != TokenType::End)
        throw std::runtime_error("Unexpected extra input at the end of program");

    return program;
}