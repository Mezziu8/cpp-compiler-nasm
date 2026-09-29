#include "ir.hpp"
#include <cctype>
#include <unordered_map>
#include <vector>

static std::shared_ptr<AssignmentCode> make_assign(const std::string &v,
                                                  const std::string &l,
                                                  const std::string &op,
                                                  const std::string &r)
{
    auto a = std::make_shared<AssignmentCode>();
    a->var = v;
    a->left = l;
    a->op = op;
    a->right = r;
    return a;
}

static std::shared_ptr<JumpCode> make_jump(const std::string &d)
{
    auto j = std::make_shared<JumpCode>();
    j->dist = d;
    return j;
}

static std::shared_ptr<LabelCode> make_label(const std::string &l)
{
    auto x = std::make_shared<LabelCode>();
    x->label = l;
    return x;
}

static std::shared_ptr<CompareCodeIR> make_compare(const std::string &l,
                                                   const std::string &op,
                                                   const std::string &r,
                                                   const std::string &j)
{
    auto c = std::make_shared<CompareCodeIR>();
    c->left = l;
    c->operation = op;
    c->right = r;
    c->jump = j;
    return c;
}

static std::shared_ptr<PrintCodeIR> make_print(const std::string &t, const std::string &v)
{
    auto p = std::make_shared<PrintCodeIR>();
    p->type = t;
    p->value = v;
    return p;
}

static bool is_int_lit(const std::string &s)
{
    if (s.empty())
        return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size())
        return false;
    for (; i < s.size(); ++i)
        if (!std::isdigit((unsigned char)s[i]))
            return false;
    return true;
}

static long long eval_arith(long long a, const std::string &op, long long b)
{
    if (op == "+")
        return a + b;
    if (op == "-")
        return a - b;
    if (op == "*")
        return a * b;
    if (op == "/")
        return (b == 0) ? 0 : (a / b);
    return 0;
}

static bool eval_cmp(long long a, const std::string &op, long long b)
{
    if (op == "<")
        return a < b;
    if (op == "<=")
        return a <= b;
    if (op == ">")
        return a > b;
    if (op == ">=")
        return a >= b;
    if (op == "==")
        return a == b;
    if (op == "!=")
        return a != b;
    return false;
}

static std::string canon_expr(const std::string &l, const std::string &op, const std::string &r)
{
    if (op == "+" || op == "*")
        return (l <= r) ? (l + op + r) : (r + op + l);
    return l + op + r;
}

static void optimize_ir(InterCodeArray &arr)
{
    std::unordered_map<std::string, std::string> constVal;

    std::unordered_map<std::string, std::string> expr2tmp;

    auto reset_block = [&]() { expr2tmp.clear(); };

    std::vector<std::shared_ptr<IRInstr>> out;
    out.reserve(arr.code.size());

    for (auto &ins : arr.code)
    {
        if (ins->kind() == IRKind::Label || ins->kind() == IRKind::Jump)
            reset_block();

        if (ins->kind() == IRKind::Assignment)
        {
            auto a = std::static_pointer_cast<AssignmentCode>(ins);

            if (constVal.count(a->left))
                a->left = constVal[a->left];
            if (constVal.count(a->right))
                a->right = constVal[a->right];

            if (!a->op.empty() && is_int_lit(a->left) && is_int_lit(a->right))
            {
                long long x = std::stoll(a->left);
                long long y = std::stoll(a->right);
                long long res = eval_arith(x, a->op, y);

                a->left = std::to_string(res);
                a->op.clear();
                a->right.clear();
            }

            if (!a->op.empty())
            {
                std::string key = canon_expr(a->left, a->op, a->right);
                auto it = expr2tmp.find(key);
                if (it != expr2tmp.end())
                {
                    a->left = it->second;
                    a->op.clear();
                    a->right.clear();
                }
                else
                {
                    expr2tmp[key] = a->var;
                }
            }

            if (a->op.empty() && is_int_lit(a->left))
                constVal[a->var] = a->left;
            else
                constVal.erase(a->var);

            out.push_back(ins);
            continue;
        }

        if (ins->kind() == IRKind::Compare)
        {
            auto c = std::static_pointer_cast<CompareCodeIR>(ins);

            if (constVal.count(c->left))
                c->left = constVal[c->left];
            if (constVal.count(c->right))
                c->right = constVal[c->right];
            if (is_int_lit(c->left) && is_int_lit(c->right))
            {
                long long a = std::stoll(c->left);
                long long b = std::stoll(c->right);
                bool take = eval_cmp(a, c->operation, b);

                if (take)
                {
                    auto j = std::make_shared<JumpCode>();
                    j->dist = c->jump;
                    out.push_back(j);
                }
                continue;
            }

            out.push_back(ins);
            continue;
        }

        if (ins->kind() == IRKind::Print)
        {
            auto p = std::static_pointer_cast<PrintCodeIR>(ins);

            if (p->type == "int" && constVal.count(p->value))
                p->value = constVal[p->value];

            out.push_back(ins);
            continue;
        }

        out.push_back(ins);
    }

    arr.code.swap(out);
}

