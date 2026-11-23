/*
 * teste_ir.c - testes do gerador de codigo intermediario (src/ir.c).
 *
 * Cada caso analisa um programa com o parser real, gera a RI dos comandos
 * do corpo da primeira funcao e compara o texto obtido com o esperado
 * (exemplos da secao 7 de docs/representacao_intermediaria.md).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/ast.h"
#include "../../src/tabela_simbolos.h"
#include "../../src/ir.h"
#include "../../src/parser.tab.h"

extern int erros_lexicos;
extern AstNo *raiz_ast;
extern int construcoes_sem_ast;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

static int falhas = 0;
static int casos = 0;

#define VERIFICA(cond) do { if (!(cond)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); falhas++; } } while (0)

static char *texto_da_ri(void) {
    FILE *f = tmpfile();
    if (!f) {
        perror("tmpfile");
        exit(1);
    }
    ir_imprime(f);
    long n = ftell(f);
    rewind(f);
    char *buf = calloc((size_t)n + 1, 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        perror("fread");
        exit(1);
    }
    fclose(f);
    return buf;
}

/* Gera a RI dos comandos do corpo da primeira funcao de "codigo". */
static void confere(const char *nome, const char *codigo, const char *esperado) {
    casos++;
    erros_lexicos = 0;
    construcoes_sem_ast = 0;
    raiz_ast = NULL;
    YY_BUFFER_STATE buf = yy_scan_string(codigo);
    int rc = yyparse();
    yy_delete_buffer(buf);
    if (rc != 0 || erros_lexicos || !raiz_ast) {
        fprintf(stderr, "FALHA [%s]: programa nao foi aceito pelo parser\n", nome);
        falhas++;
        return;
    }

    const AstNo *funcao = raiz_ast->filho[0];
    ir_inicia_funcao(funcao);
    for (const AstNo *c = funcao->filho[1]->filho[0]; c; c = c->proximo) {
        ir_gera_comando(c);
    }

    char *obtido = texto_da_ri();
    if (strcmp(obtido, esperado) != 0) {
        fprintf(stderr, "FALHA [%s]\n--- esperado ---\n%s--- obtido ---\n%s----------------\n",
                nome, esperado, obtido);
        falhas++;
    }
    free(obtido);

    ir_libera();
    VERIFICA(ir_num_instrucoes() == 0 && ir_primeira() == NULL);
    ast_libera(raiz_ast);
    raiz_ast = NULL;
    ts_libera();
}

static void testa_expressoes(void) {
    confere("aritmetica e precedencia",
            "void f(int a, int b, int c, int d, int e) { int x; x = a + b * c - d / e; }",
            "decl int x\n"
            "t1 = b * c\n"
            "t2 = a + t1\n"
            "t3 = d // e\n"
            "t4 = t2 - t3\n"
            "x = t4\n");

    confere("float com inicializador",
            "void f(float x) { float f = 2.5 * x + 1; }",
            "t1 = 2.5 * x\n"
            "t2 = t1 + 1\n"
            "f = t2\n");

    confere("divisao real quando ha float",
            "void f(float g, int a) { float h = g / a; int k = a / 2; }",
            "t1 = g / a\n"
            "h = t1\n"
            "t2 = a // 2\n"
            "k = t2\n");

    confere("unario",
            "void f(int a) { int y; y = -a * 2; }",
            "decl int y\n"
            "t1 = -a\n"
            "t2 = t1 * 2\n"
            "y = t2\n");
}

static void testa_logicos(void) {
    confere("&& com negacao",
            "void f(int a, int b, int c, int d) { int r; r = (a < b) && !(c == d); }",
            "decl int r\n"
            "t1 = a < b\n"
            "ifFalse t1 goto L1\n"
            "t2 = c == d\n"
            "t3 = !t2\n"
            "t1 = t3\n"
            "L1:\n"
            "r = t1\n");

    confere("||",
            "void f(int a, int b) { int r; r = a > 0 || b > 0; }",
            "decl int r\n"
            "t1 = a > 0\n"
            "if t1 goto L1\n"
            "t2 = b > 0\n"
            "t1 = t2\n"
            "L1:\n"
            "r = t1\n");

    confere("&& com identificadores",
            "void f(int a, int b) { int r = a && b; }",
            "t1 = a\n"
            "ifFalse t1 goto L1\n"
            "t1 = b\n"
            "L1:\n"
            "r = t1\n");

    confere("&& encadeado (label em pre-ordem)",
            "void f(int a, int b, int c) { int r = a && b && c; }",
            "t1 = a\n"
            "ifFalse t1 goto L2\n"
            "t1 = b\n"
            "L2:\n"
            "ifFalse t1 goto L1\n"
            "t1 = c\n"
            "L1:\n"
            "r = t1\n");
}

