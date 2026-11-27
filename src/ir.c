/*
 * ir.c - geracao de codigo intermediario a partir da AST (issue 4.3).
 *
 * Formato e padroes de geracao: docs/representacao_intermediaria.md.
 *
 * Divisao da issue:
 *   - infraestrutura, expressoes, declaracoes, atribuicoes e impressao: Felipe;
 *   - blocos, if/else, while, for, funcoes, chamadas e return: Pedro.
 */

#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "tabela_simbolos.h"
#include "otimizador.h"

/* ================= memoria ================= */

static void falta_memoria(void) {
    fprintf(stderr, "Erro interno: memoria insuficiente\n");
    exit(1);
}

/* Todas as strings da RI ficam num pool unico, liberado por ir_libera(). */
typedef struct StrPool {
    char *texto;
    struct StrPool *proximo;
} StrPool;

static StrPool *pool = NULL;

static char *pool_guarda(char *texto) {
    StrPool *item = malloc(sizeof(StrPool));
    if (!item) {
        falta_memoria();
    }
    item->texto = texto;
    item->proximo = pool;
    pool = item;
    return texto;
}

static char *pool_copia(const char *texto) {
    if (!texto) {
        return NULL;
    }
    char *copia = malloc(strlen(texto) + 1);
    if (!copia) {
        falta_memoria();
    }
    strcpy(copia, texto);
    return pool_guarda(copia);
}

static char *pool_formata_int(char prefixo, int n) {
    char buf[32];
    if (prefixo) {
        snprintf(buf, sizeof buf, "%c%d", prefixo, n);
    } else {
        snprintf(buf, sizeof buf, "%d", n);
    }
    return pool_copia(buf);
}

/* ================= estado da geracao ================= */

static int contador_temp = 0;
static int contador_label = 0;

/* nomes de parametros e variaveis da funcao atual */
static const char **nomes_funcao = NULL;
static int num_nomes = 0;
static int cap_nomes = 0;

/* ================= lista de instrucoes ================= */

static Quadrupla *primeira = NULL;
static Quadrupla *ultima = NULL;
static int num_instrucoes = 0;

Quadrupla *ir_emite(IrOp op, const char *arg1, const char *arg2, const char *resultado) {
    Quadrupla *q = calloc(1, sizeof(Quadrupla));
    if (!q) {
        falta_memoria();
    }
    q->op = op;
    q->arg1 = pool_copia(arg1);
    q->arg2 = pool_copia(arg2);
    q->resultado = pool_copia(resultado);

    if (ultima) {
        ultima->proximo = q;
    } else {
        primeira = q;
    }
    ultima = q;
    num_instrucoes++;
    return q;
}

const Quadrupla *ir_primeira(void) {
    return primeira;
}

int ir_num_instrucoes(void) {
    return num_instrucoes;
}

void ir_libera(void) {
    Quadrupla *q = primeira;
    while (q) {
        Quadrupla *prox = q->proximo;
        free(q);
        q = prox;
    }
    primeira = ultima = NULL;
    num_instrucoes = 0;

    free(nomes_funcao);
    nomes_funcao = NULL;
    num_nomes = cap_nomes = 0;
    contador_temp = contador_label = 0;

    while (pool) {
        StrPool *prox = pool->proximo;
        free(pool->texto);
        free(pool);
        pool = prox;
    }
}

void ir_remove_apos(Quadrupla *anterior) {
    Quadrupla *alvo = anterior ? anterior->proximo : NULL;
    if (!alvo) {
        return;
    }
    anterior->proximo = alvo->proximo;
    if (ultima == alvo) {
        ultima = anterior;
    }
    num_instrucoes--;
    free(alvo);
}

const char *ir_copia_texto(const char *texto) {
    return pool_copia(texto);
}

const char *ir_formata_int(long long valor) {
    char buf[32];
    snprintf(buf, sizeof buf, "%lld", valor);
    return pool_copia(buf);
}

/* ================= temporarios e labels ================= */

