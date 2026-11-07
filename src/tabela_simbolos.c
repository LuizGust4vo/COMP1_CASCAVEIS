#include <stdlib.h>
#include <string.h>
#include "tabela_simbolos.h"

static Simbolo *primeiro = NULL;
static Simbolo *ultimo = NULL;
static Problema *primeiro_problema = NULL;
static Problema *ultimo_problema = NULL;
static int total_simbolos = 0;
static int total_problemas = 0;

static int *pilha = NULL;
static int topo = -1;
static int capacidade = 0;
static int proximo_id = 0;

static void falta_memoria(void) {
    fprintf(stderr, "Erro interno: memoria insuficiente\n");
    exit(1);
}

static char *copia_texto(const char *texto) {
    char *copia = malloc(strlen(texto) + 1);
    if (!copia) {
        falta_memoria();
    }
    strcpy(copia, texto);
    return copia;
}

static void empilha(int id) {
    if (topo + 1 >= capacidade) {
        capacidade = capacidade ? capacidade * 2 : 16;
        int *nova = realloc(pilha, (size_t)capacidade * sizeof(int));
        if (!nova) {
            falta_memoria();
        }
        pilha = nova;
    }
    pilha[++topo] = id;
}

static Simbolo *novo_simbolo(const char *nome, AstTipoDado tipo, SimCategoria categoria, int linha) {
    Simbolo *s = calloc(1, sizeof(Simbolo));
    if (!s) {
        falta_memoria();
    }
    s->nome = copia_texto(nome);
    s->tipo = tipo;
    s->categoria = categoria;
    s->escopo = pilha[topo];
    s->linha = linha;
    s->visivel = 1;
    if (ultimo) {
        ultimo->proximo = s;
    } else {
        primeiro = s;
    }
    ultimo = s;
    total_simbolos++;
    return s;
}

static void registra_problema(ProblemaTipo tipo, const char *nome, int linha, const Simbolo *anterior) {
    Problema *p = calloc(1, sizeof(Problema));
    if (!p) {
        falta_memoria();
    }
    p->tipo = tipo;
    p->nome = copia_texto(nome);
    p->linha = linha;
    p->anterior = anterior;
    if (ultimo_problema) {
        ultimo_problema->proximo = p;
    } else {
        primeiro_problema = p;
    }
    ultimo_problema = p;
    total_problemas++;
}

void ts_libera(void) {
    while (primeiro) {
        Simbolo *proximo = primeiro->proximo;
        free(primeiro->nome);
        free(primeiro);
        primeiro = proximo;
    }
    while (primeiro_problema) {
        Problema *proximo = primeiro_problema->proximo;
        free(primeiro_problema->nome);
        free(primeiro_problema);
        primeiro_problema = proximo;
    }
    ultimo = NULL;
    ultimo_problema = NULL;
    total_simbolos = 0;
    total_problemas = 0;
    free(pilha);
    pilha = NULL;
    topo = -1;
    capacidade = 0;
    proximo_id = 0;
}

void ts_inicia(void) {
    ts_libera();
    empilha(proximo_id++);
    Simbolo *printf_nativo = novo_simbolo("printf", TIPO_VOID, SIM_FUNCAO_NATIVA, 0);
    printf_nativo->definida = 1;
    printf_nativo->num_parametros = 1;
}

void ts_abre_escopo(void) {
    if (topo < 0) {
        ts_inicia();
    }
    empilha(proximo_id++);
}

void ts_fecha_escopo(void) {
    if (topo <= 0) {
        return;
    }
    int id = pilha[topo--];
    for (Simbolo *s = primeiro; s; s = s->proximo) {
        if (s->escopo == id) {
            s->visivel = 0;
        }
    }
}

int ts_escopo_atual(void) {
    return topo >= 0 ? pilha[topo] : 0;
}

Simbolo *ts_busca_escopo_atual(const char *nome) {
    for (Simbolo *s = primeiro; s; s = s->proximo) {
        if (s->visivel && s->escopo == ts_escopo_atual() && strcmp(s->nome, nome) == 0) {
            return s;
        }
    }
    return NULL;
}

