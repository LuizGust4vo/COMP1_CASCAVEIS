#include <stdio.h>
#include "semantico.h"
#include "tabela_simbolos.h"

/*
 * Verificacoes semanticas basicas (issue 3.5).
 *
 * O cadastro de nomes e o vinculo de cada uso ao seu simbolo ja foram feitos
 * pelo parser (issue 3.4). Aqui apenas percorremos a AST:
 *   - no com simbolo == NULL  -> nome nao declarado ou fora de escopo;
 *   - problemas registrados pela tabela -> redeclaracoes;
 *   - tipos simples conforme a secao 7.1 da especificacao.
 */

/* tipos usados pela analise: os da AST, mais string literal e "erro" */
typedef enum { T_INT, T_FLOAT, T_CHAR, T_VOID, T_STRING, T_ERRO } TipoSem;

static int erros = 0;
static TipoSem retorno_atual = T_VOID;

static void erro(int linha, const char *msg, const char *nome) {
    if (nome)
        fprintf(stderr, "Erro semantico na linha %d: %s '%s'\n", linha, msg, nome);
    else
        fprintf(stderr, "Erro semantico na linha %d: %s\n", linha, msg);
    erros++;
}

static TipoSem de_ast(AstTipoDado t) {
    switch (t) {
        case TIPO_INT:   return T_INT;
        case TIPO_FLOAT: return T_FLOAT;
        case TIPO_CHAR:  return T_CHAR;
        default:         return T_VOID;
    }
}

static int numerico(TipoSem t) {
    return t == T_INT || t == T_FLOAT || t == T_CHAR;
}

static int eh_funcao(const Simbolo *s) {
    return s->categoria == SIM_FUNCAO || s->categoria == SIM_FUNCAO_NATIVA;
}

/* Tabela 7.1.2: o valor do tipo "origem" pode ser guardado em "destino"? */
static int compativel(TipoSem destino, TipoSem origem) {
    if (origem == T_ERRO) return 1; /* erro ja reportado; evita cascata */
    switch (destino) {
        case T_INT:   return origem == T_INT || origem == T_CHAR;
        case T_FLOAT: return origem == T_FLOAT || origem == T_INT || origem == T_CHAR;
        case T_CHAR:  return origem == T_CHAR;
        default:      return 0;
    }
}

/* Calcula o tipo de uma expressao (tabela 7.1.1), reportando erros no caminho */
static TipoSem tipo_expr(const AstNo *e) {
    if (!e) return T_VOID;

    switch (e->tipo) {
    case AST_NUM_INT:    return T_INT;
    case AST_NUM_FLOAT:  return T_FLOAT;
    case AST_CHAR_LIT:   return T_CHAR;
    case AST_STRING_LIT: return T_STRING;

    case AST_ID:
        if (!e->simbolo) {
            erro(e->linha, "variavel nao declarada ou fora de escopo", e->texto);
            return T_ERRO;
        }
        if (eh_funcao(e->simbolo)) {
            erro(e->linha, "nome de funcao usado como valor", e->texto);
            return T_ERRO;
        }
        return de_ast(e->simbolo->tipo);

    case AST_CHAMADA: {
        const Simbolo *f = e->simbolo;
        if (!f) {
            erro(e->linha, "funcao nao declarada", e->texto);
        } else if (!eh_funcao(f)) {
            erro(e->linha, "variavel chamada como funcao", e->texto);
            f = NULL;
        }
        /* STRING_LIT so e aceita como argumento direto de funcao nativa (7.1.3) */
        for (const AstNo *a = e->filho[0]; a; a = a->proximo) {
            if (a->tipo == AST_STRING_LIT) {
                if (f && f->categoria != SIM_FUNCAO_NATIVA)
                    erro(a->linha, "string literal como argumento de funcao nao nativa", e->texto);
            } else {
                tipo_expr(a);
            }
        }
        return f ? de_ast(f->tipo) : T_ERRO;
    }

    case AST_UNARIO: {
        TipoSem t = tipo_expr(e->filho[0]);
        if (t == T_ERRO) return T_ERRO;
        if (!numerico(t)) {
            erro(e->linha, "incompatibilidade de tipos: operando string/void em operacao unaria", NULL);
            return T_ERRO;
        }
        if (e->op == OP_NAO) return T_INT;
        return t == T_CHAR ? T_INT : t;
    }

    case AST_BINARIO: {
        TipoSem a = tipo_expr(e->filho[0]);
        TipoSem b = tipo_expr(e->filho[1]);
        if (a == T_ERRO || b == T_ERRO) return T_ERRO;
        if (!numerico(a) || !numerico(b)) {
            erro(e->linha, "incompatibilidade de tipos: operando string/void em operacao", NULL);
            return T_ERRO;
        }
        switch (e->op) {
            case OP_SOMA: case OP_SUB: case OP_MUL: case OP_DIV:
                return (a == T_FLOAT || b == T_FLOAT) ? T_FLOAT : T_INT;
            default: /* relacionais e logicos */
                return T_INT;
        }
    }

    default:
        return T_ERRO;
    }
}

