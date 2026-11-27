#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "ir.h"

typedef struct {
    FILE *saida;
    const Quadrupla **inst;
    int total;
    int usa_char;
    int usa_div_int;
    int tem_main;
    const char *tipo_retorno_atual;

    const char **params;
    int num_params;
    int cap_params;

    const char **vars_usuario;
    int num_vars;
    int cap_vars;
} CodegenCtx;

static void ctx_libera(CodegenCtx *ctx) {
    free(ctx->inst);
    free(ctx->params);
    free(ctx->vars_usuario);
    memset(ctx, 0, sizeof(*ctx));
}

static int empilha_str(const char ***lista, int *num, int *cap, const char *s) {
    if (*num == *cap) {
        int nova_cap = *cap ? (*cap * 2) : 16;
        const char **novo = realloc(*lista, (size_t)nova_cap * sizeof(*novo));
        if (!novo) return -1;
        *lista = novo;
        *cap = nova_cap;
    }
    (*lista)[(*num)++] = s;
    return 0;
}

static int eh_var_usuario(const CodegenCtx *ctx, const char *nome) {
    if (!nome) return 0;
    for (int i = 0; i < ctx->num_vars; i++) {
        if (strcmp(ctx->vars_usuario[i], nome) == 0) return 1;
    }
    return 0;
}

