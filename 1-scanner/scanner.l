%{
/* C/C++ code to include (headers, global vars, etc.) */
#include <stdio.h>
%}

/* Definitions (regex patterns, macros, options) */
DIGIT       [0-9]
ID_START    [A-Za-z_]
ID_CHAR     [A-Za-z0-9_]
IDENTIFIER  {ID_START}{ID_CHAR}*
INT_LIT     {DIGIT}+
STRING      \"[^"\n]*\"
WHITE_SPACE [ \t\r\f\v\r\n]+
COMMENT_1   "//".*
COMMENT_2   "/\\*([^*]|\\*+[^*/])*\\*+/"

%%
 /* Rules: regex → action */
{IDENTIFIER}       {   
                    std::string s(yytext, yyleng);
                    if (s == "if")          std::cout << "IF_TOKEN: " << s << std::endl;
		    else if (s == "else")   std::cout << "ELSE_TOKEN: " << s << std::endl;
                    else if (s == "int")    std::cout << "INT_TOKEN: " << s << std::endl;
		    else if (s == "while")  std::cout << "WHILE_TOKEN: " << s << std::endl;
		    else if (s == "print")  std::cout << "PRINT_TOKEN: " << s << std::endl;
                    else if (s == "prints") std::cout << "PRINTS_TOKEN: " << s << std::endl;
                    else                    std::cout << "VAR_TOKEN: " << s << std::endl; 
                   }

{WHITE_SPACE}      { /* skip */ }
{COMMENT_1}        { /* skip */ }
{COMMENT_2}        { /* skip */ }

">="        { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }
"<="        { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }
"=="        { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }
"!="        { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }
">"         { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }
"<"         { std::cout << "COMPARISON_TOKEN: " << yytext << std::endl; }

"+"         { std::cout << "ARITHMETICAL_OPERATION_TOKEN: " << yytext << std::endl; }
"-"	    { std::cout << "ARITHMETICAL_OPERATION_TOKEN: " << yytext << std::endl; }
"*"         { std::cout << "ARITHMETICAL_OPERATION_TOKEN: " << yytext << std::endl; }
"/"         { std::cout << "ARITHMETICAL_OPERATION_TOKEN: " << yytext << std::endl; }

"="         { std::cout << "ASSIGNMENT_TOKEN: " << yytext << std::endl; }

"("	    { std::cout << "L1_TOKEN: " << yytext << std::endl; }
")"	    { std::cout << "R1_TOKEN: " << yytext << std::endl; }
"{"	    { std::cout << "L2_TOKEN: " << yytext << std::endl; }
"}"	    { std::cout << "R2_TOKEN: " << yytext << std::endl; }
","	    { std::cout << "COMMA_TOKEN: " << yytext << std::endl; }
";"	    { std::cout << "SEMICOLON_TOKEN: " << yytext << std::endl; }

{STRING}    { std::cout << "STRING_TOKEN: " << yytext << std::endl; }
{INT_LIT}   { std::cout << "INT_TOKEN: " << yytext << std::endl; }
.           { std::cerr << "UNKNOWN_TOKEN: " << yytext << std::endl; }

%%
 
/* User code (helper functions, main(), etc.) */
int yyFlexLexer::yywrap() { return 1; }

int main(void) {
    yyFlexLexer lexer;
    lexer.yylex();
    return 0;
}