#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/ast.h"
#include "../../src/tabela_simbolos.h"
#include "../../src/parser.tab.h"

extern int yylineno;
extern int erros_lexicos;
extern AstNo *raiz_ast;
extern int construcoes_sem_ast;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

static int falhas = 0;

#define VERIFICA(cond) do { if (!(cond)) { \
    fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); falhas++; } } while (0)

static void testa_estado_inicial(void) {
    ts_inicia();
    Simbolo *p = ts_busca_visivel("printf");
    VERIFICA(p != NULL && p->categoria == SIM_FUNCAO_NATIVA && p->escopo == 0);
    VERIFICA(ts_num_simbolos() == 1 && ts_num_problemas() == 0);
    VERIFICA(ts_escopo_atual() == 0);
    ts_libera();
    VERIFICA(ts_num_simbolos() == 0 && ts_primeiro() == NULL);
}

static void testa_insercao_e_busca(void) {
    ts_inicia();
    ts_abre_escopo();
    Simbolo *x = ts_insere("x", TIPO_INT, SIM_VARIAVEL, 3);
    VERIFICA(x != NULL && x->escopo == ts_escopo_atual() && x->linha == 3 && x->visivel);
    VERIFICA(ts_busca_escopo_atual("x") == x);
    VERIFICA(ts_busca_funcao_atual("x") == x);
    VERIFICA(ts_busca_visivel("x") == x);
    VERIFICA(ts_busca_global("x") == NULL);
    VERIFICA(ts_busca_visivel("y") == NULL);
    ts_libera();
}

static void testa_saida_de_escopo_preserva_registros(void) {
    ts_inicia();
    ts_abre_escopo();
    Simbolo *a = ts_insere("a", TIPO_INT, SIM_PARAMETRO, 1);
    ts_abre_escopo();
    Simbolo *b = ts_insere("b", TIPO_FLOAT, SIM_VARIAVEL, 2);
    VERIFICA(ts_busca_visivel("b") == b && ts_busca_escopo_atual("a") == NULL);
    VERIFICA(ts_busca_funcao_atual("a") == a);
    int antes = ts_num_simbolos();
    ts_fecha_escopo();
    VERIFICA(ts_busca_visivel("b") == NULL);
    VERIFICA(ts_num_simbolos() == antes && b->visivel == 0 && strcmp(b->nome, "b") == 0);
    VERIFICA(ts_busca_visivel("a") == a);
    ts_fecha_escopo();
    VERIFICA(ts_busca_visivel("a") == NULL && a->visivel == 0);
    ts_fecha_escopo();
    VERIFICA(ts_busca_visivel("printf") != NULL);
    ts_libera();
}

static void testa_nomes_repetidos(void) {
    ts_inicia();
    ts_abre_escopo();
    Simbolo *x = ts_insere("x", TIPO_INT, SIM_PARAMETRO, 1);
    VERIFICA(ts_insere("x", TIPO_INT, SIM_VARIAVEL, 2) == NULL);
    ts_abre_escopo();
    VERIFICA(ts_insere("x", TIPO_INT, SIM_VARIAVEL, 3) == NULL);
    ts_fecha_escopo();
    ts_abre_escopo();
    VERIFICA(ts_insere("y", TIPO_INT, SIM_VARIAVEL, 4) != NULL);
    ts_fecha_escopo();
    ts_abre_escopo();
    VERIFICA(ts_insere("y", TIPO_INT, SIM_VARIAVEL, 5) != NULL);
    ts_fecha_escopo();

    const Problema *p = ts_problemas();
    VERIFICA(ts_num_problemas() == 2 && p != NULL);
    VERIFICA(p->tipo == PROB_REDECLARACAO && p->linha == 2 && p->anterior == x);
    VERIFICA(p->proximo && p->proximo->tipo == PROB_OCULTA && p->proximo->linha == 3);
    VERIFICA(ts_busca_visivel("x") == x);
    ts_libera();
}

