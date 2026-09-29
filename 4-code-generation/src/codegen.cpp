#include "codegen.hpp"
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <unordered_map>

static std::string op_to_asm(const std::string &op)
{
if (op == "+")
return "add";
// TBD
if (op == "-")
return "sub";
if (op == "*")
return "imul";
return "";
}

static std::string cmp_to_jmp(const std::string &c)
{
if (c == "<")
return "jl";
// TBD
if (c == "<=")
return "jle";
if (c == ">")
return "jg";
if (c == ">=")
return "jge";
if (c == "==")
return "je";
if (c == "!=")
return "jne";
return "";
}

CodeGenerator::CodeGenerator(const InterCodeArray &arr,
const std::unordered_map<std::string, std::string> &identifiers,
const std::unordered_map<std::string, std::string> &constants,
const std::unordered_map<std::string, std::string> &tempmap)
: arr(arr), ids(identifiers), consts(constants), tempmap(tempmap), need_print_num(false), need_print_string(false) {}

void CodeGenerator::pr(const std::string &s)
{
out.append(s);
out.push_back('\n');
}

std::string CodeGenerator::handleVar(const std::string &a, const std::unordered_map<std::string, std::string> &tempmap)
{
// TBD
auto it = tempmap.find(a);
if (it != tempmap.end())
return "[" + it->second + "]";

if (!a.empty() && (std::isalpha((unsigned char)a[0]) || a[0] == '_'))
return "[" + a + "]";

return a;
}

void CodeGenerator::gen_variables()
{
pr("section .bss");
if (need_print_num)
{
pr("\tdigitSpace resb 100");
pr("\tdigitSpacePos resb 8\n");
}
for (auto &kv : ids)
{
pr("\t" + kv.first + " resb 8");
}
}

void CodeGenerator::gen_start()
{
pr("section .data");
if (need_print_string)
{
// TBD
pr("\t_n1 db 10");
}
for (auto &kv : consts)
{
pr("\t" + kv.first + " db \"" + kv.second + "\",10,0");
}
pr("section .text");
pr("\tglobal _start\n");
pr("_start:");
}

void CodeGenerator::gen_end()
{
pr("\tmov rax, 60");
pr("\tmov rdi, 0");
pr("\tsyscall\n");
}

void CodeGenerator::gen_assignment(const AssignmentCode &a)
{
// TBD
// Inject missing .bss declarations ONCE, but do NOT redeclare anything already present in ids
// (otherwise NASM throws: "label inconsistently redefined").
static bool bss_injected = false;
if (!bss_injected)
{
std::unordered_map<std::string, bool> seen;

auto is_number = [](const std::string &s) -> bool {
if (s.empty())
return false;
size_t i = (s[0] == '-') ? 1 : 0;
if (i >= s.size())
return false;
for (; i < s.size(); ++i)
if (!std::isdigit((unsigned char)s[i]))
return false;
return true;
};

auto is_label = [](const std::string &s) -> bool {
if (s.size() < 2)
return false;
if (s[0] != 'L')
return false;
for (size_t i = 1; i < s.size(); ++i)
if (!std::isdigit((unsigned char)s[i]))
return false;
return true;
};

auto consider = [&](const std::string &sym) {
if (sym.empty())
return;
if (is_number(sym))
return;
if (is_label(sym))
return;
if (sym[0] == '.')
return; // local labels like ._pn_loop
if (consts.find(sym) != consts.end())
return; // string label already in .data
if (sym == "_n1")
return; // newline constant in .data

// IMPORTANT: if already declared in gen_variables() via ids, skip it
if (ids.find(sym) != ids.end())
return;

// If sym is a temp mapped to a storage label, use that label (and avoid duplicates too)
auto itT = tempmap.find(sym);
if (itT != tempmap.end())
{
if (ids.find(itT->second) != ids.end())
return;
seen[itT->second] = true;
return;
}

seen[sym] = true;
};

for (auto &ins : arr.code)
{
switch (ins->kind())
{
case IRKind::Assignment:
{
auto aa = std::static_pointer_cast<AssignmentCode>(ins);
consider(aa->var);
consider(aa->left);
consider(aa->right);
break;
}
case IRKind::Compare:
{
auto cc = std::static_pointer_cast<CompareCodeIR>(ins);
consider(cc->left);
consider(cc->right);
break;
}
case IRKind::Print:
{
auto pp = std::static_pointer_cast<PrintCodeIR>(ins);
if (pp->type != "string")
consider(pp->value);
break;
}
default:
break;
}
}

if (!seen.empty())
{
pr("section .bss");
for (auto &kv : seen)
pr("\t" + kv.first + " resb 8");
pr("section .text");
}

bss_injected = true;
}

auto mem = [&](const std::string &x) -> std::string {
auto itK = ids.find(x);
if (itK != ids.end())
return "[" + itK->first + "]";

for (const auto &kv : ids)
if (kv.second == x)
return "[" + kv.first + "]";

return handleVar(x, tempmap);
};

if (a.op.empty())
{
pr("\tmov rax, " + mem(a.left));
pr("\tmov " + mem(a.var) + ", rax");
return;
}

pr("\tmov rax, " + mem(a.left));
pr("\tmov rbx, " + mem(a.right));

if (a.op == "/")
{
pr("\tcqo");
pr("\tidiv rbx");
}
else
{
std::string opasm = op_to_asm(a.op);
pr("\t" + opasm + " rax, rbx");
}

pr("\tmov " + mem(a.var) + ", rax");
}

void CodeGenerator::gen_jump(const JumpCode &j)
{
// TBD
pr("\tjmp " + j.dist);
}

