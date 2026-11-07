/*
 * lexer.l - analisador lexico do subconjunto de C suportado pelo compilador.
 * Reconhece os tokens definidos em docs/Especificacao_da_Linguagem.md (secao 3)
 * e entrega seus valores semanticos e linha ao parser (Bison).
 */

%option noyywrap
%option yylineno
%option nounput
%option noinput

%x COMENTARIO
%x STRING_STATE

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

int erros_lexicos = 0;
static int inicio_string_linha = 0;

/* Buffer dinamico para acumular o conteudo de STRING_LIT sem as aspas */
static char *string_buf = NULL;
static size_t string_len = 0;
static size_t string_cap = 0;

static char *copia_fatia(const char *s, size_t n) {
    char *c = malloc(n + 1);
    if (!c) {
        fprintf(stderr, "Erro interno: memoria insuficiente\n");
        exit(1);
    }
    memcpy(c, s, n);
    c[n] = '\0';
    return c;
}

static void string_inicia(void) {
    if (!string_buf) {
        string_cap = 64;
        string_buf = malloc(string_cap);
        if (!string_buf) {
            fprintf(stderr, "Erro interno: memoria insuficiente\n");
            exit(1);
        }
    }
    string_len = 0;
    string_buf[0] = '\0';
}

static void string_anexa(const char *trecho, size_t n) {
    if (string_len + n + 1 > string_cap) {
        while (string_len + n + 1 > string_cap) {
            string_cap *= 2;
        }
        string_buf = realloc(string_buf, string_cap);
        if (!string_buf) {
            fprintf(stderr, "Erro interno: memoria insuficiente\n");
            exit(1);
        }
    }
    memcpy(string_buf + string_len, trecho, n);
    string_len += n;
    string_buf[string_len] = '\0';
}
%}

DIGITO      [0-9]
LETRA       [a-zA-Z_]
IDENT_INVALIDO  {DIGITO}+{LETRA}({LETRA}|{DIGITO})*

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

\"                       { BEGIN(STRING_STATE); inicio_string_linha = yylineno; string_inicia(); }
<STRING_STATE>\"         {
                            BEGIN(INITIAL);
                            yylval.texto.texto = copia_fatia(string_buf, string_len);
                            yylval.texto.linha = inicio_string_linha;
                            return STRING_LIT;
                         }
<STRING_STATE>\\.        { string_anexa(yytext, (size_t)yyleng); }
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
<STRING_STATE>.          { string_anexa(yytext, 1); }

"int"                    { yylval.linha = yylineno; return INT; }
"float"                  { yylval.linha = yylineno; return FLOAT; }
"char"                   { yylval.linha = yylineno; return CHAR; }
"void"                   { yylval.linha = yylineno; return VOID; }
"if"                     { yylval.linha = yylineno; return IF; }
"else"                   { yylval.linha = yylineno; return ELSE; }
"while"                  { yylval.linha = yylineno; return WHILE; }
"for"                    { yylval.linha = yylineno; return FOR; }
"return"                 { yylval.linha = yylineno; return RETURN; }

"=="                     { yylval.linha = yylineno; return EQ; }
"!="                     { yylval.linha = yylineno; return NE; }
"<="                     { yylval.linha = yylineno; return LE; }
">="                     { yylval.linha = yylineno; return GE; }
"&&"                     { yylval.linha = yylineno; return AND; }
"||"                     { yylval.linha = yylineno; return OR; }
"="                      { yylval.linha = yylineno; return ASSIGN; }
"<"                      { yylval.linha = yylineno; return LT; }
">"                      { yylval.linha = yylineno; return GT; }
"+"                      { yylval.linha = yylineno; return PLUS; }
"--" {
    fprintf(stderr,
            "Erro lexico: operador nao suportado '%s' na linha %d\n",
            yytext, yylineno);
    erros_lexicos++;
}
"-"                      { yylval.linha = yylineno; return MINUS; }
"*"                      { yylval.linha = yylineno; return TIMES; }
"/"                      { yylval.linha = yylineno; return DIVIDE; }
"!"                      { yylval.linha = yylineno; return NOT; }

"{"                      { yylval.linha = yylineno; return LBRACE; }
"}"                      { yylval.linha = yylineno; return RBRACE; }
"("                      { yylval.linha = yylineno; return LPAREN; }
")"                      { yylval.linha = yylineno; return RPAREN; }
","                      { yylval.linha = yylineno; return COMMA; }
";"                      { yylval.linha = yylineno; return SEMI; }

{IDENT_INVALIDO} {
    fprintf(stderr,
            "Erro lexico: identificador invalido '%s' na linha %d\n",
            yytext, yylineno);
    erros_lexicos++;
}

{DIGITO}+"."{DIGITO}+    {
                            yylval.num_float.valor = strtod(yytext, NULL);
                            yylval.num_float.linha = yylineno;
                            return NUM_FLOAT;
                         }
{DIGITO}+                {
                            yylval.num_int.valor = atoi(yytext);
                            yylval.num_int.linha = yylineno;
                            return NUM_INT;
                         }
'([^'\\\n]|\\.)'         {
                            yylval.texto.texto = copia_fatia(yytext + 1, (size_t)(yyleng - 2));
                            yylval.texto.linha = yylineno;
                            return CHAR_LIT;
                         }

{LETRA}({LETRA}|{DIGITO})*  {
                            yylval.texto.texto = copia_fatia(yytext, (size_t)yyleng);
                            yylval.texto.linha = yylineno;
                            return ID;
                         }

[ \t\r\n]+               { /* espacos em branco: ignora */ }

. {
    fprintf(stderr,
            "Erro lexico: caractere nao reconhecido '%s' na linha %d\n",
            yytext, yylineno);
    erros_lexicos++;
}

%%