static void testa_funcoes(void) {
    ts_inicia();
    Simbolo *proto = ts_insere_funcao("g", TIPO_INT, 1, 0, 1);
    VERIFICA(proto && !proto->definida && proto->num_parametros == 1 && proto->escopo == 0);
    Simbolo *def = ts_insere_funcao("g", TIPO_INT, 1, 1, 5);
    VERIFICA(def == proto && proto->definida && ts_num_problemas() == 0);
    VERIFICA(ts_insere_funcao("g", TIPO_INT, 1, 1, 9) == NULL);
    VERIFICA(ts_num_problemas() == 1 && ts_problemas()->tipo == PROB_FUNCAO_REDEFINIDA);
    VERIFICA(ts_insere_funcao("printf", TIPO_INT, 0, 1, 12) == NULL);
    VERIFICA(ts_num_problemas() == 2 && ts_problemas()->proximo->tipo == PROB_REDECLARACAO);
    VERIFICA(ts_problemas()->proximo->anterior->categoria == SIM_FUNCAO_NATIVA);
    ts_libera();
}

static int analisa_string(const char *codigo) {
    yylineno = 1;
    erros_lexicos = 0;
    construcoes_sem_ast = 0;
    raiz_ast = NULL;
    YY_BUFFER_STATE buf = yy_scan_string(codigo);
    int rc = yyparse();
    yy_delete_buffer(buf);
    return rc;
}

static void encerra_analise(void) {
    ast_libera(raiz_ast);
    raiz_ast = NULL;
    ts_libera();
    VERIFICA(ast_nos_alocados() == 0);
}

static AstNo *acha_no(AstNo *no, AstTipoNo tipo, const char *texto) {
    for (; no; no = no->proximo) {
        if (no->tipo == tipo && (!texto || (no->texto && strcmp(no->texto, texto) == 0))) {
            return no;
        }
        for (int i = 0; i < 3; i++) {
            AstNo *achado = acha_no(no->filho[i], tipo, texto);
            if (achado) {
                return achado;
            }
        }
    }
    return NULL;
}

static const Simbolo *acha_simbolo(const char *nome, int linha) {
    for (const Simbolo *s = ts_primeiro(); s; s = s->proximo) {
        if (strcmp(s->nome, nome) == 0 && s->linha == linha) {
            return s;
        }
    }
    return NULL;
}

static void testa_funcao_preenche_tabela(void) {
    const char *codigo =
        "int fatorial(int n) {\n"
        "    int resultado = 1;\n"
        "    int i = 1;\n"
        "    if (n > 1) {\n"
        "        resultado = n * fatorial(n - 1);\n"
        "    }\n"
        "    return resultado;\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);

    const Simbolo *f = ts_busca_global("fatorial");
    VERIFICA(f && f->categoria == SIM_FUNCAO && f->definida && f->num_parametros == 1 && f->linha == 1);
    const Simbolo *n = acha_simbolo("n", 1);
    const Simbolo *resultado = acha_simbolo("resultado", 2);
    const Simbolo *i = acha_simbolo("i", 3);
    VERIFICA(n && n->categoria == SIM_PARAMETRO && n->tipo == TIPO_INT && n->escopo != 0);
    VERIFICA(resultado && resultado->categoria == SIM_VARIAVEL && resultado->escopo == n->escopo);
    VERIFICA(i && i->escopo == n->escopo);
    VERIFICA(ts_num_problemas() == 0);
    VERIFICA(ts_num_simbolos() == 5);
    VERIFICA(n->visivel == 0 && resultado->visivel == 0);
    VERIFICA(ts_busca_visivel("n") == NULL && ts_busca_visivel("fatorial") == f);

    AstNo *func = raiz_ast->filho[0];
    VERIFICA(func->simbolo == f);
    VERIFICA(func->filho[0]->simbolo == n);
    AstNo *ret = acha_no(raiz_ast, AST_RETORNO, NULL);
    VERIFICA(ret && ret->filho[0]->tipo == AST_ID && ret->filho[0]->simbolo == resultado);
    AstNo *chamada = acha_no(raiz_ast, AST_CHAMADA, "fatorial");
    VERIFICA(chamada && chamada->simbolo == f);
    AstNo *atrib = acha_no(raiz_ast, AST_ATRIBUICAO, "resultado");
    VERIFICA(atrib && atrib->simbolo == resultado);
    AstNo *decl = acha_no(raiz_ast, AST_DECLARACAO, "i");
    VERIFICA(decl && decl->simbolo == i);
    encerra_analise();
}