static void registra_nome(const char *nome) {
    if (num_nomes == cap_nomes) {
        cap_nomes = cap_nomes ? cap_nomes * 2 : 16;
        const char **novo = realloc(nomes_funcao, cap_nomes * sizeof(*novo));
        if (!novo) {
            falta_memoria();
        }
        nomes_funcao = novo;
    }
    nomes_funcao[num_nomes++] = nome;
}

static int nome_da_funcao(const char *nome) {
    for (int i = 0; i < num_nomes; i++) {
        if (strcmp(nomes_funcao[i], nome) == 0) {
            return 1;
        }
    }
    return 0;
}

/* percorre a subarvore registrando parametros e declaracoes */
static void coleta_nomes(const AstNo *no) {
    for (; no; no = no->proximo) {
        if ((no->tipo == AST_PARAMETRO || no->tipo == AST_DECLARACAO) && no->texto) {
            registra_nome(no->texto);
        }
        for (int i = 0; i < 4; i++) {
            coleta_nomes(no->filho[i]);
        }
    }
}

void ir_inicia_funcao(const AstNo *funcao) {
    contador_temp = 0;
    contador_label = 0;
    num_nomes = 0;
    if (funcao) {
        coleta_nomes(funcao->filho[0]); /* parametros */
        coleta_nomes(funcao->filho[1]); /* corpo */
    }
}

const char *ir_novo_temp(void) {
    char *nome;
    do {
        nome = pool_formata_int('t', ++contador_temp);
    } while (nome_da_funcao(nome));
    return nome;
}

const char *ir_novo_label(void) {
    return pool_formata_int('L', ++contador_label);
}

int ir_eh_temporario(const char *operando) {
    if (!operando || operando[0] != 't' || operando[1] == '\0') {
        return 0;
    }
    for (const char *p = operando + 1; *p; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
    }
    int n = atoi(operando + 1);
    return n >= 1 && n <= contador_temp && !nome_da_funcao(operando);
}

/* ================= operandos constantes ================= */

/* Menor representacao decimal que preserva o valor, sempre com ponto. */
const char *ir_formata_float(double valor) {
    char buf[64];
    for (int precisao = 1; precisao <= 17; precisao++) {
        snprintf(buf, sizeof buf, "%.*g", precisao, valor);
        if (strtod(buf, NULL) == valor) {
            break;
        }
    }
    if (!strpbrk(buf, ".eEn")) { /* "n" cobre inf/nan */
        strncat(buf, ".0", sizeof buf - strlen(buf) - 1);
    }
    return pool_copia(buf);
}

static const char *entre(char delimitador, const char *texto) {
    size_t n = strlen(texto);
    char *s = malloc(n + 3);
    if (!s) {
        falta_memoria();
    }
    s[0] = delimitador;
    memcpy(s + 1, texto, n);
    s[n + 1] = delimitador;
    s[n + 2] = '\0';
    return pool_guarda(s);
}

/* ================= expressoes ================= */

static IrOp op_binario(const AstNo *e) {
    switch (e->op) {
        case OP_SOMA:          return IR_SOMA;
        case OP_SUB:           return IR_SUB;
        case OP_MUL:           return IR_MUL;
        case OP_DIV:
            /* tipos gravados pela analise semantica */
            return (e->filho[0]->tipo_dado == TIPO_FLOAT ||
                    e->filho[1]->tipo_dado == TIPO_FLOAT)
                       ? IR_DIV
                       : IR_DIV_INT;
        case OP_MENOR:         return IR_MENOR;
        case OP_MAIOR:         return IR_MAIOR;
        case OP_MENOR_IGUAL:   return IR_MENOR_IGUAL;
        case OP_MAIOR_IGUAL:   return IR_MAIOR_IGUAL;
        case OP_IGUAL:         return IR_IGUAL;
        case OP_DIFERENTE:     return IR_DIFERENTE;
        default:               return IR_SOMA; /* nao ocorre: && e || tratados a parte */
    }
}