void CodeGenerator::gen_label(const LabelCode &l)
{
// TBD
pr(l.label + ":");
}

void CodeGenerator::gen_compare(const CompareCodeIR &c)
{
// TBD
auto mem = [&](const std::string &x) -> std::string {
auto itK = ids.find(x);
if (itK != ids.end())
return "[" + itK->first + "]";
for (const auto &kv : ids)
if (kv.second == x)
return "[" + kv.first + "]";
return handleVar(x, tempmap);
};

std::string j = cmp_to_jmp(c.operation);

pr("\tmov rax, " + mem(c.left));
pr("\tmov rbx, " + mem(c.right));
pr("\tcmp rax, rbx");
pr("\t" + j + " " + c.jump);
}

void CodeGenerator::gen_print(const PrintCodeIR &p)
{
// TBD
if (p.type == "string")
{
pr("\tlea rdi, [" + p.value + "]");
pr("\tcall _print_string");
return;
}

auto mem = [&](const std::string &x) -> std::string {
auto itK = ids.find(x);
if (itK != ids.end())
return "[" + itK->first + "]";
for (const auto &kv : ids)
if (kv.second == x)
return "[" + kv.first + "]";
return handleVar(x, tempmap);
};

pr("\tmov rax, " + mem(p.value));
pr("\tcall _print_num");
}

void CodeGenerator::gen_code()
{
for (auto &ins : arr.code)
{
switch (ins->kind())
{
case IRKind::Assignment:
gen_assignment(*std::static_pointer_cast<AssignmentCode>(ins));
break;
case IRKind::Jump:
gen_jump(*std::static_pointer_cast<JumpCode>(ins));
break;
case IRKind::Label:
gen_label(*std::static_pointer_cast<LabelCode>(ins));
break;
case IRKind::Compare:
gen_compare(*std::static_pointer_cast<CompareCodeIR>(ins));
break;
case IRKind::Print:
gen_print(*std::static_pointer_cast<PrintCodeIR>(ins));
break;
}
}
}

void CodeGenerator::writeAsm(const std::string &path)
{
out.clear();

for (auto &ins : arr.code)
{
if (ins->kind() == IRKind::Print)
{
auto print_ins = std::static_pointer_cast<PrintCodeIR>(ins);
if (print_ins->type == "string")
need_print_string = true;
else
need_print_num = true;
}
}

gen_variables();
gen_start();
gen_code();
gen_end();
if (need_print_num)
gen_print_num_function();
if (need_print_string)
gen_print_string_function();

std::ofstream f(path, std::ios::binary);
f << out;
f.close();
}

void CodeGenerator::gen_print_string_function()
{
pr("\n_print_string:");
pr("\tpush rbx");
pr("\tpush rcx");
pr("\tpush rdx");
pr("\tpush rsi");
pr("\tpush rdi");

pr("\tmov rsi, rdi");
pr("\txor rdx, rdx");

pr("._strlen_loop:");
pr("\tcmp byte [rsi+rdx], 0");
pr("\tje ._strlen_done");
pr("\tinc rdx");
pr("\tjmp ._strlen_loop");

pr("._strlen_done:");
pr("\tmov rax, 1");
pr("\tmov rdi, 1");
pr("\tsyscall");

pr("\tpop rdi");
pr("\tpop rsi");
pr("\tpop rdx");
pr("\tpop rcx");
pr("\tpop rbx");
pr("\tret\n");
}

void CodeGenerator::gen_print_num_function()
{
pr("\n_print_num:");
pr("\tpush rbx");
pr("\tpush rcx");
pr("\tpush rdx");
pr("\tpush rsi");
pr("\tpush rdi");
pr("\tpush r8");

pr("\tcmp rax, 0");
pr("\tjne ._pn_not_zero");
pr("\tmov byte [digitSpace+99], '0'");
pr("\tmov rax, 1");
pr("\tmov rdi, 1");
pr("\tlea rsi, [digitSpace+99]");
pr("\tmov rdx, 1");
pr("\tsyscall");
pr("\tjmp ._pn_newline");

pr("._pn_not_zero:");

pr("\tmov rbx, 0");
pr("\tcmp rax, 0");
pr("\tjge ._pn_positive");
pr("\tneg rax");
pr("\tmov rbx, 1");

pr("._pn_positive:");

pr("\tlea rcx, [digitSpace+100]");
pr("\tmov r8, 10");

pr("._pn_loop:");
pr("\txor rdx, rdx");
pr("\tdiv r8");
pr("\tadd dl, '0'");
pr("\tdec rcx");
pr("\tmov [rcx], dl");
pr("\ttest rax, rax");
pr("\tjnz ._pn_loop");

pr("\tcmp rbx, 1");
pr("\tjne ._pn_write");
pr("\tdec rcx");
pr("\tmov byte [rcx], '-'");

pr("._pn_write:");
pr("\tmov rax, 1");
pr("\tmov rdi, 1");
pr("\tmov rsi, rcx");
pr("\tlea rdx, [digitSpace+100]");
pr("\tsub rdx, rcx");
pr("\tsyscall");

pr("._pn_newline:");
pr("\tsub rsp, 8");
pr("\tmov byte [rsp], 10");
pr("\tmov rax, 1");
pr("\tmov rdi, 1");
pr("\tmov rsi, rsp");
pr("\tmov rdx, 1");
pr("\tsyscall");
pr("\tadd rsp, 8");

pr("\tpop r8");
pr("\tpop rdi");
pr("\tpop rsi");
pr("\tpop rdx");
pr("\tpop rcx");
pr("\tpop rbx");
pr("\tret\n");
}