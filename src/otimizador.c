/*
 * otimizador.c - otimizacoes basicas do codigo intermediario (issue 6.4).
 *
 * Cada funcao passa repetidamente pelos passes abaixo ate nenhum deles
 * alterar mais nada:
 *
 *   1. dobra de constantes      t = 2 + 3            ->  t = 5
 *   2. simplificacao algebrica  t = u + 0, u * 1 ... ->  t = u
 *   3. propagacao de constantes t = 5; y = t * x     ->  y = 5 * x
 *   4. eliminacao de copias     t = a + b; x = t     ->  x = a + b
 *   5. temporarios mortos       t = a + b (t nunca usado) -> removida
 *   6. codigo inalcancavel      instrucoes entre return e o proximo label
 *
 * Restricoes que mantem a semantica e o gerador de Python funcionando:
 *   - nenhum label, goto, if ou ifFalse e removido ou criado;
 *   - temporarios definidos mais de uma vez (resultado de && e ||) nunca sao
 *     propagados nem renomeados, e a copia "r = b" antes do label do
 *     curto-circuito e preservada (o gerador depende dela);
 *   - chamadas nunca sao removidas, so o temporario do resultado;
 *   - contas que dependem do tipo em tempo de execucao (char, float) so sao
 *     simplificadas quando o resultado e garantidamente o mesmo;
 *   - divisao por zero e resultados fora da faixa de int nao sao dobrados,
 *     para que o programa gerado se comporte como o nao otimizado.
 */

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "otimizador.h"

static int ativo = 0;
static OtimizadorEstatisticas estat;

void otimizador_ativa(int ativa) {
    ativo = ativa;
}

int otimizador_ativo(void) {
    return ativo;
}

OtimizadorEstatisticas otimizador_estatisticas(void) {
    return estat;
}

void otimizador_reinicia(void) {
    memset(&estat, 0, sizeof estat);
}

/* ================= classificacao de instrucoes ================= */

static int eh_binario(IrOp op) {
    switch (op) {
        case IR_SOMA: case IR_SUB: case IR_MUL: case IR_DIV: case IR_DIV_INT:
        case IR_MENOR: case IR_MAIOR: case IR_MENOR_IGUAL: case IR_MAIOR_IGUAL:
        case IR_IGUAL: case IR_DIFERENTE:
            return 1;
        default:
            return 0;
    }
}

static int eh_relacional(IrOp op) {
    return op == IR_MENOR || op == IR_MAIOR || op == IR_MENOR_IGUAL ||
           op == IR_MAIOR_IGUAL || op == IR_IGUAL || op == IR_DIFERENTE;
}

static int eh_unario(IrOp op) {
    return op == IR_NEGATIVO || op == IR_NAO;
}

/* Instrucoes cujo resultado e um valor calculado (nao label, nem decl). */
static int define_valor(const Quadrupla *q) {
    return q->resultado &&
           (eh_binario(q->op) || eh_unario(q->op) || q->op == IR_ATRIB || q->op == IR_CALL);
}

/* Campos de q que sao operandos lidos. Devolve quantos (0 a 2). */
static int operandos(Quadrupla *q, char **campos[2]) {
    if (eh_binario(q->op)) {
        campos[0] = &q->arg1;
        campos[1] = &q->arg2;
        return 2;
    }
    switch (q->op) {
        case IR_NEGATIVO: case IR_NAO: case IR_ATRIB: case IR_PARAM:
        case IR_RETURN: case IR_IF: case IR_IF_FALSE:
            campos[0] = &q->arg1;
            return 1;
        default:
            return 0;
    }
}

