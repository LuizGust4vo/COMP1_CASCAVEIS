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

static void testa_lista_anexa(void) {
    AstNo *l = ast_lista_anexa(NULL, ast_id("a", 1));
    l = ast_lista_anexa(l, ast_id("b", 1));
    l = ast_lista_anexa(l, ast_id("c", 1));
    VERIFICA(strcmp(l->texto, "a") == 0);
    VERIFICA(strcmp(l->proximo->texto, "b") == 0);
    VERIFICA(strcmp(l->proximo->proximo->texto, "c") == 0 && l->proximo->proximo->proximo == NULL);
    ast_libera(l);
    VERIFICA(ast_nos_alocados() == 0);
}

/* int classifica(int n) { if (n > 0) { return 1; } else { return -1; } } (secao 5.2) */
static AstNo *monta_classifica(void) {
    AstNo *cond = ast_binario(OP_MAIOR, ast_id("n", 2), ast_num_int(0, 2), 2);
    AstNo *entao = ast_bloco(ast_retorno(ast_num_int(1, 3), 3), 2);
    AstNo *senao = ast_bloco(ast_retorno(ast_unario(OP_NEGATIVO, ast_num_int(1, 5), 5), 5), 4);
    AstNo *corpo = ast_bloco(ast_lista_anexa(NULL, ast_if(cond, entao, senao, 2)), 1);
    AstNo *funcao = ast_funcao(TIPO_INT, "classifica",
                               ast_lista_anexa(NULL, ast_parametro(TIPO_INT, "n", 1)), corpo, 1);
    return ast_programa(ast_lista_anexa(NULL, funcao));
}

static void testa_funcao_classifica(void) {
    AstNo *p = monta_classifica();
    VERIFICA(p->tipo == AST_PROGRAMA);
    AstNo *f = p->filho[0];
    VERIFICA(f->tipo == AST_FUNCAO && f->tipo_dado == TIPO_INT && strcmp(f->texto, "classifica") == 0);
    VERIFICA(f->filho[0]->tipo == AST_PARAMETRO && f->filho[0]->tipo_dado == TIPO_INT);
    VERIFICA(strcmp(f->filho[0]->texto, "n") == 0);
    VERIFICA(f->filho[1]->tipo == AST_BLOCO);
    AstNo *se = f->filho[1]->filho[0];
    VERIFICA(se->tipo == AST_IF && se->linha == 2 && se->filho[2] != NULL);
    VERIFICA(se->filho[1]->filho[0]->tipo == AST_RETORNO && se->filho[1]->filho[0]->linha == 3);
    ast_libera(p);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_opcionais_nulos(void) {
    AstNo *d = ast_declaracao(TIPO_INT, "x", NULL, 1);
    VERIFICA(d->tipo == AST_DECLARACAO && d->tipo_dado == TIPO_INT && d->filho[0] == NULL);
    AstNo *se = ast_if(ast_id("x", 2), ast_bloco(NULL, 2), NULL, 2);
    VERIFICA(se->filho[2] == NULL);
    AstNo *proto = ast_funcao(TIPO_VOID, "f", NULL, NULL, 3);
    VERIFICA(proto->tipo_dado == TIPO_VOID && proto->filho[0] == NULL && proto->filho[1] == NULL);
    AstNo *b = ast_bloco(NULL, 4);
    VERIFICA(b->tipo == AST_BLOCO && b->filho[0] == NULL);
    ast_libera(d);
    ast_libera(se);
    ast_libera(proto);
    ast_libera(b);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_atribuicao_e_retorno(void) {
    AstNo *a = ast_atribuicao("total", ast_binario(OP_SOMA, ast_id("total", 7), ast_id("i", 7), 7), 7);
    VERIFICA(a->tipo == AST_ATRIBUICAO && strcmp(a->texto, "total") == 0);
    VERIFICA(a->filho[0]->op == OP_SOMA);
    AstNo *r = ast_retorno(ast_id("total", 9), 9);
    VERIFICA(r->tipo == AST_RETORNO && strcmp(r->filho[0]->texto, "total") == 0);
    ast_libera(a);
    ast_libera(r);
    VERIFICA(ast_nos_alocados() == 0);
}

int main(void) {
    testa_soma_2_mais_3();
    testa_copia_de_texto();
    testa_literais_e_unario();
    testa_libera_nulo();
    testa_nomes_auxiliares();
    testa_lista_anexa();
    testa_funcao_classifica();
    testa_opcionais_nulos();
    testa_atribuicao_e_retorno();

    if (falhas) {
        printf("Testes da AST: %d falha(s)\n", falhas);
        return 1;
    }
    printf("Testes da AST: todos passaram\n");
    return 0;
}