/*
 * a && b  /  a || b com curto-circuito (secao 6 da RI):
 *
 *   <a>
 *   r = a                 ; omitido quando a ja e um temporario
 *   ifFalse r goto L      ; (if r goto L para ||)
 *   <b>
 *   r = b
 *   L:
 *
 * O label e criado ao visitar o operador, antes de gerar os operandos.
 */
static const char *gera_logico(const AstNo *e) {
    const char *fim = ir_novo_label();

    const char *a = ir_gera_expressao(e->filho[0]);
    const char *r = a;
    if (!ir_eh_temporario(a)) {
        r = ir_novo_temp();
        ir_emite(IR_ATRIB, a, NULL, r);
    }
    ir_emite(e->op == OP_E ? IR_IF_FALSE : IR_IF, r, NULL, fim);

    const char *b = ir_gera_expressao(e->filho[1]);
    ir_emite(IR_ATRIB, b, NULL, r);
    ir_emite(IR_LABEL, NULL, NULL, fim);
    return r;
}

const char *ir_gera_expressao(const AstNo *e) {
    if (!e) {
        return NULL;
    }
    switch (e->tipo) {
        case AST_ID:
            return pool_copia(e->texto);

        case AST_NUM_INT:
            return pool_formata_int(0, e->valor_int);

        case AST_NUM_FLOAT:
            return ir_formata_float(e->valor_float);

        case AST_CHAR_LIT:
            return entre('\'', e->texto);

        case AST_STRING_LIT:
            return entre('"', e->texto);

        case AST_CHAMADA:
            return ir_gera_chamada(e, 1);

        case AST_UNARIO: {
            const char *a = ir_gera_expressao(e->filho[0]);
            const char *t = ir_novo_temp();
            ir_emite(e->op == OP_NAO ? IR_NAO : IR_NEGATIVO, a, NULL, t);
            return t;
        }

        case AST_BINARIO: {
            if (e->op == OP_E || e->op == OP_OU) {
                return gera_logico(e);
            }
            const char *a = ir_gera_expressao(e->filho[0]);
            const char *b = ir_gera_expressao(e->filho[1]);
            const char *t = ir_novo_temp();
            ir_emite(op_binario(e), a, b, t);
            return t;
        }

        default:
            fprintf(stderr, "Erro interno: no %d nao e uma expressao\n", e->tipo);
            return NULL;
    }
}

/* ================= comandos ================= */

/* int x;  ->  decl int x      int x = e;  ->  <e>; x = e */
static void gera_declaracao(const AstNo *d) {
    if (d->filho[0]) {
        const char *v = ir_gera_expressao(d->filho[0]);
        ir_emite(IR_ATRIB, v, NULL, d->texto);
    } else {
        ir_emite(IR_DECL, ast_nome_tipo_dado(d->tipo_dado), NULL, d->texto);
    }
}

/* x = e  ->  <e>; x = e   (copia direta quando e ja e um operando) */
static void gera_atribuicao(const AstNo *a) {
    const char *v = ir_gera_expressao(a->filho[0]);
    ir_emite(IR_ATRIB, v, NULL, a->texto);
}

/* if sem else: ifFalse c goto L1; <then>; L1:
   com else:     ifFalse c goto L1; <then>; goto L2; L1: <else>; L2: */
static void gera_if(const AstNo *c) {
    const char *senao = ir_novo_label();
    const char *fim = NULL;
    if (c->filho[2]) {
        fim = ir_novo_label();
    }

    const char *cond = ir_gera_expressao(c->filho[0]);
    ir_emite(IR_IF_FALSE, cond, NULL, senao);
    ir_gera_comando(c->filho[1]);
    if (c->filho[2]) {
        ir_emite(IR_GOTO, NULL, NULL, fim);
        ir_emite(IR_LABEL, NULL, NULL, senao);
        ir_gera_comando(c->filho[2]);
        ir_emite(IR_LABEL, NULL, NULL, fim);
    } else {
        ir_emite(IR_LABEL, NULL, NULL, senao);
    }
}