static void testa_visibilidade_de_bloco(void) {
    const char *codigo =
        "int f() {\n"
        "    int a = 1;\n"
        "    if (a > 0) {\n"
        "        int b = 2;\n"
        "        a = b;\n"
        "    }\n"
        "    a = b;\n"
        "    return a;\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    const Simbolo *a = acha_simbolo("a", 2);
    const Simbolo *b = acha_simbolo("b", 4);
    VERIFICA(a && b && b->escopo != a->escopo && b->visivel == 0);

    AstNo *bloco = raiz_ast->filho[0]->filho[1];
    AstNo *se = acha_no(bloco, AST_IF, NULL);
    AstNo *dentro = se->filho[1]->filho[0]->proximo;
    VERIFICA(dentro && dentro->tipo == AST_ATRIBUICAO && dentro->filho[0]->simbolo == b);
    AstNo *fora = se->proximo;
    VERIFICA(fora && fora->tipo == AST_ATRIBUICAO && fora->simbolo == a);
    VERIFICA(fora->filho[0]->tipo == AST_ID && fora->filho[0]->simbolo == NULL);
    encerra_analise();
}

static void testa_inicializador_e_ponto_de_uso(void) {
    const char *codigo =
        "int f() {\n"
        "    int x = x + 1;\n"
        "    int y = 2;\n"
        "    return y;\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    AstNo *decl = acha_no(raiz_ast, AST_DECLARACAO, "x");
    VERIFICA(decl && decl->simbolo != NULL);
    VERIFICA(decl->filho[0]->filho[0]->tipo == AST_ID && decl->filho[0]->filho[0]->simbolo == NULL);
    encerra_analise();
}

static void testa_duplicidades_registradas(void) {
    const char *codigo =
        "int calcula(int a) {\n"
        "    int a = 1;\n"
        "    int b = 2;\n"
        "    int b = 3;\n"
        "    if (a > 0) {\n"
        "        int b = 4;\n"
        "    }\n"
        "    return b;\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    VERIFICA(ts_num_problemas() == 3);
    const Problema *p = ts_problemas();
    VERIFICA(p->tipo == PROB_REDECLARACAO && strcmp(p->nome, "a") == 0 && p->linha == 2 && p->anterior->linha == 1);
    p = p->proximo;
    VERIFICA(p->tipo == PROB_REDECLARACAO && strcmp(p->nome, "b") == 0 && p->linha == 4 && p->anterior->linha == 3);
    p = p->proximo;
    VERIFICA(p->tipo == PROB_OCULTA && strcmp(p->nome, "b") == 0 && p->linha == 6 && p->anterior->linha == 3);

    AstNo *decl_a = acha_no(raiz_ast, AST_DECLARACAO, "a");
    VERIFICA(decl_a && decl_a->simbolo == NULL);
    AstNo *ret = acha_no(raiz_ast, AST_RETORNO, NULL);
    VERIFICA(ret->filho[0]->simbolo == acha_simbolo("b", 3));
    encerra_analise();
}

static void testa_prototipo_e_definicao(void) {
    const char *codigo =
        "int g(int a);\n"
        "int g(int b) {\n"
        "    return b;\n"
        "}\n"
        "int principal() {\n"
        "    return g(1);\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    const Simbolo *g = ts_busca_global("g");
    VERIFICA(g && g->definida && g->linha == 1 && ts_num_problemas() == 0);
    VERIFICA(acha_simbolo("a", 1) == NULL);
    VERIFICA(acha_simbolo("b", 2) != NULL);
    AstNo *chamada = acha_no(raiz_ast, AST_CHAMADA, "g");
    VERIFICA(chamada && chamada->simbolo == g);
    encerra_analise();
}

