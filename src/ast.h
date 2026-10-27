/*
 * ast.h - arvore sintatica abstrata (AST) do subconjunto de C suportado.
 * Define os nos e a interface publica usada pelo parser e pelas etapas
 * seguintes (docs/Especificacao_da_Linguagem.md, secoes 8.3 e 8.7).
 */

#ifndef AST_H
#define AST_H

#include <stdio.h>

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

/*
 * Significado dos filhos por tipo de no:
 *
 *   PROGRAMA    filho[0] = lista de FUNCAO
 *   FUNCAO      filho[0] = lista de PARAMETRO (ou NULL)
 *               filho[1] = BLOCO do corpo (NULL = prototipo)
 *   BLOCO       filho[0] = lista de comandos (ou NULL)
 *   DECLARACAO  filho[0] = inicializacao (ou NULL)
 *   ATRIBUICAO  filho[0] = valor atribuido
 *   RETORNO     filho[0] = valor retornado
 *   IF          filho[0] = condicao, filho[1] = BLOCO entao,
 *               filho[2] = BLOCO senao (ou NULL)
 *   CHAMADA     filho[0] = lista de argumentos (ou NULL)
 *   BINARIO     filho[0] = operando esquerdo, filho[1] = operando direito
 *   UNARIO      filho[0] = operando
 *
 * Listas (funcoes, parametros, comandos, argumentos) sao encadeadas pelo
 * campo proximo, na ordem em que aparecem no codigo-fonte.
 */
typedef struct AstNo {
    AstTipoNo tipo;
    int linha;
    AstTipoDado tipo_dado;   /* FUNCAO (retorno), PARAMETRO, DECLARACAO */
    AstOperador op;          /* BINARIO, UNARIO */
    char *texto;             /* nome (ID, CHAMADA, FUNCAO, PARAMETRO, DECLARACAO,
                                ATRIBUICAO) ou conteudo sem delimitadores
                                (CHAR_LIT, STRING_LIT); o no guarda copia propria */
    int valor_int;           /* NUM_INT */
    double valor_float;      /* NUM_FLOAT */
    struct AstNo *filho[3];
    struct AstNo *proximo;   /* proximo item de uma lista */
} AstNo;

/* construtores: textos recebidos sao copiados */
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

/* anexa item ao fim da lista e devolve a cabeca (lista NULL = lista vazia) */
AstNo *ast_lista_anexa(AstNo *lista, AstNo *item);

/* imprime o no e sua subarvore com indentacao (nao segue no->proximo) */
void ast_imprime(FILE *saida, const AstNo *no);

/* libera o no, seus filhos e toda a cadeia proximo; aceita NULL */
void ast_libera(AstNo *no);

/* quantidade de nos alocados e ainda nao liberados (usado nos testes) */
int ast_nos_alocados(void);

const char *ast_nome_tipo_dado(AstTipoDado tipo);
const char *ast_simbolo_operador(AstOperador op);

#endif