/* L1: <cond>; ifFalse c goto L2; <corpo>; goto L1; L2: */
static void gera_while(const AstNo *c) {
    const char *inicio = ir_novo_label();
    const char *fim = ir_novo_label();

    ir_emite(IR_LABEL, NULL, NULL, inicio);
    const char *cond = ir_gera_expressao(c->filho[0]);
    ir_emite(IR_IF_FALSE, cond, NULL, fim);
    ir_gera_comando(c->filho[1]);
    ir_emite(IR_GOTO, NULL, NULL, inicio);
    ir_emite(IR_LABEL, NULL, NULL, fim);
}

/* igual ao while, com o init antes de L1 e o passo depois do corpo */
static void gera_for(const AstNo *c) {
    const char *inicio = ir_novo_label();
    const char *fim = ir_novo_label();

    ir_gera_comando(c->filho[0]);
    ir_emite(IR_LABEL, NULL, NULL, inicio);
    const char *cond = ir_gera_expressao(c->filho[1]);
    ir_emite(IR_IF_FALSE, cond, NULL, fim);
    ir_gera_comando(c->filho[3]);
    ir_gera_comando(c->filho[2]);
    ir_emite(IR_GOTO, NULL, NULL, inicio);
    ir_emite(IR_LABEL, NULL, NULL, fim);
}

void ir_gera_comando(const AstNo *c) {
    if (!c) {
        return;
    }
    switch (c->tipo) {
        case AST_DECLARACAO:
            gera_declaracao(c);
            break;

        case AST_ATRIBUICAO:
            gera_atribuicao(c);
            break;

        case AST_BLOCO:
            for (const AstNo *cmd = c->filho[0]; cmd; cmd = cmd->proximo) {
                ir_gera_comando(cmd);
            }
            break;

        case AST_IF:
            gera_if(c);
            break;

        case AST_WHILE:
            gera_while(c);
            break;

        case AST_FOR:
            gera_for(c);
            break;

        case AST_RETORNO: {
            const char *v = ir_gera_expressao(c->filho[0]); /* NULL se for so "return;" */
            ir_emite(IR_RETURN, v, NULL, NULL);
            break;
        }

        case AST_CHAMADA:
            ir_gera_chamada(c, 0);
            break;

        default:
            fprintf(stderr, "Erro interno: no %d nao e um comando\n", c->tipo);
            break;
    }
}

/*
 * Avalia todos os argumentos antes de emitir os param, para que nenhuma
 * outra chamada fique entre os param e o call (secao 4 da RI).
 */
const char *ir_gera_chamada(const AstNo *chamada, int usa_resultado) {
    int n = 0;
    for (const AstNo *a = chamada->filho[0]; a; a = a->proximo) {
        n++;
    }

    const char **args = NULL;
    if (n > 0) {
        args = malloc(n * sizeof(*args));
        if (!args) {
            falta_memoria();
        }
    }
    int i = 0;
    for (const AstNo *a = chamada->filho[0]; a; a = a->proximo) {
        args[i++] = ir_gera_expressao(a);
    }

    for (i = 0; i < n; i++) {
        ir_emite(IR_PARAM, args[i], NULL, NULL);
    }
    free(args);

    const char *t = NULL;
    if (usa_resultado) {
        t = ir_novo_temp();
    }
    ir_emite(IR_CALL, chamada->texto, pool_formata_int(0, n), t);
    return t;
}

/* func f(...) -> T; fparam...; <corpo>; endfunc  (prototipos nao geram nada) */
void ir_gera_programa(const AstNo *raiz) {
    for (const AstNo *f = raiz->filho[0]; f; f = f->proximo) {
        if (!f->filho[1]) {
            continue;
        }
        ir_inicia_funcao(f);
        Quadrupla *inicio = ir_emite(IR_FUNC, ast_nome_tipo_dado(f->tipo_dado), NULL, f->texto);
        for (const AstNo *p = f->filho[0]; p; p = p->proximo) {
            ir_emite(IR_FPARAM, ast_nome_tipo_dado(p->tipo_dado), NULL, p->texto);
        }
        ir_gera_comando(f->filho[1]);
        ir_emite(IR_FIM_FUNC, NULL, NULL, NULL);

        /* com a funcao ainda "atual", ir_eh_temporario distingue tN de variaveis */
        if (otimizador_ativo()) {
            otimizador_otimiza_funcao(inicio);
        }
    }
}

