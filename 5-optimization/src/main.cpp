#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "tokens.hpp"
#include "ast.hpp"
#include "ir.hpp"


// If Flex's buffer type isn't visible, declare it.
    typedef struct yy_buffer_state *YY_BUFFER_STATE;
    YY_BUFFER_STATE yy_scan_string(const char* str);
    void yy_delete_buffer(YY_BUFFER_STATE b);

// Provided by Bison (parser.yy)
extern int yyparse(void);
extern std::shared_ptr<Node> g_ast_root; // The global AST root

static void printIndent(int indent)
{
    for (int i = 0; i < indent; ++i)
        std::cout << "  ";
}

static void printAST(const std::shared_ptr<Node> &node, int indent = 0)
{
    if (!node)
    {
        printIndent(indent);
        std::cout << "(null)\n";
        return;
    }

    if (auto num = std::dynamic_pointer_cast<NumberNode>(node))
    {
        printIndent(indent);
        std::cout << "Number: " << num->tok.value
                  << " (line " << num->tok.line << ")\n";
        return;
    }

    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
    {
        printIndent(indent);
        std::cout << "Identifier: " << id->tok.value
                  << " (line " << id->tok.line << ")\n";
        return;
    }

    if (auto bin = std::dynamic_pointer_cast<BinOpNode>(node))
    {
        printIndent(indent);
        std::cout << "BinOp: '" << bin->op_tok.value
                  << "' (line " << bin->op_tok.line << ")\n";
        printAST(bin->left,  indent + 1);
        printAST(bin->right, indent + 1);
        return;
    }

    if (auto st = std::dynamic_pointer_cast<Statement>(node))
    {
        printIndent(indent);
        std::cout << "StatementList\n";
        printIndent(indent + 1);
        std::cout << "Left:\n";
        printAST(st->left, indent + 2);
        printIndent(indent + 1);
        std::cout << "Right:\n";
        printAST(st->right, indent + 2);
        return;
    }

    if (auto cond = std::dynamic_pointer_cast<Condition>(node))
    {
        printIndent(indent);
        std::cout << "Condition (op: " << cond->comparison.value
                  << ", line " << cond->comparison.line << ")\n";
        printIndent(indent + 1);
        std::cout << "Left expr:\n";
        printAST(cond->left_expression, indent + 2);
        printIndent(indent + 1);
        std::cout << "Right expr:\n";
        printAST(cond->right_expression, indent + 2);
        return;
    }

    if (auto ifs = std::dynamic_pointer_cast<IfStatement>(node))
    {
        printIndent(indent);
        std::cout << "IfStatement\n";
        printIndent(indent + 1);
        std::cout << "Condition:\n";
        printAST(ifs->if_condition, indent + 2);
        printIndent(indent + 1);
        std::cout << "If body:\n";
        printAST(ifs->if_body, indent + 2);
        if (ifs->else_body)
        {
            printIndent(indent + 1);
            std::cout << "Else body:\n";
            printAST(ifs->else_body, indent + 2);
        }
        return;
    }

    if (auto w = std::dynamic_pointer_cast<WhileStatement>(node))
    {
        printIndent(indent);
        std::cout << "WhileStatement\n";
        printIndent(indent + 1);
        std::cout << "Condition:\n";
        printAST(w->condition, indent + 2);
        printIndent(indent + 1);
        std::cout << "Body:\n";
        printAST(w->body, indent + 2);
        return;
    }

    if (auto p = std::dynamic_pointer_cast<PrintStatement>(node))
    {
        printIndent(indent);
        std::cout << "PrintStatement (type: " << p->type << ")\n";
        if (p->type == "int")
        {
            printIndent(indent + 1);
            std::cout << "Expr:\n";
            printAST(p->intExpr, indent + 2);
        }
        else if (p->type == "string")
        {
            printIndent(indent + 1);
            std::cout << "String: \"" << p->strValue << "\"\n";
        }
        return;
    }

    if (auto asg = std::dynamic_pointer_cast<Assignment>(node))
    {
        printIndent(indent);
        std::cout << "Assignment to: " << asg->identifier.value
                  << " (line " << asg->identifier.line << ")\n";
        printIndent(indent + 1);
        std::cout << "Expr:\n";
        printAST(asg->expression, indent + 2);
        return;
    }

    if (auto decl = std::dynamic_pointer_cast<Declaration>(node))
    {
        printIndent(indent);
        std::cout << "Declaration of type: " << decl->declaration_type.value
                  << " (line " << decl->declaration_type.line << ")\n";
        printIndent(indent + 1);
        std::cout << "Identifiers:\n";
        for (const auto &tok : decl->identifiers)
        {
            printIndent(indent + 2);
            std::cout << tok.value << " (line " << tok.line << ")\n";
        }
        return;
    }

    printIndent(indent);
    std::cout << "<Unknown node type>\n";
}

int main()
{
    while (true)
    {
        std::ifstream fin("../read.txt");
        if (!fin) { std::cerr << "Cannot open read.txt\n"; return 1; }

        std::stringstream buffer;
        buffer << fin.rdbuf();
        std::string input = buffer.str();

        try
        {
            YY_BUFFER_STATE buf = yy_scan_string(input.c_str());
            int parse_result = yyparse();
            yy_delete_buffer(buf);

            if (parse_result == 0)
            {
                std::cout << "Parsing successful!\n";

                if (g_ast_root)
                {
                    std::cout << "AST (DFS traversal):\n";
                    printAST(g_ast_root);
                
                    IntermediateCodeGen irgen(g_ast_root);
                    auto gen = irgen.get();

                    std::cout << "\nGenerated IR:\n";

                    for (auto &instr : gen.code.code)
                    {
                        switch (instr->kind())
                        {
                            case IRKind::Assignment:
                            {
                                auto c = std::dynamic_pointer_cast<AssignmentCode>(instr);
                                if (c->op.empty())
                                    std::cout << c->var << " = " << c->left << "\n";
                                else
                                    std::cout << c->var << " = " << c->left << " " << c->op << " " << c->right << "\n";
                                break;
                            }

                            case IRKind::Jump:
                            {
                                auto c = std::dynamic_pointer_cast<JumpCode>(instr);
                                std::cout << "goto " << c->dist << "\n";
                                break;
                            }

                            case IRKind::Label:
                            {
                                auto c = std::dynamic_pointer_cast<LabelCode>(instr);
                                std::cout << c->label << ":\n";
                                break;
                            }

                            case IRKind::Compare:
                            {
                                auto c = std::dynamic_pointer_cast<CompareCodeIR>(instr);
                                std::cout << "IF " << c->left << " " << c->operation << " " << c->right
                                          << " GOTO " << c->jump << "\n";
                                break;
                            }

                            case IRKind::Print:
                            {
                                auto c = std::dynamic_pointer_cast<PrintCodeIR>(instr);
                                std::cout << "PRINT " << c->type << " " << c->value << "\n";
                                break;
                            }
                        }
                    }
                }
                
                else
                {
                    std::cout << "AST is empty.\n";
                }

                //auto root = g_ast_root;
                //IntermediateCodeGen irgen(root);
                //auto gen = irgen.get();
            }
            
            else
            {
                std::cerr << "Parsing failed.\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << "\n";
        }
        std::cout << "------------------------------\n";
        std::string dummy;
        std::getline(std::cin, dummy);
    }
}