static void testa_declaracoes_e_atribuicoes(void) {
    confere("declaracao sem inicializador e copia direta",
            "void saudacao() { int contador; contador = 3; }",
            "decl int contador\n"
            "contador = 3\n");

    confere("literais",
            "void f() { char c = 'a'; float g = 1.0; float h = 3.14; int i = 42; }",
            "c = 'a'\n"
            "g = 1.0\n"
            "h = 3.14\n"
            "i = 42\n");

    confere("temporario pula nome de variavel",
            "void f(int t2) { int t1 = 0; int x; x = t1 + t2 * 3; }",
            "t1 = 0\n"
            "decl int x\n"
            "t3 = t2 * 3\n"
            "t4 = t1 + t3\n"
            "x = t4\n");
}

static void testa_if_else(void) {
    confere("if / else",
            "void f(int a, int b) { int m; if (a > b) { m = a; } else { m = b; } }",
            "decl int m\n"
            "t1 = a > b\n"
            "ifFalse t1 goto L1\n"
            "m = a\n"
            "goto L2\n"
            "L1:\n"
            "m = b\n"
            "L2:\n");

    confere("if sem else",
            "void f(int x) { int y; if (x == 0) { y = 1; } }",
            "decl int y\n"
            "t1 = x == 0\n"
            "ifFalse t1 goto L1\n"
            "y = 1\n"
            "L1:\n");

    confere("if com && (label do if antes do label do &&)",
            "void f(int x) { int y; if (x > 0 && x < 10) { y = 1; } }",
            "decl int y\n"
            "t1 = x > 0\n"
            "ifFalse t1 goto L2\n"
            "t2 = x < 10\n"
            "t1 = t2\n"
            "L2:\n"
            "ifFalse t1 goto L1\n"
            "y = 1\n"
            "L1:\n");

    confere("else if",
            "void f(int x) { int s; if (x < 0) { s = 1; } else { if (x == 0) { s = 2; } else { s = 3; } } }",
            "decl int s\n"
            "t1 = x < 0\n"
            "ifFalse t1 goto L1\n"
            "s = 1\n"
            "goto L2\n"
            "L1:\n"
            "t2 = x == 0\n"
            "ifFalse t2 goto L3\n"
            "s = 2\n"
            "goto L4\n"
            "L3:\n"
            "s = 3\n"
            "L4:\n"
            "L2:\n");
}

static void testa_repeticoes(void) {
    confere("while",
            "void f(int i, int n, int s) { while (i < n) { s = s + i; i = i + 1; } }",
            "L1:\n"
            "t1 = i < n\n"
            "ifFalse t1 goto L2\n"
            "t2 = s + i\n"
            "s = t2\n"
            "t3 = i + 1\n"
            "i = t3\n"
            "goto L1\n"
            "L2:\n");

    confere("for",
            "void f(int n, int s) { for (int i = 0; i < n; i = i + 1) { s = s + i; } }",
            "i = 0\n"
            "L1:\n"
            "t1 = i < n\n"
            "ifFalse t1 goto L2\n"
            "t2 = s + i\n"
            "s = t2\n"
            "t3 = i + 1\n"
            "i = t3\n"
            "goto L1\n"
            "L2:\n");
}

int main(void) {
    testa_expressoes();
    testa_logicos();
    testa_declaracoes_e_atribuicoes();
    testa_if_else();
    testa_repeticoes();

    if (falhas) {
        printf("Testes do gerador de RI: %d falha(s) em %d caso(s)\n", falhas, casos);
        return 1;
    }
    printf("Testes do gerador de RI: %d caso(s), todos passaram\n", casos);
    return 0;
}