/* ================= impressao ================= */

static const char *simbolo_ir(IrOp op) {
    switch (op) {
        case IR_SOMA:         return "+";
        case IR_SUB:          return "-";
        case IR_MUL:          return "*";
        case IR_DIV:          return "/";
        case IR_DIV_INT:      return "//";
        case IR_MENOR:        return "<";
        case IR_MAIOR:        return ">";
        case IR_MENOR_IGUAL:  return "<=";
        case IR_MAIOR_IGUAL:  return ">=";
        case IR_IGUAL:        return "==";
        case IR_DIFERENTE:    return "!=";
        case IR_NEGATIVO:     return "-";
        case IR_NAO:          return "!";
        default:              return "?";
    }
}

static const char *ou_vazio(const char *s) {
    return s ? s : "";
}

void ir_imprime(FILE *saida) {
    for (const Quadrupla *q = primeira; q; q = q->proximo) {
        switch (q->op) {
            case IR_SOMA: case IR_SUB: case IR_MUL: case IR_DIV: case IR_DIV_INT:
            case IR_MENOR: case IR_MAIOR: case IR_MENOR_IGUAL: case IR_MAIOR_IGUAL:
            case IR_IGUAL: case IR_DIFERENTE:
                fprintf(saida, "%s = %s %s %s\n", q->resultado, q->arg1, simbolo_ir(q->op), q->arg2);
                break;
            case IR_NEGATIVO:
            case IR_NAO:
                fprintf(saida, "%s = %s%s\n", q->resultado, simbolo_ir(q->op), q->arg1);
                break;
            case IR_ATRIB:
                fprintf(saida, "%s = %s\n", q->resultado, q->arg1);
                break;
            case IR_DECL:
                fprintf(saida, "decl %s %s\n", q->arg1, q->resultado);
                break;
            case IR_LABEL:
                fprintf(saida, "%s:\n", q->resultado);
                break;
            case IR_GOTO:
                fprintf(saida, "goto %s\n", q->resultado);
                break;
            case IR_IF:
                fprintf(saida, "if %s goto %s\n", q->arg1, q->resultado);
                break;
            case IR_IF_FALSE:
                fprintf(saida, "ifFalse %s goto %s\n", q->arg1, q->resultado);
                break;
            case IR_FUNC: {
                /* func f(T p1, T p2) -> T : os IR_FPARAM seguintes formam a lista */
                const Quadrupla *func = q;
                fprintf(saida, "func %s(", func->resultado);
                const char *sep = "";
                while (q->proximo && q->proximo->op == IR_FPARAM) {
                    q = q->proximo;
                    fprintf(saida, "%s%s %s", sep, q->arg1, q->resultado);
                    sep = ", ";
                }
                fprintf(saida, ") -> %s\n", ou_vazio(func->arg1));
                break;
            }
            case IR_FPARAM: /* so aparece logo apos IR_FUNC */
                fprintf(saida, "fparam %s %s\n", q->arg1, q->resultado);
                break;
            case IR_FIM_FUNC:
                fprintf(saida, "endfunc\n");
                break;
            case IR_PARAM:
                fprintf(saida, "param %s\n", q->arg1);
                break;
            case IR_CALL:
                if (q->resultado) {
                    fprintf(saida, "%s = call %s, %s\n", q->resultado, q->arg1, q->arg2);
                } else {
                    fprintf(saida, "call %s, %s\n", q->arg1, q->arg2);
                }
                break;
            case IR_RETURN:
                if (q->arg1) {
                    fprintf(saida, "return %s\n", q->arg1);
                } else {
                    fprintf(saida, "return\n");
                }
                break;
        }
    }
}
