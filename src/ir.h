#ifndef IR_H
#define IR_H

/*
 * ir.h - geracao de codigo intermediario (issue 4.3).
 *
 * A representacao adotada e o codigo de tres enderecos em quadruplas,
 * definida em docs/representacao_intermediaria.md (issue 4.2).
 */

#include <stdio.h>
#include "ast.h"

typedef enum {
    IR_SOMA, IR_SUB, IR_MUL, IR_DIV, IR_DIV_INT,
    IR_MENOR, IR_MAIOR, IR_MENOR_IGUAL, IR_MAIOR_IGUAL, IR_IGUAL, IR_DIFERENTE,
    IR_NEGATIVO, IR_NAO,
    IR_ATRIB, IR_DECL,
    IR_LABEL, IR_GOTO, IR_IF, IR_IF_FALSE,
    IR_FUNC, IR_FPARAM, IR_FIM_FUNC,
    IR_PARAM, IR_CALL, IR_RETURN
} IrOp;

/*
 * Campos que nao se aplicam ficam NULL. As strings pertencem ao modulo e
 * sao liberadas por ir_libera().
 */
typedef struct Quadrupla {
    IrOp op;
    char *arg1;
    char *arg2;
    char *resultado;
    struct Quadrupla *proximo;
} Quadrupla;

/* ---------- infraestrutura ---------- */

/* Anexa uma instrucao ao fim da lista. Os argumentos sao copiados. */
Quadrupla *ir_emite(IrOp op, const char *arg1, const char *arg2, const char *resultado);

/*
 * Prepara a geracao de uma funcao: reinicia os contadores de temporarios e
 * labels e registra os nomes de parametros e variaveis da funcao, para que
 * nenhum temporario t<n> colida com eles (secao 2.2 da RI).
 */
void ir_inicia_funcao(const AstNo *funcao);

/* Novo temporario (t1, t2, ...) e novo label (L1, L2, ...) da funcao atual. */
const char *ir_novo_temp(void);
const char *ir_novo_label(void);

/* Indica se o operando e um temporario criado na funcao atual. */
int ir_eh_temporario(const char *operando);

/* ---------- geracao ---------- */

/*
 * Gera o codigo de uma expressao e devolve o operando (identificador,
 * temporario ou constante) que guarda o seu resultado.
 */
const char *ir_gera_expressao(const AstNo *expressao);

/* Gera o codigo de um comando. */
void ir_gera_comando(const AstNo *comando);

/*
 * Gera uma chamada: avalia os argumentos, emite os param e o call.
 * Se usa_resultado, devolve o temporario com o retorno; senao, NULL.
 */
const char *ir_gera_chamada(const AstNo *chamada, int usa_resultado);

/* Gera o codigo de todo o programa (raiz AST_PROGRAMA). */
void ir_gera_programa(const AstNo *raiz);

/* ---------- consulta, impressao e liberacao ---------- */

const Quadrupla *ir_primeira(void);
int ir_num_instrucoes(void);

/* Imprime a RI no formato textual da secao 3, uma instrucao por linha. */
void ir_imprime(FILE *saida);

/* Libera todas as instrucoes e strings geradas. */
void ir_libera(void);

/* ---------- apoio ao otimizador (issue 6.4) ---------- */

/* Remove e libera a instrucao seguinte a "anterior" (que nao pode ser NULL). */
void ir_remove_apos(Quadrupla *anterior);

/* Copia um texto para o pool da RI (liberado por ir_libera). */
const char *ir_copia_texto(const char *texto);

/* Formata constantes no mesmo formato usado pelo gerador. */
const char *ir_formata_int(long long valor);
const char *ir_formata_float(double valor);

#endif
