/*
 * teste_ast.c - testes do modulo da AST (src/ast.c) e da construcao da AST
 * pelo parser integrado ao lexer (Issue 3.3).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/ast.h"
#include "../../src/parser.tab.h"

extern FILE *yyin;
extern int yylineno;
extern int erros_lexicos;
extern AstNo *raiz_ast;
extern int construcoes_sem_ast;

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
extern void yyrestart(FILE *input_file);

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

/* arquivo no diretorio atual: tmpfile() pode falhar no Windows (MinGW grava em C:\\) */
#define ARQUIVO_SAIDA "ast_saida.tmp"

/* imprime a arvore em um arquivo temporario e devolve o conteudo (liberar com free) */
static char *imprime_em_texto(const AstNo *no) {
    FILE *tmp = fopen(ARQUIVO_SAIDA, "w+");
    if (!tmp) {
        perror("erro ao criar " ARQUIVO_SAIDA);
        exit(1);
    }
    ast_imprime(tmp, no);
    long tamanho = ftell(tmp);
    rewind(tmp);
    char *s = malloc(tamanho + 1);
    size_t lidos = fread(s, 1, tamanho, tmp);
    s[lidos] = '\0';
    fclose(tmp);
    remove(ARQUIVO_SAIDA);
    return s;
}

