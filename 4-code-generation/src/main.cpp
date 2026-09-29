#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "tokens.hpp"
#include "ast.hpp"
#include "ir.hpp"
#include "codegen.hpp"

// If Flex's buffer type isn't visible, declare it.
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *str);
void yy_delete_buffer(YY_BUFFER_STATE b);

// Provided by Bison (parser.yy)
extern int yyparse(void);
extern std::shared_ptr<Node> g_ast_root; // The global AST root

static void printIndent(int indent)
{
    for (int i = 0; i < indent; ++i)
        std::cerr << "  ";
}

static void printAST(const std::shared_ptr<Node> &node, int indent = 0)
{
    if (!node)
    {
        printIndent(indent);
        std::cerr << "(null)\n";
        return;
    }

    if (auto num = std::dynamic_pointer_cast<NumberNode>(node))
    {
        printIndent(indent);
        std::cerr << "Number: " << num->tok.value
                  << " (line " << num->tok.line << ")\n";
        return;
    }

    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(node))
    {
        printIndent(indent);
        std::cerr << "Identifier: " << id->tok.value
                  << " (line " << id->tok.line << ")\n";
        return;
    }

    if (auto bin = std::dynamic_pointer_cast<BinOpNode>(node))
    {
        printIndent(indent);
        std::cerr << "BinOp: '" << bin->op_tok.value
                  << "' (line " << bin->op_tok.line << ")\n";
        printAST(bin->left, indent + 1);
        printAST(bin->right, indent + 1);
        return;
    }

    if (auto st = std::dynamic_pointer_cast<Statement>(node))
    {
        printIndent(indent);
        std::cerr << "StatementList\n";
        printIndent(indent + 1);
        std::cerr << "Left:\n";
        printAST(st->left, indent + 2);
        printIndent(indent + 1);
        std::cerr << "Right:\n";
        printAST(st->right, indent + 2);
        return;
    }

    if (auto cond = std::dynamic_pointer_cast<Condition>(node))
    {
        printIndent(indent);
        std::cerr << "Condition (op: " << cond->comparison.value
                  << ", line " << cond->comparison.line << ")\n";
        printIndent(indent + 1);
        std::cerr << "Left expr:\n";
        printAST(cond->left_expression, indent + 2);
        printIndent(indent + 1);
        std::cerr << "Right expr:\n";
        printAST(cond->right_expression, indent + 2);
        return;
    }

    if (auto ifs = std::dynamic_pointer_cast<IfStatement>(node))
    {
        printIndent(indent);
        std::cerr << "IfStatement\n";
        printIndent(indent + 1);
        std::cerr << "Condition:\n";
        printAST(ifs->if_condition, indent + 2);
        printIndent(indent + 1);
        std::cerr << "If body:\n";
        printAST(ifs->if_body, indent + 2);
        if (ifs->else_body)
        {
            printIndent(indent + 1);
            std::cerr << "Else body:\n";
            printAST(ifs->else_body, indent + 2);
        }
        return;
    }

    if (auto w = std::dynamic_pointer_cast<WhileStatement>(node))
    {
        printIndent(indent);
        std::cerr << "WhileStatement\n";
        printIndent(indent + 1);
        std::cerr << "Condition:\n";
        printAST(w->condition, indent + 2);
        printIndent(indent + 1);
        std::cerr << "Body:\n";
        printAST(w->body, indent + 2);
        return;
    }

    if (auto p = std::dynamic_pointer_cast<PrintStatement>(node))
    {
        printIndent(indent);
        std::cerr << "PrintStatement (type: " << p->type << ")\n";
        if (p->type == "int")
        {
            printIndent(indent + 1);
            std::cerr << "Expr:\n";
            printAST(p->intExpr, indent + 2);
        }
        else if (p->type == "string")
        {
            printIndent(indent + 1);
            std::cerr << "String: \"" << p->strValue << "\"\n";
        }
        return;
    }

    if (auto asg = std::dynamic_pointer_cast<Assignment>(node))
    {
        printIndent(indent);
        std::cerr << "Assignment to: " << asg->identifier.value
                  << " (line " << asg->identifier.line << ")\n";
        printIndent(indent + 1);
        std::cerr << "Expr:\n";
        printAST(asg->expression, indent + 2);
        return;
    }

    if (auto decl = std::dynamic_pointer_cast<Declaration>(node))
    {
        printIndent(indent);
        std::cerr << "Declaration of type: " << decl->declaration_type.value
                  << " (line " << decl->declaration_type.line << ")\n";
        printIndent(indent + 1);
        std::cerr << "Identifiers:\n";
        for (const auto &tok : decl->identifiers)
        {
            printIndent(indent + 2);
            std::cerr << tok.value << " (line " << tok.line << ")\n";
        }
        return;
    }

    printIndent(indent);
    std::cerr << "<Unknown node type>\n";
}