static int eh_nome_temporario(const CodegenCtx *ctx, const char *nome) {
    if (!nome || nome[0] != 't' || !isdigit((unsigned char)nome[1])) return 0;
    for (const char *p = nome + 1; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return !eh_var_usuario(ctx, nome);
}

static int eh_literal_numerico(const char *s) {
    if (!s || !s[0]) return 0;
    int i = (s[0] == '-') ? 1 : 0;
    if (!isdigit((unsigned char)s[i])) return 0;
    int tem_ponto = 0;
    for (; s[i]; i++) {
        if (s[i] == '.') {
            if (tem_ponto) return 0;
            tem_ponto = 1;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

static int eh_literal_char(const char *s) {
    return s && s[0] == '\'';
}

static const char *operador_python(IrOp op) {
    switch (op) {
        case IR_SOMA:        return "+";
        case IR_SUB:         return "-";
        case IR_MUL:         return "*";
        case IR_DIV:         return "/";
        case IR_MENOR:       return "<";
        case IR_MAIOR:       return ">";
        case IR_MENOR_IGUAL: return "<=";
        case IR_MAIOR_IGUAL: return ">=";
        case IR_IGUAL:       return "==";
        case IR_DIFERENTE:   return "!=";
        default:             return NULL;
    }
}

static int eh_relacional(IrOp op) {
    return op == IR_MENOR || op == IR_MAIOR ||
           op == IR_MENOR_IGUAL || op == IR_MAIOR_IGUAL ||
           op == IR_IGUAL || op == IR_DIFERENTE;
}

static void indenta(FILE *saida, int nivel) {
    for (int i = 0; i < nivel; i++) {
        fputs("    ", saida);
    }
}

static void imprime_numerico(const CodegenCtx *ctx, const char *op) {
    if (ctx->usa_char && !eh_literal_numerico(op)) {
        fprintf(ctx->saida, "_num(%s)", op);
    } else {
        fputs(op, ctx->saida);
    }
}

static void imprime_normalizacao_bool(const CodegenCtx *ctx, const char *var, int nivel) {
    indenta(ctx->saida, nivel);
    if (ctx->usa_char) {
        fprintf(ctx->saida, "%s = _bool(%s)\n", var, var);
    } else {
        fprintf(ctx->saida, "%s = int(bool(%s))\n", var, var);
    }
}

static int busca_label(const CodegenCtx *ctx, const char *label, int inicio, int fim) {
    if (!label) return -1;
    for (int i = inicio; i < fim; i++) {
        if (ctx->inst[i]->op == IR_LABEL &&
            ctx->inst[i]->resultado &&
            strcmp(ctx->inst[i]->resultado, label) == 0) {
            return i;
        }
    }
    return -1;
}

static int busca_goto_retorno(const CodegenCtx *ctx, const char *label, int inicio, int fim) {
    if (!label) return -1;
    for (int i = fim - 1; i >= inicio; i--) {
        if (ctx->inst[i]->op == IR_GOTO &&
            ctx->inst[i]->resultado &&
            strcmp(ctx->inst[i]->resultado, label) == 0) {
            return i;
        }
    }
    return -1;
}

static int analisa_programa(CodegenCtx *ctx) {
    int n = ir_num_instrucoes();
    if (n <= 0) return 0;

    ctx->inst = malloc((size_t)n * sizeof(*ctx->inst));
    if (!ctx->inst) return -1;

    int idx = 0;
    for (const Quadrupla *q = ir_primeira(); q && idx < n; q = q->proximo) {
        ctx->inst[idx++] = q;

        if (q->op == IR_DIV_INT) {
            ctx->usa_div_int = 1;
        }
        if ((q->op == IR_FUNC || q->op == IR_FPARAM || q->op == IR_DECL) &&
            q->arg1 && strcmp(q->arg1, "char") == 0) {
            ctx->usa_char = 1;
        }
        if (eh_literal_char(q->arg1) || eh_literal_char(q->arg2)) {
            ctx->usa_char = 1;
        }
    }
    ctx->total = idx;

    for (int i = 0; i < ctx->total; i++) {
        const Quadrupla *q = ctx->inst[i];
        if (q->op == IR_FUNC && q->resultado && strcmp(q->resultado, "main") == 0) {
            if (!(i + 1 < ctx->total && ctx->inst[i + 1]->op == IR_FPARAM)) {
                ctx->tem_main = 1;
            }
        }
    }
    return 0;
}

static void emite_helpers(const CodegenCtx *ctx) {
    if (ctx->usa_char) {
        fputs("def _num(v):\n", ctx->saida);
        fputs("    return ord(v) if isinstance(v, str) and len(v) == 1 else v\n\n", ctx->saida);
        fputs("def _bool(v):\n", ctx->saida);
        fputs("    return 1 if _num(v) else 0\n\n", ctx->saida);
    }

    if (ctx->usa_div_int) {
        fputs("def _div_int(a, b):\n", ctx->saida);
        if (ctx->usa_char) {
            fputs("    a = _num(a)\n", ctx->saida);
            fputs("    b = _num(b)\n", ctx->saida);
        }
        fputs("    q = abs(a) // abs(b)\n", ctx->saida);
        fputs("    return -q if (a < 0) ^ (b < 0) else q\n\n", ctx->saida);
    }
}

static int emite_bloco(CodegenCtx *ctx, int inicio, int fim, int nivel, const char *label_break);

static int emite_instrucao_simples(CodegenCtx *ctx, const Quadrupla *q, int nivel) {
    switch (q->op) {
        case IR_DECL:
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = None\n", q->resultado);
            return 1;

        case IR_ATRIB:
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = %s\n", q->resultado, q->arg1);
            return 1;

        case IR_SOMA:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_MENOR:
        case IR_MAIOR:
        case IR_MENOR_IGUAL:
        case IR_MAIOR_IGUAL:
        case IR_IGUAL:
        case IR_DIFERENTE: {
            int rel = eh_relacional(q->op);
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = %s", q->resultado, rel ? "int(" : "");
            imprime_numerico(ctx, q->arg1);
            fprintf(ctx->saida, " %s ", operador_python(q->op));
            imprime_numerico(ctx, q->arg2);
            fputs(rel ? ")\n" : "\n", ctx->saida);
            return 1;
        }

        case IR_DIV_INT:
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = _div_int(%s, %s)\n", q->resultado, q->arg1, q->arg2);
            return 1;

        case IR_NEGATIVO:
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = -", q->resultado);
            imprime_numerico(ctx, q->arg1);
            fputc('\n', ctx->saida);
            return 1;

        case IR_NAO:
            indenta(ctx->saida, nivel);
            fprintf(ctx->saida, "%s = int(not ", q->resultado);
            imprime_numerico(ctx, q->arg1);
            fputs(")\n", ctx->saida);
            return 1;

        case IR_PARAM:
            return empilha_str(&ctx->params, &ctx->num_params, &ctx->cap_params, q->arg1) == 0
                       ? 0
                       : -1;

        case IR_CALL: {
            int n = q->arg2 ? atoi(q->arg2) : 0;
            if (n < 0 || n > ctx->num_params) n = ctx->num_params;
            int base = ctx->num_params - n;
            const char *nome_func = (strcmp(q->arg1, "printf") == 0) ? "print" : q->arg1;

            indenta(ctx->saida, nivel);
            if (q->resultado) {
                fprintf(ctx->saida, "%s = %s(", q->resultado, nome_func);
            } else {
                fprintf(ctx->saida, "%s(", nome_func);
            }
            for (int k = 0; k < n; k++) {
                if (k > 0) fputs(", ", ctx->saida);
                fputs(ctx->params[base + k], ctx->saida);
            }
            fputs(")\n", ctx->saida);
            ctx->num_params = base;
            return 1;
        }

        case IR_RETURN:
            indenta(ctx->saida, nivel);
            if (q->arg1) {
                int conv_num = ctx->usa_char &&
                               ctx->tipo_retorno_atual &&
                               (strcmp(ctx->tipo_retorno_atual, "int") == 0 ||
                                strcmp(ctx->tipo_retorno_atual, "float") == 0);
                fputs("return ", ctx->saida);
                if (conv_num) {
                    imprime_numerico(ctx, q->arg1);
                } else {
                    fputs(q->arg1, ctx->saida);
                }
                fputc('\n', ctx->saida);
            } else {
                fputs("return\n", ctx->saida);
            }
            return 1;

        default:
            return 0;
    }
}

static int emite_bloco(CodegenCtx *ctx, int inicio, int fim, int nivel, const char *label_break) {
    int emitidos = 0;
    int i = inicio;

    while (i < fim) {
        const Quadrupla *q = ctx->inst[i];

        /* Laco (while ou for): L_inicio: ... goto L_inicio; L_fim: */
        if (q->op == IR_LABEL && q->resultado) {
            int pos_goto = busca_goto_retorno(ctx, q->resultado, i + 1, fim);
            if (pos_goto != -1) {
                const char *label_fim = NULL;
                if (pos_goto + 1 < fim && ctx->inst[pos_goto + 1]->op == IR_LABEL) {
                    label_fim = ctx->inst[pos_goto + 1]->resultado;
                }

                indenta(ctx->saida, nivel);
                fputs("while True:\n", ctx->saida);

                int corpo = emite_bloco(ctx, i + 1, pos_goto, nivel + 1, label_fim);
                if (corpo < 0) return -1;
                if (corpo == 0) {
                    indenta(ctx->saida, nivel + 1);
                    fputs("pass\n", ctx->saida);
                }
                emitidos++;
                i = label_fim ? (pos_goto + 2) : (pos_goto + 1);
                continue;
            }
            i++;
            continue;
        }

        /* Condicao de parada do laco atual: ifFalse c goto L_fim -> if not c: break */
        if (q->op == IR_IF_FALSE && label_break && q->resultado &&
            strcmp(q->resultado, label_break) == 0) {
            indenta(ctx->saida, nivel);
            fputs("if not ", ctx->saida);
            imprime_numerico(ctx, q->arg1);
            fputs(":\n", ctx->saida);
            indenta(ctx->saida, nivel + 1);
            fputs("break\n", ctx->saida);
            emitidos++;
            i++;
            continue;
        }

        /* Salto condicional para frente: if, if/else ou curto-circuito de && */
        if (q->op == IR_IF_FALSE && q->resultado) {
            int pos_alvo = busca_label(ctx, q->resultado, i + 1, fim);
            if (pos_alvo != -1) {
                int tem_else = 0;
                int pos_fim_else = -1;
                if (pos_alvo > i + 1 && ctx->inst[pos_alvo - 1]->op == IR_GOTO) {
                    pos_fim_else = busca_label(
                        ctx, ctx->inst[pos_alvo - 1]->resultado, pos_alvo + 1, fim);
                    if (pos_fim_else != -1) tem_else = 1;
                }

                int eh_curto_e = !tem_else &&
                                 eh_nome_temporario(ctx, q->arg1) &&
                                 pos_alvo > i + 1 &&
                                 ctx->inst[pos_alvo - 1]->op == IR_ATRIB &&
                                 ctx->inst[pos_alvo - 1]->resultado &&
                                 strcmp(ctx->inst[pos_alvo - 1]->resultado, q->arg1) == 0;

                indenta(ctx->saida, nivel);
                fputs("if ", ctx->saida);
                imprime_numerico(ctx, q->arg1);
                fputs(":\n", ctx->saida);

                int fim_then = tem_else ? (pos_alvo - 1) : pos_alvo;
                int n_then = emite_bloco(ctx, i + 1, fim_then, nivel + 1, NULL);
                if (n_then < 0) return -1;
                if (n_then == 0) {
                    indenta(ctx->saida, nivel + 1);
                    fputs("pass\n", ctx->saida);
                }
                emitidos++;

                if (tem_else) {
                    indenta(ctx->saida, nivel);
                    fputs("else:\n", ctx->saida);

                    int n_else = emite_bloco(ctx, pos_alvo + 1, pos_fim_else, nivel + 1, NULL);
                    if (n_else < 0) return -1;
                    if (n_else == 0) {
                        indenta(ctx->saida, nivel + 1);
                        fputs("pass\n", ctx->saida);
                    }
                    i = pos_fim_else + 1;
                } else {
                    if (eh_curto_e) {
                        imprime_normalizacao_bool(ctx, q->arg1, nivel);
                        emitidos++;
                    }
                    i = pos_alvo + 1;
                }
                continue;
            }
        }

        /* Curto-circuito de ||: if r goto L -> if not r: ... */
        if (q->op == IR_IF && q->resultado) {
            int pos_alvo = busca_label(ctx, q->resultado, i + 1, fim);
            if (pos_alvo != -1) {
                indenta(ctx->saida, nivel);
                fputs("if not ", ctx->saida);
                imprime_numerico(ctx, q->arg1);
                fputs(":\n", ctx->saida);

                int n_corpo = emite_bloco(ctx, i + 1, pos_alvo, nivel + 1, NULL);
                if (n_corpo < 0) return -1;
                if (n_corpo == 0) {
                    indenta(ctx->saida, nivel + 1);
                    fputs("pass\n", ctx->saida);
                }
                emitidos++;

                if (eh_nome_temporario(ctx, q->arg1)) {
                    imprime_normalizacao_bool(ctx, q->arg1, nivel);
                    emitidos++;
                }
                i = pos_alvo + 1;
                continue;
            }
        }

        int res = emite_instrucao_simples(ctx, q, nivel);
        if (res < 0) return -1;
        emitidos += res;
        i++;
    }

    return emitidos;
}

static int emite_funcao(CodegenCtx *ctx, int idx_func) {
    const Quadrupla *func = ctx->inst[idx_func];
    ctx->num_vars = 0;
    ctx->num_params = 0;
    ctx->tipo_retorno_atual = func->arg1;

    fprintf(ctx->saida, "def %s(", func->resultado);
    int i = idx_func + 1;
    int primeiro = 1;
    while (i < ctx->total && ctx->inst[i]->op == IR_FPARAM) {
        if (!primeiro) fputs(", ", ctx->saida);
        fputs(ctx->inst[i]->resultado, ctx->saida);
        if (empilha_str(&ctx->vars_usuario, &ctx->num_vars, &ctx->cap_vars,
                        ctx->inst[i]->resultado) != 0) {
            return -1;
        }
        primeiro = 0;
        i++;
    }
    fputs("):\n", ctx->saida);

    int corpo_inicio = i;
    int fim_func = corpo_inicio;
    while (fim_func < ctx->total && ctx->inst[fim_func]->op != IR_FIM_FUNC) {
        if (ctx->inst[fim_func]->op == IR_DECL &&
            empilha_str(&ctx->vars_usuario, &ctx->num_vars, &ctx->cap_vars,
                        ctx->inst[fim_func]->resultado) != 0) {
            return -1;
        }
        fim_func++;
    }

    int emitidos = emite_bloco(ctx, corpo_inicio, fim_func, 1, NULL);
    ctx->tipo_retorno_atual = NULL;
    if (emitidos < 0) return -1;
    if (emitidos == 0) {
        fputs("    pass\n", ctx->saida);
    }
    return (fim_func < ctx->total) ? (fim_func + 1) : fim_func;
}

int codegen_gera_python(FILE *saida) {
    if (!saida) return 1;

    CodegenCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.saida = saida;

    if (analisa_programa(&ctx) != 0) {
        ctx_libera(&ctx);
        return 1;
    }

    emite_helpers(&ctx);

    int i = 0;
    int primeira_funcao = 1;
    while (i < ctx.total) {
        if (ctx.inst[i]->op == IR_FUNC) {
            if (!primeira_funcao) fputc('\n', saida);
            int proximo = emite_funcao(&ctx, i);
            if (proximo < 0) {
                ctx_libera(&ctx);
                return 1;
            }
            primeira_funcao = 0;
            i = proximo;
        } else {
            int fim_solto = i + 1;
            while (fim_solto < ctx.total && ctx.inst[fim_solto]->op != IR_FUNC) {
                fim_solto++;
            }
            if (emite_bloco(&ctx, i, fim_solto, 0, NULL) < 0) {
                ctx_libera(&ctx);
                return 1;
            }
            i = fim_solto;
        }
    }

    if (ctx.tem_main) {
        fputs("\nif __name__ == \"__main__\":\n", saida);
        fputs("    main()\n", saida);
    }

    ctx_libera(&ctx);
    return 0;
}