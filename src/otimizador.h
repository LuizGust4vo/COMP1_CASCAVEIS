#ifndef OTIMIZADOR_H
#define OTIMIZADOR_H

/*
 * otimizador.h - otimizacoes basicas do codigo intermediario (issue 6.4).
 *
 * As otimizacoes sao opcionais (opcao --otimizar) e rodam sobre cada funcao
 * logo apos a sua geracao. Elas so alteram contas e temporarios: labels,
 * saltos e o padrao de curto-circuito de && e || ficam intactos, porque o
 * gerador de Python reconstroi if/while a partir deles.
 *
 * Detalhes e exemplos: docs/representacao_intermediaria.md, secao 9.
 */

#include "ir.h"

/* Liga ou desliga as otimizacoes (desligadas por padrao). */
void otimizador_ativa(int ativa);
int otimizador_ativo(void);

/*
 * Otimiza a funcao que comeca na instrucao "func" (IR_FUNC) e termina no
 * IR_FIM_FUNC seguinte. Deve ser chamada enquanto a funcao ainda e a atual
 * do gerador, para que ir_eh_temporario reconheca os temporarios dela.
 */
void otimizador_otimiza_funcao(Quadrupla *func);

/* Estatisticas acumuladas desde o ultimo otimizador_reinicia(). */
typedef struct {
    int dobras_constantes;   /* constant folding */
    int simplificacoes;      /* x + 0, x * 1, ... */
    int propagacoes;         /* usos de temporario trocados pela constante */
    int copias_eliminadas;   /* t = a op b; x = t  ->  x = a op b */
    int temporarios_mortos;  /* definicoes de temporarios nunca usados */
    int inalcancaveis;       /* instrucoes apos return */
} OtimizadorEstatisticas;

OtimizadorEstatisticas otimizador_estatisticas(void);
void otimizador_reinicia(void);

#endif