static void visita(const AstNo *n);

static void visita_lista(const AstNo *n) {
    for (; n; n = n->proximo) visita(n);
}

static void visita(const AstNo *n) {
    if (!n) return;

    switch (n->tipo) {
    case AST_PROGRAMA:
        visita_lista(n->filho[0]);
        break;

    case AST_FUNCAO:
        /* prototipo tem corpo NULL; visita(NULL) nao faz nada */
        retorno_atual = de_ast(n->tipo_dado);
        visita(n->filho[1]);
        break;

    case AST_BLOCO:
        visita_lista(n->filho[0]);
        break;

    case AST_DECLARACAO:
        if (n->filho[0]) {
            TipoSem t = tipo_expr(n->filho[0]);
            if (!compativel(de_ast(n->tipo_dado), t))
                erro(n->linha, "inicializacao com tipo incompativel", n->texto);
        }
        break;

    case AST_ATRIBUICAO: {
        TipoSem t = tipo_expr(n->filho[0]);
        if (!n->simbolo) {
            erro(n->linha, "variavel nao declarada ou fora de escopo", n->texto);
        } else if (eh_funcao(n->simbolo)) {
            erro(n->linha, "atribuicao a nome de funcao", n->texto);
        } else if (!compativel(de_ast(n->simbolo->tipo), t)) {
            erro(n->linha, "atribuicao com tipo incompativel", n->texto);
        }
        break;
    }

    case AST_RETORNO:
        if (n->filho[0]) {
            TipoSem t = tipo_expr(n->filho[0]);
            if (!compativel(retorno_atual, t))
                erro(n->linha, "retorno com tipo incompativel com a funcao", NULL);
        }
        break;

    case AST_IF: {
        TipoSem c = tipo_expr(n->filho[0]);
        if (c != T_ERRO && !numerico(c))
            erro(n->linha, "condicao do if deve ser numerica", NULL);
        visita(n->filho[1]);
        visita(n->filho[2]);
        break;
    }

    case AST_CHAMADA: /* chamada usada como comando, ex.: printf("oi"); */
        tipo_expr(n);
        break;

    default:
        break;
    }
}

/* Erros que a tabela de simbolos registrou durante o cadastro (S3-04) */
static void reporta_problemas_da_tabela(void) {
    for (const Problema *p = ts_problemas(); p; p = p->proximo) {
        const char *msg;
        switch (p->tipo) {
            case PROB_REDECLARACAO:      msg = "redeclaracao no mesmo escopo"; break;
            case PROB_OCULTA:            msg = "nome ja declarado em escopo externo da funcao"; break;
            case PROB_FUNCAO_REDEFINIDA: msg = "funcao definida duas vezes"; break;
            default:                     msg = "problema de declaracao"; break;
        }
        erro(p->linha, msg, p->nome);
    }
}

int semantico_analisa(const AstNo *raiz) {
    erros = 0;
    retorno_atual = T_VOID;
    reporta_problemas_da_tabela();
    visita(raiz);
    return erros;
}
