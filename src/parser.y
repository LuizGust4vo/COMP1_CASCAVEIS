/*
 * parser.y - gramatica do subconjunto de C suportado pelo compilador.
 * Implementa as regras sintaticas definidas em docs/Especificacao_da_Linguagem.md
 * e constroi a arvore sintatica abstrata (AST) inicial.
 */

%code requires {
#include "ast.h"

typedef struct {
    char *texto;
    int linha;
} TokenTexto;

typedef struct {
    int valor;
    int linha;
} TokenInt;

typedef struct {
    double valor;
    int linha;
} TokenFloat;
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
extern int yylineno;
void yyerror(const char *s);

AstNo *raiz_ast = NULL;
int construcoes_sem_ast = 0;
%}

%define parse.error verbose
/* Falha na geracao se surgirem conflitos nao resolvidos na gramatica. */
%expect 0

%union {
    int linha;
    TokenInt num_int;
    TokenFloat num_float;
    TokenTexto texto;
    AstTipoDado tipo_dado;
    AstNo *no;
}

%token <linha> INT FLOAT CHAR VOID
%token <linha> IF ELSE WHILE FOR RETURN
%token <texto> ID CHAR_LIT STRING_LIT
%token <num_int> NUM_INT
%token <num_float> NUM_FLOAT
%token <linha> EQ NE LE GE
%token <linha> AND OR NOT
%token <linha> ASSIGN LT GT PLUS MINUS TIMES DIVIDE
%token <linha> LBRACE RBRACE LPAREN RPAREN COMMA SEMI

/* Se ocorrer erro sintatico no meio da analise, libera textos e nos da pilha */
%destructor { free($$.texto); } <texto>
%destructor { ast_libera($$); } <no>

%type <tipo_dado> tipo tipo_retorno
%type <no> programa funcao declaracao_funcao definicao_funcao
%type <no> parametros_opcionais parametros parametro
%type <no> bloco comandos comando declaracao atribuicao
%type <no> condicional repeticao retorno chamada_funcao
%type <no> argumentos_opcionais argumentos expressao

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
        {
            $$ = ast_programa(ast_lista_anexa(NULL, $1));
            raiz_ast = $$;
        }
    | programa funcao
        {
            $1->filho[0] = ast_lista_anexa($1->filho[0], $2);
            $$ = $1;
            raiz_ast = $$;
        }
    ;

funcao
    : declaracao_funcao
        { $$ = $1; }
    | definicao_funcao
        { $$ = $1; }
    ;

tipo
    : INT   { $$ = TIPO_INT; }
    | FLOAT { $$ = TIPO_FLOAT; }
    | CHAR  { $$ = TIPO_CHAR; }
    ;

tipo_retorno
    : tipo  { $$ = $1; }
    | VOID  { $$ = TIPO_VOID; }
    ;

declaracao_funcao
    : tipo_retorno ID LPAREN parametros_opcionais RPAREN SEMI
        {
            $$ = ast_funcao($1, $2.texto, $4, NULL, $2.linha);
            free($2.texto);
        }
    ;

definicao_funcao
    : tipo_retorno ID LPAREN parametros_opcionais RPAREN bloco
        {
            $$ = ast_funcao($1, $2.texto, $4, $6, $2.linha);
            free($2.texto);
        }
    ;

parametros_opcionais
    : /* vazio */
        { $$ = NULL; }
    | parametros
        { $$ = $1; }
    ;

parametros
    : parametro
        { $$ = ast_lista_anexa(NULL, $1); }
    | parametros COMMA parametro
        { $$ = ast_lista_anexa($1, $3); }
    ;

parametro
    : tipo ID
        {
            $$ = ast_parametro($1, $2.texto, $2.linha);
            free($2.texto);
        }
    ;

bloco
    : LBRACE comandos RBRACE
        { $$ = ast_bloco($2, $1); }
    ;

comandos
    : /* vazio */
        { $$ = NULL; }
    | comandos comando
        { $$ = ast_lista_anexa($1, $2); }
    ;

comando
    : declaracao
        { $$ = $1; }
    | atribuicao
        { $$ = $1; }
    | chamada_funcao SEMI
        { $$ = $1; }
    | condicional
        { $$ = $1; }
    | repeticao
        { $$ = $1; }
    | retorno
        { $$ = $1; }
    ;

