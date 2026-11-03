#ifndef AST_H
#define AST_H

#include <stdio.h>

struct Simbolo;

typedef enum {
    AST_PROGRAMA, AST_FUNCAO, AST_PARAMETRO, AST_BLOCO,
    AST_DECLARACAO, AST_ATRIBUICAO, AST_RETORNO, AST_IF,
    AST_CHAMADA, AST_BINARIO, AST_UNARIO,
    AST_NUM_INT, AST_NUM_FLOAT, AST_CHAR_LIT, AST_STRING_LIT, AST_ID
} AstTipoNo;

typedef enum { TIPO_INT, TIPO_FLOAT, TIPO_CHAR, TIPO_VOID } AstTipoDado;

typedef enum {
    OP_SOMA, OP_SUB, OP_MUL, OP_DIV,
    OP_MENOR, OP_MAIOR, OP_MENOR_IGUAL, OP_MAIOR_IGUAL, OP_IGUAL, OP_DIFERENTE,
    OP_E, OP_OU,
    OP_NEGATIVO, OP_NAO
} AstOperador;

typedef struct AstNo {
    AstTipoNo tipo;
    int linha;
    AstTipoDado tipo_dado;
    AstOperador op;
    char *texto;
    int valor_int;
    double valor_float;
    struct Simbolo *simbolo;
    struct AstNo *filho[3];
    struct AstNo *proximo;
} AstNo;

AstNo *ast_programa(AstNo *funcoes);
AstNo *ast_funcao(AstTipoDado retorno, const char *nome, AstNo *parametros, AstNo *corpo, int linha);
AstNo *ast_parametro(AstTipoDado tipo, const char *nome, int linha);
AstNo *ast_bloco(AstNo *comandos, int linha);
AstNo *ast_declaracao(AstTipoDado tipo, const char *nome, AstNo *inicializacao, int linha);
AstNo *ast_atribuicao(const char *nome, AstNo *valor, int linha);
AstNo *ast_retorno(AstNo *valor, int linha);
AstNo *ast_if(AstNo *condicao, AstNo *entao, AstNo *senao, int linha);
AstNo *ast_chamada(const char *nome, AstNo *argumentos, int linha);
AstNo *ast_binario(AstOperador op, AstNo *esquerda, AstNo *direita, int linha);
AstNo *ast_unario(AstOperador op, AstNo *operando, int linha);
AstNo *ast_num_int(int valor, int linha);
AstNo *ast_num_float(double valor, int linha);
AstNo *ast_char_lit(const char *texto, int linha);
AstNo *ast_string_lit(const char *texto, int linha);
AstNo *ast_id(const char *nome, int linha);

AstNo *ast_lista_anexa(AstNo *lista, AstNo *item);

void ast_imprime(FILE *saida, const AstNo *no);

void ast_libera(AstNo *no);

int ast_nos_alocados(void);

const char *ast_nome_tipo_dado(AstTipoDado tipo);
const char *ast_simbolo_operador(AstOperador op);

#endif