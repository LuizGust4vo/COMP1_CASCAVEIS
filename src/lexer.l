/*
 * lexer.l - analisador lexico do subconjunto de C suportado pelo compilador.
 * Reconhece os tokens definidos em docs/Especificacao_da_Linguagem.md (secao 3).
 */

%option noyywrap
%option yylineno

%x COMENTARIO
%x STRING_STATE

%{
#include "parser.tab.h"

int erros_lexicos = 0;
static int inicio_string_linha = 0;
%}

DIGITO      [0-9]
LETRA       [a-zA-Z_]

%%

"//".*                  { /* comentario de linha unica: ignora */ }

"/*"                    { BEGIN(COMENTARIO); }
<COMENTARIO>"*/"         { BEGIN(INITIAL); }
<COMENTARIO>\n           { /* yylineno ja incrementado automaticamente */ }
<COMENTARIO>.            { /* ignora conteudo do comentario */ }
<COMENTARIO><<EOF>>      {
                            fprintf(stderr, "Erro lexico: comentario de bloco nao terminado (linha %d)\n", yylineno);
                            erros_lexicos++;
                            yyterminate();
                         }

\"                       { BEGIN(STRING_STATE); inicio_string_linha = yylineno; }
<STRING_STATE>\"         { BEGIN(INITIAL); return STRING_LIT; }
<STRING_STATE>\\.        { /* sequencia de escape: consome */ }
<STRING_STATE>\n         {
                            fprintf(stderr, "Erro lexico: string nao terminada iniciada na linha %d\n", inicio_string_linha);
                            erros_lexicos++;
                            BEGIN(INITIAL);
                         }
<STRING_STATE><<EOF>>    {
                            fprintf(stderr, "Erro lexico: string nao terminada iniciada na linha %d\n", inicio_string_linha);
                            erros_lexicos++;
                            yyterminate();
                         }
<STRING_STATE>.          { /* conteudo da string: consome */ }

"int"                    { return INT; }
"float"                  { return FLOAT; }
"char"                   { return CHAR; }
"void"                   { return VOID; }
"if"                     { return IF; }
"else"                   { return ELSE; }
"while"                  { return WHILE; }
"for"                    { return FOR; }
"return"                 { return RETURN; }

"=="                     { return EQ; }
"!="                     { return NE; }
"<="                     { return LE; }
">="                     { return GE; }
"&&"                     { return AND; }
"||"                     { return OR; }
"="                      { return ASSIGN; }
"<"                      { return LT; }
">"                      { return GT; }
"+"                      { return PLUS; }
"-"                      { return MINUS; }
"*"                      { return TIMES; }
"/"                      { return DIVIDE; }
"!"                      { return NOT; }

"{"                      { return LBRACE; }
"}"                      { return RBRACE; }
"("                      { return LPAREN; }
")"                      { return RPAREN; }
","                      { return COMMA; }
";"                      { return SEMI; }

{DIGITO}+"."{DIGITO}+    { return NUM_FLOAT; }
{DIGITO}+                { return NUM_INT; }
'([^'\\\n]|\\.)'         { return CHAR_LIT; }

{LETRA}({LETRA}|{DIGITO})*  { return ID; }

[ \t\r\n]+               { /* espacos em branco: ignora */ }

.                        {
                            fprintf(stderr, "Erro lexico: caractere nao reconhecido '%s' na linha %d\n", yytext, yylineno);
                            erros_lexicos++;
                         }

%%
