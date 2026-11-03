

#ifndef TABELA_SIMBOLOS_H
#define TABELA_SIMBOLOS_H

#include <stdio.h>
#include "ast.h"

typedef enum {
    SIM_VARIAVEL,
    SIM_PARAMETRO,
    SIM_FUNCAO,
    SIM_FUNCAO_NATIVA
} SimCategoria;

typedef struct Simbolo {
    char *nome;               
    AstTipoDado tipo;       
    SimCategoria categoria;
    int escopo;               
    int linha;                
    int visivel;              
    int definida;             
    int num_parametros;      
    struct Simbolo *proximo;  
} Simbolo;

typedef enum {
    PROB_REDECLARACAO,        
    PROB_OCULTA,              
    PROB_FUNCAO_REDEFINIDA    
} ProblemaTipo;

typedef struct Problema {
    ProblemaTipo tipo;
    char *nome;
    int linha;                
    const Simbolo *anterior;  
    struct Problema *proximo;
} Problema;

void ts_inicia(void);


void ts_libera(void);


void ts_abre_escopo(void);
void ts_fecha_escopo(void);


int ts_escopo_atual(void);


Simbolo *ts_insere(const char *nome, AstTipoDado tipo, SimCategoria categoria, int linha);

Simbolo *ts_insere_funcao(const char *nome, AstTipoDado retorno, int num_parametros,
                          int definicao, int linha);


Simbolo *ts_busca_escopo_atual(const char *nome);  
Simbolo *ts_busca_funcao_atual(const char *nome);   
Simbolo *ts_busca_visivel(const char *nome);      
Simbolo *ts_busca_global(const char *nome);         


const Simbolo *ts_primeiro(void);
const Problema *ts_problemas(void);
int ts_num_simbolos(void);
int ts_num_problemas(void);

void ts_imprime(FILE *saida);

const char *ts_nome_categoria(SimCategoria categoria);

#endif