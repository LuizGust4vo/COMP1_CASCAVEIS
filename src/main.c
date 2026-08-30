/*
 * main.c - ponto de entrada do prototipo de analise lexica/sintatica.
 * Le codigo fonte da entrada padrao (ou de um arquivo, se informado por
 * argumento) e reporta se o programa foi aceito ou rejeitado.
 */

#include <stdio.h>

extern int yyparse(void);
extern FILE *yyin;
extern int erros_lexicos;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("erro ao abrir arquivo de entrada");
            return 1;
        }
    }

    int erro_sintatico = yyparse();

    if (erro_sintatico == 0 && erros_lexicos == 0) {
        printf("Analise concluida: programa aceito.\n");
        return 0;
    }

    printf("Analise concluida: programa rejeitado.\n");
    return 1;
}
