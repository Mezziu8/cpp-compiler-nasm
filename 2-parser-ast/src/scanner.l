%option noyywrap nodefault nounput yylineno

%{
#include <string>
#include <vector>
#include <stdexcept>
#include "tokens.hpp"
#include <cstdio>

// Match Flex-generated declarations (C++ linkage) DO NOT EDIT
typedef struct yy_buffer_state *YY_BUFFER_STATE;
int yylex(void);
void yyrestart(FILE*);
YY_BUFFER_STATE yy_scan_string(const char* str);
void yy_delete_buffer(YY_BUFFER_STATE b);

static std::vector<Token>* g_tokens = nullptr;
static YY_BUFFER_STATE g_buf = nullptr;

void scan_to_tokens(std::vector<Token>& out) {
    g_tokens = &out;
    yyrestart(nullptr);
    yylex();
}

void scan_string_to_tokens(const std::string& src, std::vector<Token>& out) {
    g_tokens = &out;
    g_buf = yy_scan_string(src.c_str());
    yylex();
    yy_delete_buffer(g_buf);
    g_buf = nullptr;
}

%}

DIGIT      [0-9]
ID_START   [A-Za-z_]
ID_CONT    [A-Za-z0-9_]
WS         [ \t\r\f]+
NL         [\n]

%%

"#"[^\n]*                ;   // Comment

\"([^\"\\]|\\.)*\"       {
    std::string s(yytext + 1, yyleng - 2);
    g_tokens->push_back(Token{TokenType::String, s, yylineno});
}

"=="      {
    g_tokens->push_back(Token{TokenType::Comparison,
                              std::string(yytext, yyleng), yylineno});
}

">="      { g_tokens->push_back(Token{TokenType::Comparison, ">=", yylineno}); }
"<="      { g_tokens->push_back(Token{TokenType::Comparison, "<=", yylineno}); }
"!="      { g_tokens->push_back(Token{TokenType::Comparison, "!=", yylineno}); }
">"       { g_tokens->push_back(Token{TokenType::Comparison, ">",  yylineno}); }
"<"       { g_tokens->push_back(Token{TokenType::Comparison, "<",  yylineno}); }

"+"|"="      {
    TokenType tt = (yytext[0] == '=') ? TokenType::Assign : TokenType::Arth;
    g_tokens->push_back(Token{tt, std::string(yytext, yyleng), yylineno});
}

"-"       { g_tokens->push_back(Token{TokenType::Arth, "-", yylineno}); }
"*"       { g_tokens->push_back(Token{TokenType::Arth, "*", yylineno}); }
"/"       { g_tokens->push_back(Token{TokenType::Arth, "/", yylineno}); }

"("       { g_tokens->push_back(Token{TokenType::L1, "(", yylineno}); }
")"       { g_tokens->push_back(Token{TokenType::R1, ")", yylineno}); }
"{"       { g_tokens->push_back(Token{TokenType::L2, "{", yylineno}); }
"}"       { g_tokens->push_back(Token{TokenType::R2, "}", yylineno}); }
","       { g_tokens->push_back(Token{TokenType::Separator, ",", yylineno}); }

";"                      { g_tokens->push_back(Token{TokenType::Semicolon, ";", yylineno}); }

"else"    { g_tokens->push_back(Token{TokenType::Else, "else", yylineno}); }
"while"   { g_tokens->push_back(Token{TokenType::While, "while", yylineno}); }
"int"     { g_tokens->push_back(Token{TokenType::Int, "int", yylineno}); }
"print"   { g_tokens->push_back(Token{TokenType::Print, "print", yylineno}); }
"prints"  { g_tokens->push_back(Token{TokenType::Prints, "prints", yylineno}); }
"string"  { g_tokens->push_back(Token{TokenType::StringKw, "string", yylineno}); }

{ID_START}{ID_CONT}*     {
    std::string s(yytext, yyleng);
    if      (s == "if")     g_tokens->push_back(Token{TokenType::If, s, yylineno});
    else                    g_tokens->push_back(Token{TokenType::Var, "V" + s, yylineno});
}

{DIGIT}+                 {
    g_tokens->push_back(Token{TokenType::IntLit,
                              std::string(yytext, yyleng), yylineno});
}

{WS}                     ;   // Ignore whitespace
{NL}                     ;   // Ignore newlines

.                        {
    throw std::runtime_error("Unknown char at line " +
                             std::to_string(yylineno));
}

<<EOF>>                  {
    g_tokens->push_back(Token{TokenType::End, "END", yylineno});
    return 0;
}

%%
