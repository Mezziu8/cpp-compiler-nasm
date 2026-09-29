#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "tokens.hpp"
#include "parser.hpp"
#include <queue>

extern void scan_string_to_tokens(const std::string &src, std::vector<Token> &out);

void print_ast(const std::shared_ptr<Node>& node, std::string prefix = "", bool isLast = true) 
{
    if (!node)
        return;
        
    std::cout << prefix;

    if (isLast)
        std::cout << "`-- ";
    else
        std::cout << "|-- ";

    if (auto p = std::dynamic_pointer_cast<Program>(node))
    {
        std::cout << "Program\n";
        print_ast(p->body, prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    if (auto sl = std::dynamic_pointer_cast<StatementList>(node))
    {
        std::cout <<"StatementList\n";
        for (size_t i = 0; i < sl->statements.size(); ++i)
        {   
            print_ast(sl->statements[i],
            prefix + (isLast ? "    " : "|   "),
            i == sl->statements.size() - 1);
        }   
        return;
    }

    if (auto n = std::dynamic_pointer_cast<NumberNode>(node))
    {
        std::cout << "Number(" <<n->getValue() << ")\n";
        return;
    }

    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
    {
        std::cout << "Identifier(" << id->getValue() << ")\n";
        return;
    }

    if (auto b = std::dynamic_pointer_cast<BinOpNode>(node))
    {
        std::cout << "BinOp(" << b->op_tok.value << ")\n";
        print_ast(b->left, prefix + (isLast ? "    " : "|   "), false);
        print_ast(b->right, prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    if (auto a = std::dynamic_pointer_cast<Assignment>(node))
    {
        std::cout << "Assignment(" << a->identifier.value << ")\n";
        print_ast(a->expression, prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    if (auto d = std::dynamic_pointer_cast<Declaration>(node))
    {
        std::cout << "Declaration(";
        for (size_t i = 0; i < d->identifiers.size(); ++i)
        {
            std::cout << d->identifiers[i].value;
            if (i + 1 < d->identifiers.size()) std::cout << ", ";
        }
        std::cout << ")\n";
        return;
    }

    if (auto w = std::dynamic_pointer_cast<WhileStatement>(node))
    {
        std::cout << "While\n";
        print_ast(w->condition, prefix + (isLast ? "    " : "|   "), false);
        print_ast(w->body,      prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    if (auto i = std::dynamic_pointer_cast<IfStatement>(node))
    {
        std::cout << "If\n";
        print_ast(i->condition, prefix + (isLast ? "    " : "|   "), false);

        print_ast(i->thenBlock, prefix + (isLast ? "    " : "|   "),
                  i->elseBlock == nullptr);

        if (i->elseBlock)
            print_ast(i->elseBlock,
                      prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    if (auto p = std::dynamic_pointer_cast<PrintStatement>(node))
    {
        if (p->isString)
        {
            std::cout << "PrintString(\"" << p->stringValue << "\")\n";
        }
        else
        {
            std::cout << "PrintExpr\n";
            print_ast(p->expression, prefix + (isLast ? "    " : "|   "), true);
        }
        return;
    }

    if (auto c = std::dynamic_pointer_cast<Condition>(node))
    {
        std::cout << "Condition(" << c->op_tok.value << ")\n";
        print_ast(c->left,  prefix + (isLast ? "    " : "|   "), false);
        print_ast(c->right, prefix + (isLast ? "    " : "|   "), true);
        return;
    }

    std::cout << "UnknownNode\n";
}

void print_ast_pretty_tree(const std::shared_ptr<Node>& node, int indent = 0)
{
    if (!node) return;

    auto label_of = [](const std::shared_ptr<Node>& n) -> std::string {
        if (!n) return "(null)";

        if (auto p = std::dynamic_pointer_cast<Program>(n)) return "Program";
        if (auto sl = std::dynamic_pointer_cast<StatementList>(n)) return "StmtList";

        if (auto d = std::dynamic_pointer_cast<Declaration>(n)) return "Decl";

        if (auto a = std::dynamic_pointer_cast<Assignment>(n))
            return "Assign(" + a->identifier.value + ")";

        if (auto bin = std::dynamic_pointer_cast<BinOpNode>(n))
            return "Op(" + bin->op_tok.value + ")";

        if (auto c = std::dynamic_pointer_cast<Condition>(n))
            return "Cond(" + c->op_tok.value + ")";

        if (auto w = std::dynamic_pointer_cast<WhileStatement>(n)) return "While";
        if (auto i = std::dynamic_pointer_cast<IfStatement>(n)) return "If";

        if (auto pr = std::dynamic_pointer_cast<PrintStatement>(n))
            return pr->isString ? "PrintS(\"" + pr->stringValue + "\")"
                                : "Print";

        if (auto id = std::dynamic_pointer_cast<IdentifierNode>(n))
            return "Id(" + id->getValue() + ")";

        if (auto num = std::dynamic_pointer_cast<NumberNode>(n))
        {
            std::ostringstream oss;
            oss << "Num(" << num->getValue() << ")";
            return oss.str();
        }

        return "?";
    };

    auto children_of = [&](const std::shared_ptr<Node>& n) -> std::vector<std::shared_ptr<Node>> {
        std::vector<std::shared_ptr<Node>> v;

        if (auto p = std::dynamic_pointer_cast<Program>(n))
        {
            if (p->body) v.push_back(p->body);
            return v;
        }

        if (auto sl = std::dynamic_pointer_cast<StatementList>(n))
        {
            std::vector<std::shared_ptr<Node>> out;
            for (auto &stmt : sl->statements)
                out.push_back(stmt);
            return out;
        }

        if (auto d = std::dynamic_pointer_cast<Declaration>(n))
            return {};

        if (auto a = std::dynamic_pointer_cast<Assignment>(n))
        {
            v.push_back(std::make_shared<IdentifierNode>(a->identifier));
            v.push_back(a->expression);
            return v;
        }

        if (auto bin = std::dynamic_pointer_cast<BinOpNode>(n))
        {
            v.push_back(bin->left);
            v.push_back(bin->right);
            return v;
        }

        if (auto c = std::dynamic_pointer_cast<Condition>(n))
        {
            v.push_back(c->left);
            v.push_back(c->right);
            return v;
        }

        if (auto i = std::dynamic_pointer_cast<IfStatement>(n))
        {
            v.push_back(i->condition);
            v.push_back(i->thenBlock);
            if (i->elseBlock) v.push_back(i->elseBlock);
            return v;
        }

        if (auto w = std::dynamic_pointer_cast<WhileStatement>(n))
        {
            v.push_back(w->condition);
            v.push_back(w->body);
            return v;
        }

        if (auto p = std::dynamic_pointer_cast<PrintStatement>(n))
        {
            if (!p->isString) v.push_back(p->expression);
            return v;
        }

        return {};
    };

    std::string label = label_of(node);
    std::cout << std::string(indent, ' ') << label << "\n";

    auto ch = children_of(node);
    if (ch.empty()) return;

    int mid = indent + label.size() / 2;

    std::cout << std::string(mid, ' ') << "|\n";

    for (size_t i = 0; i < ch.size(); i++)
    {
        std::cout << std::string(mid, ' ') << (i + 1 == ch.size() ? "`-- " : "|-- ");
        print_ast_pretty_tree(ch[i], mid + 4);
    }
}

#include <unordered_map>

struct StatementList;
struct Declaration;
struct Assignment;
struct PrintStatement;
struct IfStatement;
struct WhileStatement;
struct Condition;
struct BinOpNode;
struct IdentifierNode;
struct NumberNode;
struct Program;


void checkNode(const std::shared_ptr<Node>& node,
               std::unordered_map<std::string, bool>& declared);

void checkStatementList(const std::shared_ptr<StatementList>& sl,
                        std::unordered_map<std::string, bool>& declared);

void checkDeclaration(const std::shared_ptr<Declaration>& d,
                      std::unordered_map<std::string, bool>& declared);

void checkStatementList(const std::shared_ptr<StatementList>& sl,
                        std::unordered_map<std::string, bool>& declared)
{
    if (!sl) return;
    for (auto& stmt : sl->statements)
        checkNode(stmt, declared);
}

void checkAssignment(const std::shared_ptr<Assignment>& a,
                     std::unordered_map<std::string, bool>& declared);

void checkPrint(const std::shared_ptr<PrintStatement>& p,
                std::unordered_map<std::string, bool>& declared);

void checkIf(const std::shared_ptr<IfStatement>& is,
             std::unordered_map<std::string, bool>& declared);

void checkWhile(const std::shared_ptr<WhileStatement>& ws,
                std::unordered_map<std::string, bool>& declared);

void ensureDeclared(const std::string& name,
                    const std::unordered_map<std::string, bool>& declared,
                    int line)
{
    if (!declared.count(name))
        throw std::runtime_error(
            "Semantic error: variable '" + name +
            "' used before declaration (line " +
            std::to_string(line) + ")"
        );
}

void checkDeclaration(const std::shared_ptr<Declaration>& d,
                      std::unordered_map<std::string, bool>& declared)
{
    for (auto& tok : d->identifiers)
    {
        if (declared.count(tok.value))
            throw std::runtime_error(
                "Semantic error: variable '" + tok.value +
                "' redeclared (line " +
                std::to_string(tok.line) + ")"
            );

        declared[tok.value] = true;
    }
}

void checkAssignment(const std::shared_ptr<Assignment>& a,
                     std::unordered_map<std::string, bool>& declared)
{
    ensureDeclared(a->identifier.value, declared, a->identifier.line);

    if (a->expression)
        checkNode(a->expression, declared);
}

void checkPrint(const std::shared_ptr<PrintStatement>& p,
                std::unordered_map<std::string, bool>& declared)
{
    if (p->isString)
        return;

    if (p->expression)
        checkNode(p->expression, declared);
}

void checkIf(const std::shared_ptr<IfStatement>& is,
             std::unordered_map<std::string, bool>& declared)
{
    checkNode(is->condition, declared);

    checkStatementList(is->thenBlock, declared);

    if (is->elseBlock)
        checkStatementList(is->elseBlock, declared);
}

void checkWhile(const std::shared_ptr<WhileStatement>& ws,
                std::unordered_map<std::string, bool>& declared)
{
    checkNode(ws->condition, declared);

    checkStatementList(ws->body, declared);
}

void checkNode(const std::shared_ptr<Node>& node,
               std::unordered_map<std::string, bool>& declared)
{
    if (!node) return;

    if (auto prog = std::dynamic_pointer_cast<Program>(node))
    {
        if (prog->body)
            return checkNode(prog->body, declared);
        return;
    }

    if (auto sl = std::dynamic_pointer_cast<StatementList>(node))
        return checkStatementList(sl, declared);

    if (auto d = std::dynamic_pointer_cast<Declaration>(node))
        return checkDeclaration(d, declared);

    if (auto a = std::dynamic_pointer_cast<Assignment>(node))
        return checkAssignment(a, declared);

    if (auto p = std::dynamic_pointer_cast<PrintStatement>(node))
        return checkPrint(p, declared);

    if (auto is = std::dynamic_pointer_cast<IfStatement>(node))
        return checkIf(is, declared);

    if (auto ws = std::dynamic_pointer_cast<WhileStatement>(node))
        return checkWhile(ws, declared);

    if (auto cond = std::dynamic_pointer_cast<Condition>(node))
    {
        if (cond->left)
            checkNode(cond->left, declared);
        if (cond->right)
            checkNode(cond->right, declared);
        return;
    }

    if (auto bin = std::dynamic_pointer_cast<BinOpNode>(node))
    {
        if (bin->left)
            checkNode(bin->left, declared);
        if (bin->right)
            checkNode(bin->right, declared);
        return;
    }

    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
    {
        ensureDeclared(id->tok.value, declared, id->tok.line);
        return;
    }

    if (std::dynamic_pointer_cast<NumberNode>(node))
        return;
}


int main()
{
    std::ifstream fin("../read.txt");
    if (!fin)
    {
        std::cerr << "Cannot open read.txt\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << fin.rdbuf();
    std::string input = buffer.str();
    std::vector<Token> toks;
    try
    {
        scan_string_to_tokens(input, toks);
        TokenArray arr;
        for (auto &t : toks)
            arr.push(t);
        for (auto &t : toks)
            std::cout << t.value << " ";
        std::cout << "\n";
        Parser parser(arr);
        auto root = parser.get_root();
        std::cout << "\n--- Pretty AST (Y-diagram) ---\n";
        print_ast_pretty_tree(root);

        std::cout << "\n--- Tree AST ---\n";
        print_ast(root);

        std::unordered_map<std::string, bool> declared;
        try 
        {
            checkNode(root, declared);
            std::cout << "\n[Semantic check OK]\n";
        } 
        catch (const std::exception &e)
        {
            std::cerr << e.what() << "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
    std::cout << "\n";
}