Simbolo *ts_busca_funcao_atual(const char *nome) {
    for (Simbolo *s = primeiro; s; s = s->proximo) {
        if (s->visivel && s->escopo != 0 && strcmp(s->nome, nome) == 0) {
            return s;
        }
    }
    return NULL;
}

Simbolo *ts_busca_visivel(const char *nome) {
    for (Simbolo *s = primeiro; s; s = s->proximo) {
        if (s->visivel && strcmp(s->nome, nome) == 0) {
            return s;
        }
    }
    return NULL;
}

Simbolo *ts_busca_global(const char *nome) {
    for (Simbolo *s = primeiro; s; s = s->proximo) {
        if (s->escopo == 0 && strcmp(s->nome, nome) == 0) {
            return s;
        }
    }
    return NULL;
}

Simbolo *ts_insere(const char *nome, AstTipoDado tipo, SimCategoria categoria, int linha) {
    if (topo < 0) {
        ts_inicia();
    }
    Simbolo *existente = ts_busca_escopo_atual(nome);
    if (existente) {
        registra_problema(PROB_REDECLARACAO, nome, linha, existente);
        return NULL;
    }
    existente = ts_busca_funcao_atual(nome);
    if (existente) {
        registra_problema(PROB_OCULTA, nome, linha, existente);
        return NULL;
    }
    return novo_simbolo(nome, tipo, categoria, linha);
}

Simbolo *ts_insere_funcao(const char *nome, AstTipoDado retorno, int num_parametros,
                          int definicao, int linha) {
    if (topo < 0) {
        ts_inicia();
    }
    Simbolo *existente = ts_busca_global(nome);
    if (!existente) {
        int atual = pilha[topo];
        pilha[topo] = 0;
        Simbolo *s = novo_simbolo(nome, retorno, SIM_FUNCAO, linha);
        pilha[topo] = atual;
        s->definida = definicao;
        s->num_parametros = num_parametros;
        return s;
    }
    if (existente->categoria == SIM_FUNCAO_NATIVA) {
        registra_problema(PROB_REDECLARACAO, nome, linha, existente);
        return NULL;
    }
    if (definicao && existente->definida) {
        registra_problema(PROB_FUNCAO_REDEFINIDA, nome, linha, existente);
        return NULL;
    }
    if (definicao) {
        existente->definida = 1;
    }
    return existente;
}

const Simbolo *ts_primeiro(void) {
    return primeiro;
}

const Problema *ts_problemas(void) {
    return primeiro_problema;
}

int ts_num_simbolos(void) {
    return total_simbolos;
}

int ts_num_problemas(void) {
    return total_problemas;
}

const char *ts_nome_categoria(SimCategoria categoria) {
    switch (categoria) {
        case SIM_VARIAVEL:       return "variavel";
        case SIM_PARAMETRO:      return "parametro";
        case SIM_FUNCAO:         return "funcao";
        case SIM_FUNCAO_NATIVA:  return "funcao nativa";
    }
    return "?";
}

static const char *nome_problema(ProblemaTipo tipo) {
    switch (tipo) {
        case PROB_REDECLARACAO:      return "redeclaracao";
        case PROB_OCULTA:            return "oculta declaracao externa";
        case PROB_FUNCAO_REDEFINIDA: return "funcao definida duas vezes";
    }
    return "?";
}

void ts_imprime(FILE *saida) {
    fprintf(saida, "Tabela de simbolos:\n");
    for (const Simbolo *s = primeiro; s; s = s->proximo) {
        fprintf(saida, "  [escopo %d] %s %s %s (linha %d",
                s->escopo, ts_nome_categoria(s->categoria),
                ast_nome_tipo_dado(s->tipo), s->nome, s->linha);
        if (s->categoria == SIM_FUNCAO || s->categoria == SIM_FUNCAO_NATIVA) {
            fprintf(saida, ", %d parametro(s)%s", s->num_parametros,
                    s->definida ? ", definida" : ", so prototipo");
        }
        fprintf(saida, ")\n");
    }
    if (primeiro_problema) {
        fprintf(saida, "Problemas de cadastro:\n");
        for (const Problema *p = primeiro_problema; p; p = p->proximo) {
            fprintf(saida, "  linha %d: '%s' - %s (declaracao anterior na linha %d)\n",
                    p->linha, p->nome, nome_problema(p->tipo), p->anterior->linha);
        }
    }
}