static void testa_funcao_definida_duas_vezes_e_chamada_antes(void) {
    const char *codigo =
        "int h() {\n"
        "    return 1;\n"
        "}\n"
        "int h() {\n"
        "    return naoexiste(2);\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    VERIFICA(ts_num_problemas() == 1 && ts_problemas()->tipo == PROB_FUNCAO_REDEFINIDA);
    AstNo *segunda = raiz_ast->filho[0]->proximo;
    VERIFICA(segunda && segunda->simbolo == NULL);
    AstNo *chamada = acha_no(raiz_ast, AST_CHAMADA, "naoexiste");
    VERIFICA(chamada && chamada->simbolo == NULL);
    encerra_analise();
}

static void testa_escopo_do_for(void) {
    const char *codigo =
        "int f() {\n"
        "    int t = 0;\n"
        "    for (int i = 0; i < 3; i = i + 1) {\n"
        "        int z = 1;\n"
        "        t = t + z;\n"
        "    }\n"
        "    for (int i = 0; i < 2; i = i + 1) {\n"
        "        int i = 5;\n"
        "    }\n"
        "    t = i;\n"
        "    while (t > 0) {\n"
        "        int w = 1;\n"
        "        t = t - w;\n"
        "    }\n"
        "    return t;\n"
        "}\n";
    VERIFICA(analisa_string(codigo) == 0);
    VERIFICA(acha_simbolo("i", 3) && acha_simbolo("i", 3)->visivel == 0);
    VERIFICA(acha_simbolo("i", 7) != NULL);
    VERIFICA(acha_simbolo("z", 4) && acha_simbolo("w", 12));
    VERIFICA(acha_simbolo("i", 3)->escopo == acha_simbolo("z", 4)->escopo);
    VERIFICA(ts_num_problemas() == 1);
    VERIFICA(ts_problemas()->tipo == PROB_REDECLARACAO && ts_problemas()->linha == 8);
    AstNo *t_igual_i = acha_no(raiz_ast, AST_ATRIBUICAO, "t");
    VERIFICA(t_igual_i && t_igual_i->filho[0]->tipo == AST_ID && t_igual_i->filho[0]->simbolo == NULL);
    VERIFICA(construcoes_sem_ast == 3);
    encerra_analise();
}

static void testa_reinicio_entre_analises(void) {
    VERIFICA(analisa_string("int a() { int x = 1; return x; }\n") == 0);
    int primeira = ts_num_simbolos();
    encerra_analise();
    VERIFICA(analisa_string("int b() { return 1; }\n") == 0);
    VERIFICA(ts_num_simbolos() == 2 && primeira == 3);
    VERIFICA(ts_busca_global("a") == NULL && ts_busca_global("b") != NULL);
    encerra_analise();
}

static void testa_erro_sintatico_nao_vaza(void) {
    VERIFICA(analisa_string("int f() {\n    int x = 1\n    return x;\n}\n") != 0);
    VERIFICA(ts_busca_global("f") != NULL);
    ts_libera();
    VERIFICA(ts_num_simbolos() == 0);
}

int main(void) {
    testa_estado_inicial();
    testa_insercao_e_busca();
    testa_saida_de_escopo_preserva_registros();
    testa_nomes_repetidos();
    testa_funcoes();

    testa_funcao_preenche_tabela();
    testa_visibilidade_de_bloco();
    testa_inicializador_e_ponto_de_uso();
    testa_duplicidades_registradas();
    testa_prototipo_e_definicao();
    testa_funcao_definida_duas_vezes_e_chamada_antes();
    testa_escopo_do_for();
    testa_reinicio_entre_analises();
    testa_erro_sintatico_nao_vaza();

    if (falhas) {
        printf("Testes da tabela de simbolos: %d falha(s)\n", falhas);
        return 1;
    }
    printf("Testes da tabela de simbolos: todos passaram\n");
    return 0;
}