/*
 * parser.y - gramatica do subconjunto de C suportado pelo compilador.
 * Implementa as regras sintaticas definidas em docs/Especificacao_da_Linguagem.md (secao 4).
 */

%{
#include <stdio.h>

int yylex(void);
extern int yylineno;
void yyerror(const char *s);
%}

%define parse.error verbose

%token INT FLOAT CHAR VOID
%token IF ELSE WHILE FOR RETURN
%token ID NUM_INT NUM_FLOAT CHAR_LIT STRING_LIT
%token EQ NE LE GE
%token AND OR NOT
%token ASSIGN LT GT PLUS MINUS TIMES DIVIDE
%token LBRACE RBRACE LPAREN RPAREN COMMA SEMI

/* precedencia: da menor para a maior, conforme secao 4.5 da especificacao */
%left OR
%left AND
%left EQ NE
%left LT GT LE GE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS

%%

programa
    : funcao
    | programa funcao
    ;

funcao
    : declaracao_funcao
    | definicao_funcao
    ;

tipo
    : INT
    | FLOAT
    | CHAR
    ;

tipo_retorno
    : tipo
    | VOID
    ;

declaracao_funcao
    : tipo_retorno ID LPAREN parametros_opcionais RPAREN SEMI
    ;

definicao_funcao
    : tipo_retorno ID LPAREN parametros_opcionais RPAREN bloco
    ;

parametros_opcionais
    : /* vazio */
    | parametros
    ;

parametros
    : parametro
    | parametros COMMA parametro
    ;

parametro
    : tipo ID
    ;

bloco
    : LBRACE comandos RBRACE
    ;

comandos
    : /* vazio */
    | comandos comando
    ;

comando
    : declaracao
    | atribuicao
    | chamada_funcao SEMI
    | condicional
    | repeticao
    | retorno
    ;

declaracao
    : tipo ID SEMI
    | tipo ID ASSIGN expressao SEMI
    ;

atribuicao
    : ID ASSIGN expressao SEMI
    ;

condicional
    : IF LPAREN expressao RPAREN bloco
    | IF LPAREN expressao RPAREN bloco ELSE bloco
    ;

repeticao
    : WHILE LPAREN expressao RPAREN bloco
    | FOR LPAREN inicializacao_for SEMI expressao SEMI atualizacao_for RPAREN bloco
    ;

inicializacao_for
    : tipo ID ASSIGN expressao
    | ID ASSIGN expressao
    ;

atualizacao_for
    : ID ASSIGN expressao
    ;

retorno
    : RETURN expressao SEMI
    ;

chamada_funcao
    : ID LPAREN argumentos_opcionais RPAREN
    ;

argumentos_opcionais
    : /* vazio */
    | argumentos
    ;

argumentos
    : expressao
    | argumentos COMMA expressao
    ;

expressao
    : NUM_INT
    | NUM_FLOAT
    | CHAR_LIT
    | STRING_LIT
    | ID
    | chamada_funcao
    | LPAREN expressao RPAREN
    | MINUS expressao %prec UMINUS
    | NOT expressao
    | expressao PLUS expressao
    | expressao MINUS expressao
    | expressao TIMES expressao
    | expressao DIVIDE expressao
    | expressao LT expressao
    | expressao GT expressao
    | expressao LE expressao
    | expressao GE expressao
    | expressao EQ expressao
    | expressao NE expressao
    | expressao AND expressao
    | expressao OR expressao
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Erro sintatico na linha %d: %s\n", yylineno, s);
}
