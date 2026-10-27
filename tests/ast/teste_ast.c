/*
 * teste_ast.c - testes do modulo da AST (src/ast.c), montando arvores
 * diretamente, sem passar pelo parser.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/ast.h"

static int falhas = 0;

#define VERIFICA(cond) do { if (!(cond)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); falhas++; } } while (0)

/* arvore de 2 + 3 */
static void testa_soma_2_mais_3(void) {
    AstNo *e = ast_binario(OP_SOMA, ast_num_int(2, 1), ast_num_int(3, 1), 1);
    VERIFICA(e->tipo == AST_BINARIO && e->op == OP_SOMA && e->linha == 1);
    VERIFICA(e->filho[0]->tipo == AST_NUM_INT && e->filho[0]->valor_int == 2);
    VERIFICA(e->filho[1]->tipo == AST_NUM_INT && e->filho[1]->valor_int == 3);
    VERIFICA(ast_nos_alocados() == 3);
    ast_libera(e);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_copia_de_texto(void) {
    char buf[16] = "total";
    AstNo *id = ast_id(buf, 4);
    strcpy(buf, "xxxxx");
    VERIFICA(strcmp(id->texto, "total") == 0 && id->texto != buf && id->linha == 4);
    ast_libera(id);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_literais_e_unario(void) {
    AstNo *f = ast_num_float(3.14, 2);
    AstNo *c = ast_char_lit("x", 2);
    AstNo *s = ast_string_lit("Hello World", 2);
    AstNo *u = ast_unario(OP_NAO, ast_id("a", 2), 2);
    VERIFICA(f->tipo == AST_NUM_FLOAT && f->valor_float == 3.14);
    VERIFICA(c->tipo == AST_CHAR_LIT && strcmp(c->texto, "x") == 0);
    VERIFICA(s->tipo == AST_STRING_LIT && strcmp(s->texto, "Hello World") == 0);
    VERIFICA(u->tipo == AST_UNARIO && u->op == OP_NAO);
    VERIFICA(u->filho[0]->tipo == AST_ID && strcmp(u->filho[0]->texto, "a") == 0);

    AstNo *args = ast_num_int(1, 2);
    args->proximo = ast_id("y", 2);
    AstNo *ch = ast_chamada("f", args, 2);
    VERIFICA(ch->tipo == AST_CHAMADA && strcmp(ch->texto, "f") == 0);
    VERIFICA(ch->filho[0]->valor_int == 1 && strcmp(ch->filho[0]->proximo->texto, "y") == 0);

    ast_libera(f);
    ast_libera(c);
    ast_libera(s);
    ast_libera(u);
    ast_libera(ch);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_libera_nulo(void) {
    ast_libera(NULL);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_nomes_auxiliares(void) {
    VERIFICA(strcmp(ast_nome_tipo_dado(TIPO_FLOAT), "float") == 0);
    VERIFICA(strcmp(ast_simbolo_operador(OP_MENOR_IGUAL), "<=") == 0);
    VERIFICA(strcmp(ast_simbolo_operador(OP_E), "&&") == 0);
    VERIFICA(strcmp(ast_simbolo_operador(OP_NAO), "!") == 0);
}

int main(void) {
    testa_soma_2_mais_3();
    testa_copia_de_texto();
    testa_literais_e_unario();
    testa_libera_nulo();
    testa_nomes_auxiliares();

    if (falhas) {
        printf("Testes da AST: %d falha(s)\n", falhas);
        return 1;
    }
    printf("Testes da AST: todos passaram\n");
    return 0;
}