static void testa_impressao_soma(void) {
    AstNo *e = ast_binario(OP_SOMA, ast_num_int(2, 1), ast_num_int(3, 1), 1);
    char *s = imprime_em_texto(e);
    VERIFICA(strcmp(s, "BINARIO +\n  NUM_INT 2\n  NUM_INT 3\n") == 0);
    free(s);
    ast_libera(e);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_impressao_classifica(void) {
    const char *esperado =
        "PROGRAMA\n"
        "  FUNCAO int classifica\n"
        "    PARAMETRO int n\n"
        "    BLOCO\n"
        "      IF\n"
        "        BINARIO >\n"
        "          ID n\n"
        "          NUM_INT 0\n"
        "        BLOCO\n"
        "          RETORNO\n"
        "            NUM_INT 1\n"
        "        ELSE\n"
        "        BLOCO\n"
        "          RETORNO\n"
        "            UNARIO -\n"
        "              NUM_INT 1\n";
    AstNo *p = monta_classifica();
    char *s = imprime_em_texto(p);
    VERIFICA(strcmp(s, esperado) == 0);
    if (strcmp(s, esperado) != 0) {
        fprintf(stderr, "obtido:\n%s", s);
    }
    free(s);
    ast_libera(p);
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_impressao_opcionais(void) {
    const char *esperado =
        "BLOCO\n"
        "  DECLARACAO int x\n"
        "  ATRIBUICAO c\n"
        "    CHAR_LIT 'x'\n"
        "  CHAMADA printf\n"
        "    STRING_LIT \"fim\"\n"
        "    NUM_FLOAT 3.14\n";
    AstNo *args = ast_lista_anexa(NULL, ast_string_lit("fim", 3));
    args = ast_lista_anexa(args, ast_num_float(3.14, 3));
    AstNo *cmds = ast_lista_anexa(NULL, ast_declaracao(TIPO_INT, "x", NULL, 1));
    cmds = ast_lista_anexa(cmds, ast_atribuicao("c", ast_char_lit("x", 2), 2));
    cmds = ast_lista_anexa(cmds, ast_chamada("printf", args, 3));
    AstNo *b = ast_bloco(cmds, 1);
    char *s = imprime_em_texto(b);
    VERIFICA(strcmp(s, esperado) == 0);
    if (strcmp(s, esperado) != 0) {
        fprintf(stderr, "obtido:\n%s", s);
    }
    free(s);

    AstNo *proto = ast_funcao(TIPO_VOID, "f", NULL, NULL, 5);
    s = imprime_em_texto(proto);
    VERIFICA(strcmp(s, "FUNCAO void f (prototipo)\n") == 0);
    free(s);

    ast_libera(b);
    ast_libera(proto);
    VERIFICA(ast_nos_alocados() == 0);
}

/* --- Testes de integracao Flex + Bison -> AST (Issue 3.3) --- */

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

static void testa_parser_valores_lexicos(void) {
    const char *codigo =
        "int f(int p) {\n"
        "    float taxa = 3.14;\n"
        "    char letra = 'k';\n"
        "    avisa(\"ola mundo\");\n"
        "    return p;\n"
        "}\n";
    int rc = analisa_string(codigo);
    VERIFICA(rc == 0 && erros_lexicos == 0 && raiz_ast != NULL);
    AstNo *fn = raiz_ast->filho[0];
    VERIFICA(fn->tipo == AST_FUNCAO && strcmp(fn->texto, "f") == 0 && fn->linha == 1);
    VERIFICA(fn->filho[0]->tipo == AST_PARAMETRO && strcmp(fn->filho[0]->texto, "p") == 0);

    AstNo *cmd1 = fn->filho[1]->filho[0];
    VERIFICA(cmd1->tipo == AST_DECLARACAO && cmd1->tipo_dado == TIPO_FLOAT && cmd1->linha == 2);
    VERIFICA(cmd1->filho[0]->tipo == AST_NUM_FLOAT && cmd1->filho[0]->valor_float == 3.14);

    AstNo *cmd2 = cmd1->proximo;
    VERIFICA(cmd2->tipo == AST_DECLARACAO && cmd2->tipo_dado == TIPO_CHAR && cmd2->linha == 3);
    VERIFICA(cmd2->filho[0]->tipo == AST_CHAR_LIT && strcmp(cmd2->filho[0]->texto, "k") == 0);

    AstNo *cmd3 = cmd2->proximo;
    VERIFICA(cmd3->tipo == AST_CHAMADA && strcmp(cmd3->texto, "avisa") == 0 && cmd3->linha == 4);
    VERIFICA(cmd3->filho[0]->tipo == AST_STRING_LIT && strcmp(cmd3->filho[0]->texto, "ola mundo") == 0);

    ast_libera(raiz_ast);
    raiz_ast = NULL;
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_parser_precedencia_associatividade(void) {
    const char *codigo =
        "int main() {\n"
        "    int x = 2 + 3 * 4;\n"
        "    int y = 10 - 3 - 2;\n"
        "    return x;\n"
        "}\n";
    int rc = analisa_string(codigo);
    VERIFICA(rc == 0 && erros_lexicos == 0 && raiz_ast != NULL);

    /* 2 + 3 * 4: soma no topo, multiplicacao a direita */
    AstNo *decl_x = raiz_ast->filho[0]->filho[1]->filho[0];
    AstNo *soma = decl_x->filho[0];
    VERIFICA(soma->tipo == AST_BINARIO && soma->op == OP_SOMA);
    VERIFICA(soma->filho[0]->tipo == AST_NUM_INT && soma->filho[0]->valor_int == 2);
    VERIFICA(soma->filho[1]->tipo == AST_BINARIO && soma->filho[1]->op == OP_MUL);

    /* 10 - 3 - 2: associatividade a esquerda -> (10 - 3) a esquerda */
    AstNo *decl_y = decl_x->proximo;
    AstNo *sub_ext = decl_y->filho[0];
    VERIFICA(sub_ext->tipo == AST_BINARIO && sub_ext->op == OP_SUB);
    VERIFICA(sub_ext->filho[0]->tipo == AST_BINARIO && sub_ext->filho[0]->op == OP_SUB);
    VERIFICA(sub_ext->filho[1]->tipo == AST_NUM_INT && sub_ext->filho[1]->valor_int == 2);

    ast_libera(raiz_ast);
    raiz_ast = NULL;
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_parser_arvore_esperada(void) {
    FILE *in = fopen("tests/validos/precedencia_e_retorno.c", "r");
    VERIFICA(in != NULL);
    if (!in) return;

    yylineno = 1;
    erros_lexicos = 0;
    construcoes_sem_ast = 0;
    raiz_ast = NULL;
    yyrestart(in);
    int rc = yyparse();
    fclose(in);
    VERIFICA(rc == 0 && erros_lexicos == 0 && raiz_ast != NULL);

    char *obtido = imprime_em_texto(raiz_ast);

    FILE *esp = fopen("tests/ast/precedencia_e_retorno.ast", "r");
    VERIFICA(esp != NULL);
    if (esp) {
        char buf_esp[4096];
        size_t j = 0;
        int ch;
        while ((ch = fgetc(esp)) != EOF && j + 1 < sizeof(buf_esp)) {
            if (ch != '\r') {
                buf_esp[j++] = (char)ch;
            }
        }
        buf_esp[j] = '\0';
        fclose(esp);

        /* remove \r de obtido caso esteja no Windows */
        char buf_obt[4096];
        size_t k = 0;
        for (size_t i = 0; obtido[i] != '\0' && k + 1 < sizeof(buf_obt); i++) {
            if (obtido[i] != '\r') {
                buf_obt[k++] = obtido[i];
            }
        }
        buf_obt[k] = '\0';

        while (j > 0 && (buf_esp[j - 1] == '\n' || buf_esp[j - 1] == ' ')) buf_esp[--j] = '\0';
        while (k > 0 && (buf_obt[k - 1] == '\n' || buf_obt[k - 1] == ' ')) buf_obt[--k] = '\0';
        VERIFICA(strcmp(buf_obt, buf_esp) == 0);
    }

    free(obtido);
    ast_libera(raiz_ast);
    raiz_ast = NULL;
    VERIFICA(ast_nos_alocados() == 0);
}

static void testa_parser_repeticao_sem_ast(void) {
    const char *codigo =
        "int laco(int n) {\n"
        "    int i = 0;\n"
        "    while (i < n) { i = i + 1; }\n"
        "    for (int j = 0; j < n; j = j + 1) { i = i + j; }\n"
        "    return i;\n"
        "}\n";
    int rc = analisa_string(codigo);
    VERIFICA(rc == 0 && erros_lexicos == 0);
    VERIFICA(construcoes_sem_ast == 2);
    ast_libera(raiz_ast);
    raiz_ast = NULL;
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
    testa_impressao_soma();
    testa_impressao_classifica();
    testa_impressao_opcionais();

    /* Testes de integracao Flex + Bison -> AST (Issue 3.3) */
    testa_parser_valores_lexicos();
    testa_parser_precedencia_associatividade();
    testa_parser_arvore_esperada();
    testa_parser_repeticao_sem_ast();

    if (falhas) {
        printf("Testes da AST: %d falha(s)\n", falhas);
        return 1;
    }
    printf("Testes da AST: todos passaram\n");
    return 0;
}