static int mesmo(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

/* ================= consultas sobre a funcao ================= */

/* A funcao vai de func->proximo ate (excluindo) o IR_FIM_FUNC. */
#define PARA_CADA(q, func) \
    for (Quadrupla *q = (func)->proximo; q && q->op != IR_FIM_FUNC; q = q->proximo)

static int conta_usos(Quadrupla *func, const char *nome) {
    int n = 0;
    PARA_CADA(q, func) {
        char **campos[2];
        int k = operandos(q, campos);
        for (int i = 0; i < k; i++) {
            if (mesmo(*campos[i], nome)) {
                n++;
            }
        }
    }
    return n;
}

static int conta_defs(Quadrupla *func, const char *nome) {
    int n = 0;
    PARA_CADA(q, func) {
        if (define_valor(q) && mesmo(q->resultado, nome)) {
            n++;
        }
    }
    return n;
}

static void substitui_usos(Quadrupla *func, const char *nome, const char *valor) {
    PARA_CADA(q, func) {
        char **campos[2];
        int k = operandos(q, campos);
        for (int i = 0; i < k; i++) {
            if (mesmo(*campos[i], nome)) {
                *campos[i] = (char *)valor;
                estat.propagacoes++;
            }
        }
    }
}

/* ================= literais ================= */

typedef enum { LIT_NENHUM, LIT_INT, LIT_FLOAT } TipoLiteral;

typedef struct {
    TipoLiteral tipo;
    long long i;
    double f;
} Literal;

/*
 * Reconhece constantes inteiras, de ponto flutuante e de char simples ('a').
 * Um char vale o seu codigo, como em C (o Python gerado usa ord()).
 */
static Literal le_literal(const char *s) {
    Literal lit = { LIT_NENHUM, 0, 0.0 };
    if (!s || !s[0]) {
        return lit;
    }
    if (s[0] == '\'') {
        if (strlen(s) == 3 && s[1] != '\\' && s[2] == '\'') {
            lit.tipo = LIT_INT;
            lit.i = (unsigned char)s[1];
        }
        return lit;
    }
    int ini = (s[0] == '-') ? 1 : 0;
    if (s[ini] < '0' || s[ini] > '9') {
        return lit;
    }
    char *fim;
    if (!strpbrk(s, ".eE")) {
        long long v = strtoll(s, &fim, 10);
        if (*fim == '\0') {
            lit.tipo = LIT_INT;
            lit.i = v;
        }
        return lit;
    }
    double v = strtod(s, &fim);
    if (*fim == '\0' && isfinite(v)) {
        lit.tipo = LIT_FLOAT;
        lit.f = v;
    }
    return lit;
}

static int eh_literal(const char *s) {
    return le_literal(s).tipo != LIT_NENHUM;
}

static double como_double(Literal l) {
    return l.tipo == LIT_INT ? (double)l.i : l.f;
}

static int cabe_em_int(long long v) {
    return v >= INT_MIN && v <= INT_MAX;
}

static const char *int_texto(long long v) {
    return cabe_em_int(v) ? ir_formata_int(v) : NULL;
}

static const char *float_texto(double v) {
    return isfinite(v) ? ir_formata_float(v) : NULL;
}

/* Calcula "a op b" para constantes; NULL quando nao deve ser dobrado. */
static const char *dobra_binario(IrOp op, Literal a, Literal b) {
    if (a.tipo == LIT_INT && b.tipo == LIT_INT && op != IR_DIV) {
        long long x = a.i, y = b.i;
        switch (op) {
            case IR_SOMA:         return int_texto(x + y);
            case IR_SUB:          return int_texto(x - y);
            case IR_MUL:          return int_texto(x * y);
            case IR_DIV_INT:      return y == 0 ? NULL : int_texto(x / y); /* trunca para zero, como C */
            case IR_MENOR:        return ir_formata_int(x < y);
            case IR_MAIOR:        return ir_formata_int(x > y);
            case IR_MENOR_IGUAL:  return ir_formata_int(x <= y);
            case IR_MAIOR_IGUAL:  return ir_formata_int(x >= y);
            case IR_IGUAL:        return ir_formata_int(x == y);
            case IR_DIFERENTE:    return ir_formata_int(x != y);
            default:              return NULL;
        }
    }

    if (op == IR_DIV_INT) {
        return NULL; /* so ocorre entre inteiros/char */
    }
    double x = como_double(a), y = como_double(b);
    switch (op) {
        case IR_SOMA:         return float_texto(x + y);
        case IR_SUB:          return float_texto(x - y);
        case IR_MUL:          return float_texto(x * y);
        case IR_DIV:          return y == 0.0 ? NULL : float_texto(x / y);
        case IR_MENOR:        return ir_formata_int(x < y);
        case IR_MAIOR:        return ir_formata_int(x > y);
        case IR_MENOR_IGUAL:  return ir_formata_int(x <= y);
        case IR_MAIOR_IGUAL:  return ir_formata_int(x >= y);
        case IR_IGUAL:        return ir_formata_int(x == y);
        case IR_DIFERENTE:    return ir_formata_int(x != y);
        default:              return NULL;
    }
}

static const char *dobra_unario(IrOp op, Literal a) {
    if (op == IR_NAO) {
        return ir_formata_int(como_double(a) == 0.0);
    }
    return a.tipo == LIT_INT ? int_texto(-a.i) : float_texto(-a.f);
}

static void vira_copia(Quadrupla *q, const char *valor) {
    q->op = IR_ATRIB;
    q->arg1 = (char *)valor;
    q->arg2 = NULL;
}

/* ================= passes ================= */

/* 1. Dobra de constantes. */
static int passe_dobra(Quadrupla *func) {
    int mudou = 0;
    PARA_CADA(q, func) {
        const char *valor = NULL;
        if (eh_binario(q->op)) {
            Literal a = le_literal(q->arg1), b = le_literal(q->arg2);
            if (a.tipo != LIT_NENHUM && b.tipo != LIT_NENHUM) {
                valor = dobra_binario(q->op, a, b);
            }
        } else if (eh_unario(q->op)) {
            Literal a = le_literal(q->arg1);
            if (a.tipo != LIT_NENHUM) {
                valor = dobra_unario(q->op, a);
            }
        }
        if (valor) {
            vira_copia(q, valor);
            estat.dobras_constantes++;
            mudou = 1;
        }
    }
    return mudou;
}

/*
 * Operando cujo valor e garantidamente numerico no Python gerado: constante
 * numerica ou temporario que so recebe resultados de contas. Variaveis e
 * retornos de chamada podem ser char, e "c + 0" vale ord(c), nao c.
 */
static int numerico_garantido(Quadrupla *func, const char *x) {
    Literal l = le_literal(x);
    if (l.tipo != LIT_NENHUM) {
        return x[0] != '\'';
    }
    if (!ir_eh_temporario(x)) {
        return 0;
    }
    int defs = 0;
    PARA_CADA(q, func) {
        if (define_valor(q) && mesmo(q->resultado, x)) {
            if (!eh_binario(q->op) && !eh_unario(q->op)) {
                return 0;
            }
            defs++;
        }
    }
    return defs > 0;
}

/*
 * 2. Simplificacao algebrica: x + 0, 0 + x, x - 0, x * 1, 1 * x, x / 1.
 * Os neutros precisam ser os inteiros "0" e "1": com 0.0 o resultado
 * passaria a float e mudaria a forma como o valor e impresso.
 * x * 0 nao e simplificado, pois daria 0 mesmo quando x e float (0.0).
 */
static int passe_simplifica(Quadrupla *func) {
    int mudou = 0;
    PARA_CADA(q, func) {
        if (!eh_binario(q->op) || eh_relacional(q->op)) {
            continue;
        }
        const char *resto = NULL;
        if (q->op == IR_SOMA && mesmo(q->arg2, "0"))        resto = q->arg1;
        else if (q->op == IR_SOMA && mesmo(q->arg1, "0"))   resto = q->arg2;
        else if (q->op == IR_SUB && mesmo(q->arg2, "0"))    resto = q->arg1;
        else if (q->op == IR_MUL && mesmo(q->arg2, "1"))    resto = q->arg1;
        else if (q->op == IR_MUL && mesmo(q->arg1, "1"))    resto = q->arg2;
        else if ((q->op == IR_DIV_INT || q->op == IR_DIV) && mesmo(q->arg2, "1")) resto = q->arg1;

        if (resto && numerico_garantido(func, resto)) {
            vira_copia(q, resto);
            estat.simplificacoes++;
            mudou = 1;
        }
    }
    return mudou;
}

/* 3. Propagacao de constantes: temporario com uma unica definicao "t = c". */
static int passe_propaga(Quadrupla *func) {
    int mudou = 0;
    PARA_CADA(q, func) {
        if (q->op != IR_ATRIB || !ir_eh_temporario(q->resultado) || !eh_literal(q->arg1)) {
            continue;
        }
        if (conta_defs(func, q->resultado) != 1 || conta_usos(func, q->resultado) == 0) {
            continue;
        }
        substitui_usos(func, q->resultado, q->arg1);
        mudou = 1;
    }
    return mudou;
}

/*
 * 4. Eliminacao de copias: "t = <expr>" seguido de "x = t", com t usado so
 * ali e x uma variavel do programa, vira "x = <expr>". Quando x e um
 * temporario (curto-circuito de && e ||) a copia e mantida.
 */
static int passe_copias(Quadrupla *func) {
    int mudou = 0;
    PARA_CADA(q, func) {
        Quadrupla *prox = q->proximo;
        while (prox && define_valor(q) && ir_eh_temporario(q->resultado) &&
               prox->op == IR_ATRIB && mesmo(prox->arg1, q->resultado) &&
               prox->resultado && !ir_eh_temporario(prox->resultado) &&
               conta_defs(func, q->resultado) == 1 &&
               conta_usos(func, q->resultado) == 1) {
            q->resultado = prox->resultado;
            ir_remove_apos(q);
            estat.copias_eliminadas++;
            mudou = 1;
            prox = q->proximo;
        }
    }
    return mudou;
}

/* 5. Temporarios mortos: definicoes cujo valor nunca e lido. */
static int passe_mortos(Quadrupla *func) {
    int mudou = 0;
    Quadrupla *ant = func;
    while (ant->proximo && ant->proximo->op != IR_FIM_FUNC) {
        Quadrupla *q = ant->proximo;
        if (define_valor(q) && ir_eh_temporario(q->resultado) &&
            conta_usos(func, q->resultado) == 0) {
            estat.temporarios_mortos++;
            mudou = 1;
            if (q->op == IR_CALL) {
                q->resultado = NULL; /* a chamada pode ter efeitos */
            } else {
                ir_remove_apos(ant);
                continue;
            }
        }
        ant = q;
    }
    return mudou;
}

static int eh_controle(IrOp op) {
    return op == IR_LABEL || op == IR_GOTO || op == IR_IF || op == IR_IF_FALSE ||
           op == IR_FUNC || op == IR_FIM_FUNC;
}

/*
 * 6. Codigo inalcancavel: depois de um return, as instrucoes ate o proximo
 * label, goto ou fim de funcao nunca executam. So removemos o trecho quando
 * ele nao tem saltos nem declaracoes, para nao desmontar os padroes de
 * if/while que o gerador de Python reconhece.
 */
static int passe_inalcancavel(Quadrupla *func) {
    int mudou = 0;
    PARA_CADA(q, func) {
        if (q->op != IR_RETURN) {
            continue;
        }
        int removivel = 0;
        Quadrupla *p = q->proximo;
        for (; p; p = p->proximo) {
            if (p->op == IR_LABEL || p->op == IR_GOTO || p->op == IR_FIM_FUNC) {
                break;
            }
            if (eh_controle(p->op) || p->op == IR_DECL) {
                removivel = 0;
                p = NULL;
                break;
            }
            removivel = 1;
        }
        if (!p || !removivel) {
            continue;
        }
        while (q->proximo != p) {
            ir_remove_apos(q);
            estat.inalcancaveis++;
        }
        mudou = 1;
    }
    return mudou;
}

void otimizador_otimiza_funcao(Quadrupla *func) {
    if (!func || func->op != IR_FUNC) {
        return;
    }
    for (int rodada = 0; rodada < 50; rodada++) {
        int mudou = 0;
        mudou |= passe_dobra(func);
        mudou |= passe_simplifica(func);
        mudou |= passe_propaga(func);
        mudou |= passe_copias(func);
        mudou |= passe_mortos(func);
        mudou |= passe_inalcancavel(func);
        if (!mudou) {
            break;
        }
    }
}
