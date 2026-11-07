#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "tabela_simbolos.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int erros_lexicos;
extern AstNo *raiz_ast;
extern int construcoes_sem_ast;

int main(int argc, char **argv) {
    int mostrar_ast = 0;
    int mostrar_simbolos = 0;
    const char *arquivo = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ast") == 0) {
            mostrar_ast = 1;
        } else if (strcmp(argv[i], "--simbolos") == 0) {
            mostrar_simbolos = 1;
        } else if (strcmp(argv[i], "--sintatico") == 0) {
        } else {
            arquivo = argv[i];
        }
    }

    if (arquivo) {
        yyin = fopen(arquivo, "r");
        if (!yyin) {
            perror("erro ao abrir arquivo de entrada");
            return 1;
        }
    }

    int erro_sintatico = yyparse();

    if (arquivo) {
        fclose(yyin);
    }

    if (erro_sintatico == 0 && erros_lexicos == 0) {
        if (mostrar_ast && raiz_ast) {
            ast_imprime(stdout, raiz_ast);
        }
        if (mostrar_simbolos) {
            ts_imprime(stdout);
        }
        if (construcoes_sem_ast > 0) {
            printf("Nota: %d construcao(oes) de repeticao (while/for) reconhecida(s) sintaticamente, mas ainda sem representacao na AST inicial.\n",
                   construcoes_sem_ast);
        }
        printf("Analise concluida: programa aceito.\n");
        ast_libera(raiz_ast);
        raiz_ast = NULL;
        ts_libera();
        return 0;
    }

    ast_libera(raiz_ast);
    raiz_ast = NULL;
    ts_libera();
    printf("Analise concluida: programa rejeitado.\n");
    return 1;
}