static void printIR(const GeneratedIR &gen)
{
    for (auto &instr : gen.code.code)
    {
        switch (instr->kind())
        {
        case IRKind::Assignment:
        {
            auto c = std::dynamic_pointer_cast<AssignmentCode>(instr);
            if (c->op.empty())
                std::cerr << c->var << " = " << c->left << "\n";
            else
                std::cerr << c->var << " = " << c->left << " " << c->op << " " << c->right << "\n";
            break;
        }
        case IRKind::Jump:
        {
            auto c = std::dynamic_pointer_cast<JumpCode>(instr);
            std::cerr << "goto " << c->dist << "\n";
            break;
        }
        case IRKind::Label:
        {
            auto c = std::dynamic_pointer_cast<LabelCode>(instr);
            std::cerr << c->label << ":\n";
            break;
        }
        case IRKind::Compare:
        {
            auto c = std::dynamic_pointer_cast<CompareCodeIR>(instr);
            std::cerr << "IF " << c->left << " " << c->operation << " " << c->right
                      << " GOTO " << c->jump << "\n";
            break;
        }
        case IRKind::Print:
        {
            auto c = std::dynamic_pointer_cast<PrintCodeIR>(instr);
            std::cerr << "PRINT " << c->type << " " << c->value << "\n";
            break;
        }
        }
    }
}

static std::filesystem::path exe_dir(char **argv)
{
    // On macOS/Linux this is usually a usable path (relative or absolute).
    // If it's relative, we resolve it against current_path().
    std::filesystem::path p = argv[0] ? std::filesystem::path(argv[0]) : std::filesystem::path();
    if (p.empty())
        return std::filesystem::current_path();
    if (p.is_relative())
        p = std::filesystem::current_path() / p;
    p = std::filesystem::weakly_canonical(p);
    return p.parent_path();
}

static bool read_file_to_string(const std::filesystem::path &p, std::string &out)
{
    std::ifstream fin(p);
    if (!fin)
        return false;
    std::stringstream buffer;
    buffer << fin.rdbuf();
    out = buffer.str();
    return true;
}

int main(int argc, char **argv)
{
    namespace fs = std::filesystem;

    // We want this to work whether you run:
    // - ./build/compiler            (read.txt is in project root)
    // - ./build/compiler read.txt   (explicit)
    // - ./compiler                  (if you run from root and compiler is there)
    //
    // So we try a few sane candidates when argv[1] is not provided.
    fs::path inputPath;
    std::vector<fs::path> candidates;

    if (argc > 1)
    {
        inputPath = fs::path(argv[1]);
        candidates.push_back(inputPath);
    }
    else
    {
        candidates.push_back(fs::path("read.txt"));                 // current working dir
        candidates.push_back(fs::path("..") / "read.txt");          // common when running from build/
        candidates.push_back(exe_dir(argv) / "read.txt");           // next to executable
        candidates.push_back(exe_dir(argv) / ".." / "read.txt");    // executable in build/, file in root
    }

    std::string input;
    fs::path chosen;

    for (const auto &c : candidates)
    {
        fs::path abs = c.is_relative() ? (fs::current_path() / c) : c;
        abs = fs::weakly_canonical(abs);
        if (read_file_to_string(abs, input))
        {
            chosen = abs;
            break;
        }
    }

    if (chosen.empty())
    {
        std::cerr << "Cannot open read.txt.\n";
        std::cerr << "Tried these paths:\n";
        for (const auto &c : candidates)
        {
            fs::path abs = c.is_relative() ? (fs::current_path() / c) : c;
            std::cerr << "  " << abs.string() << "\n";
        }
        std::cerr << "Tip: run with an explicit path, e.g. ./build/compiler ./read.txt\n";
        return 1;
    }

    // Write output.asm in the SAME directory as the input file (so it ends up in project root
    // when read.txt is in project root, even if you run the compiler from build/).
    fs::path outAsmPath = chosen.parent_path() / "output.asm";

    try
    {
        YY_BUFFER_STATE buf = yy_scan_string(input.c_str());
        int parse_result = yyparse();
        yy_delete_buffer(buf);

        if (parse_result != 0)
        {
            std::cerr << "Parsing failed.\n";
            return 1;
        }

        std::cerr << "Parsing successful!\n";

        if (!g_ast_root)
        {
            std::cerr << "AST is empty.\n";
            return 1;
        }

        // Optional debug (kept on stderr so it never pollutes output.asm)
        std::cerr << "AST (DFS traversal):\n";
        printAST(g_ast_root);

        IntermediateCodeGen irgen(g_ast_root);
        GeneratedIR gen = irgen.get();

        std::cerr << "\nGenerated IR:\n";
        printIR(gen);
        std::cerr << "------------------------------\n";

        // === CODEGEN: generate output.asm ===
        std::unordered_map<std::string, std::string> emptyTempMap;
        CodeGenerator cg(gen.code, gen.identifiers, gen.constants, emptyTempMap);
        cg.writeAsm(outAsmPath.string());

        std::cerr << "Wrote " << outAsmPath.string() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}