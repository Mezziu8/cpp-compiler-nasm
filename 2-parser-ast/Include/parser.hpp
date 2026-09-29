#pragma once
#include <memory>
#include "tokens.hpp"
#include "ast.hpp"

class Parser
{
public:
    explicit Parser(TokenArray tokens);
    std::shared_ptr<Node> get_root();

private:
    void read_token_pass(const std::string &expected, const std::string &message);
    std::shared_ptr<Node> factor();
    std::shared_ptr<Node> term();
    std::shared_ptr<Node> expr();
    std::shared_ptr<Condition> condition();
    std::shared_ptr<Node> if_statement();
    std::shared_ptr<Node> declarations();
    std::shared_ptr<Node> assignment();
    std::shared_ptr<Node> while_statement();
    std::shared_ptr<Node> printing(const std::string &type);
    std::shared_ptr<Node> statements();

private:
    TokenArray tokens;
    int dele{0};
};