declaracao
    : tipo ID SEMI
        {
            $$ = ast_declaracao($1, $2.texto, NULL, $2.linha);
            free($2.texto);
        }
    | tipo ID ASSIGN expressao SEMI
        {
            $$ = ast_declaracao($1, $2.texto, $4, $2.linha);
            free($2.texto);
        }
    ;

atribuicao
    : ID ASSIGN expressao SEMI
        {
            $$ = ast_atribuicao($1.texto, $3, $1.linha);
            free($1.texto);
        }
    ;

condicional
    : IF LPAREN expressao RPAREN bloco
        { $$ = ast_if($3, $5, NULL, $1); }
    | IF LPAREN expressao RPAREN bloco ELSE bloco
        { $$ = ast_if($3, $5, $7, $1); }
    ;

repeticao
    : WHILE LPAREN expressao RPAREN bloco
        {
            construcoes_sem_ast++;
            ast_libera($3);
            ast_libera($5);
            $$ = NULL;
        }
    | FOR LPAREN inicializacao_for SEMI expressao SEMI atualizacao_for RPAREN bloco
        {
            construcoes_sem_ast++;
            ast_libera($5);
            ast_libera($9);
            $$ = NULL;
        }
    ;

inicializacao_for
    : tipo ID ASSIGN expressao
        {
            free($2.texto);
            ast_libera($4);
        }
    | ID ASSIGN expressao
        {
            free($1.texto);
            ast_libera($3);
        }
    ;

atualizacao_for
    : ID ASSIGN expressao
        {
            free($1.texto);
            ast_libera($3);
        }
    ;

retorno
    : RETURN expressao SEMI
        { $$ = ast_retorno($2, $1); }
    | RETURN SEMI
        { $$ = ast_retorno(NULL, $1); }
    ;

chamada_funcao
    : ID LPAREN argumentos_opcionais RPAREN
        {
            $$ = ast_chamada($1.texto, $3, $1.linha);
            free($1.texto);
        }
    ;

argumentos_opcionais
    : /* vazio */
        { $$ = NULL; }
    | argumentos
        { $$ = $1; }
    ;

argumentos
    : expressao
        { $$ = ast_lista_anexa(NULL, $1); }
    | argumentos COMMA expressao
        { $$ = ast_lista_anexa($1, $3); }
    ;

expressao
    : NUM_INT
        { $$ = ast_num_int($1.valor, $1.linha); }
    | NUM_FLOAT
        { $$ = ast_num_float($1.valor, $1.linha); }
    | CHAR_LIT
        {
            $$ = ast_char_lit($1.texto, $1.linha);
            free($1.texto);
        }
    | STRING_LIT
        {
            $$ = ast_string_lit($1.texto, $1.linha);
            free($1.texto);
        }
    | ID
        {
            $$ = ast_id($1.texto, $1.linha);
            free($1.texto);
        }
    | chamada_funcao
        { $$ = $1; }
    | LPAREN expressao RPAREN
        { $$ = $2; }
    | MINUS expressao %prec UMINUS
        { $$ = ast_unario(OP_NEGATIVO, $2, $1); }
    | NOT expressao
        { $$ = ast_unario(OP_NAO, $2, $1); }
    | expressao PLUS expressao
        { $$ = ast_binario(OP_SOMA, $1, $3, $2); }
    | expressao MINUS expressao
        { $$ = ast_binario(OP_SUB, $1, $3, $2); }
    | expressao TIMES expressao
        { $$ = ast_binario(OP_MUL, $1, $3, $2); }
    | expressao DIVIDE expressao
        { $$ = ast_binario(OP_DIV, $1, $3, $2); }
    | expressao LT expressao
        { $$ = ast_binario(OP_MENOR, $1, $3, $2); }
    | expressao GT expressao
        { $$ = ast_binario(OP_MAIOR, $1, $3, $2); }
    | expressao LE expressao
        { $$ = ast_binario(OP_MENOR_IGUAL, $1, $3, $2); }
    | expressao GE expressao
        { $$ = ast_binario(OP_MAIOR_IGUAL, $1, $3, $2); }
    | expressao EQ expressao
        { $$ = ast_binario(OP_IGUAL, $1, $3, $2); }
    | expressao NE expressao
        { $$ = ast_binario(OP_DIFERENTE, $1, $3, $2); }
    | expressao AND expressao
        { $$ = ast_binario(OP_E, $1, $3, $2); }
    | expressao OR expressao
        { $$ = ast_binario(OP_OU, $1, $3, $2); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Erro sintatico na linha %d: %s\n", yylineno, s);
}