IntermediateCodeGen::IntermediateCodeGen(const std::shared_ptr<Node> &root) : root(root)
{
    exec_statement(root);
}

GeneratedIR IntermediateCodeGen::get()
{
    optimize_ir(arr);
    return GeneratedIR{arr, identifiers, constants};
}

std::string IntermediateCodeGen::nextTemp() { return "T" + std::to_string(tCounter++); }
std::string IntermediateCodeGen::nextLabel() { return "L" + std::to_string(lCounter++); }
std::string IntermediateCodeGen::currentLabel() const { return "L" + std::to_string(lCounter); }
std::string IntermediateCodeGen::nextStringSym() { return "S" + std::to_string(sCounter++); }

std::string IntermediateCodeGen::exec_expr(const std::shared_ptr<Node> &n)
{
    if (auto id = std::dynamic_pointer_cast<IdentifierNode>(n))
        return id->getValue();

    if (auto num = std::dynamic_pointer_cast<NumberNode>(n))
        return num->getValue();

    auto bin = std::dynamic_pointer_cast<BinOpNode>(n);
    auto left = exec_expr(bin->left);
    auto right = exec_expr(bin->right);

    if (is_int_lit(left) && is_int_lit(right))
    {
        long long a = std::stoll(left);
        long long b = std::stoll(right);
        long long res = eval_arith(a, bin->op_tok.value, b);
        return std::to_string(res);
    }

    auto t = nextTemp();
    identifiers[t] = "int";
    arr.append(make_assign(t, left, bin->op_tok.value, right));
    return t;
}

void IntermediateCodeGen::exec_assignment(const std::shared_ptr<Assignment> &a)
{
    auto right = exec_expr(a->expression);
    arr.append(make_assign(a->identifier.value, right, "", ""));
}

void IntermediateCodeGen::exec_condition(const std::shared_ptr<Condition> &c)
{
    auto left = exec_expr(c->left_expression);
    auto right = exec_expr(c->right_expression);

    auto body = currentLabel();
    arr.append(make_compare(left, c->comparison.value, right, body));
}

void IntermediateCodeGen::exec_if(const std::shared_ptr<IfStatement> &i)
{
    exec_condition(i->if_condition);
    auto body = nextLabel();
    auto end_if = nextLabel();

    if (i->else_body)
    {
        auto goto_else_end = nextLabel();
        arr.append(make_jump(goto_else_end));
        arr.append(make_label(body));
        exec_statement(i->if_body);
        arr.append(make_jump(end_if));
        arr.append(make_label(goto_else_end));
        exec_statement(i->else_body);
        arr.append(make_label(end_if));
    }
    else
    {
        arr.append(make_jump(end_if));
        arr.append(make_label(body));
        exec_statement(i->if_body);
        arr.append(make_label(end_if));
    }
}

void IntermediateCodeGen::exec_while(const std::shared_ptr<WhileStatement> &w)
{
    auto start_loop = nextLabel();
    arr.append(make_label(start_loop));
    exec_condition(w->condition);

    auto body_label = nextLabel();
    auto end_label = nextLabel();

    arr.append(make_jump(end_label));
    arr.append(make_label(body_label));
    exec_statement(w->body);
    arr.append(make_jump(start_loop));
    arr.append(make_label(end_label));
}

void IntermediateCodeGen::exec_print(const std::shared_ptr<PrintStatement> &p)
{
    if (p->type == "string")
    {
        auto sym = nextStringSym();
        constants[sym] = p->strValue;
        arr.code.push_back(make_print("string", sym));
    }
    else
    {
        auto temp = exec_expr(p->intExpr);
        arr.code.push_back(make_print("int", temp));
    }
}

void IntermediateCodeGen::exec_declaration(const std::shared_ptr<Declaration> &d)
{
    for (const auto &i : d->identifiers)
        identifiers[i.value] = d->declaration_type.value;
}

void IntermediateCodeGen::exec_statement(const std::shared_ptr<Node> &n)
{
    if (!n)
        return;

    if (auto st = std::dynamic_pointer_cast<Statement>(n))
    {
        exec_statement(st->left);
        exec_statement(st->right);
        return;
    }

    if (auto is = std::dynamic_pointer_cast<IfStatement>(n))
    {
        exec_if(is);
        return;
    }

    if (auto wh = std::dynamic_pointer_cast<WhileStatement>(n))
    {
        exec_while(wh);
        return;
    }

    if (auto pr = std::dynamic_pointer_cast<PrintStatement>(n))
    {
        exec_print(pr);
        return;
    }

    if (auto de = std::dynamic_pointer_cast<Declaration>(n))
    {
        exec_declaration(de);
        return;
    }

    if (auto asg = std::dynamic_pointer_cast<Assignment>(n))
    {
        exec_assignment(asg);
        return;
    }
}
