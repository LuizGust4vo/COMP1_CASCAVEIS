#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "tabela_simbolos.h"
#include "semantico.h"
#include "ir.h"
#include "codegen.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int erros_lexicos;
extern AstNo *raiz_ast;

int main(int argc, char **argv) {
    int mostrar_ast = 0;
    int mostrar_simbolos = 0;
    int mostrar_ir = 0;
    int somente_sintatico = 0;
    int gerar_python = 0;

    const char *arquivo = NULL;
    const char *arquivo_python = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--ast") == 0)
        {
            mostrar_ast = 1;
        }
        else if (strcmp(argv[i], "--simbolos") == 0)
        {
            mostrar_simbolos = 1;
        }
        else if (strcmp(argv[i], "--ir") == 0)
        {
            mostrar_ir = 1;
        }
        else if (strcmp(argv[i], "--python") == 0)
        {
            gerar_python = 1;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "Uso: --python <arquivo_saida.py>\n");
                return 1;
            }
            arquivo_python = argv[++i];
        }
        else if (strcmp(argv[i], "--sintatico") == 0)
        {
            somente_sintatico = 1;
        }
        else
        {
            arquivo = argv[i];
        }
    }

        if (arquivo)
        {
            yyin = fopen(arquivo, "r");
            if (!yyin)
            {
                perror("erro ao abrir arquivo de entrada");
                return 1;
            }
        }

        int erro_sintatico = yyparse();

        if (arquivo)
        {
            fclose(yyin);
        }

        int erros_semanticos = 0;
        if (erro_sintatico == 0 && erros_lexicos == 0 && !somente_sintatico)
        {
            erros_semanticos = semantico_analisa(raiz_ast);
        }

        if (erro_sintatico == 0 && erros_lexicos == 0 && erros_semanticos == 0)
        {
            if (mostrar_ast && raiz_ast)
            {
                ast_imprime(stdout, raiz_ast);
            }
            if (mostrar_simbolos)
            {
                ts_imprime(stdout);
            }
            if ((mostrar_ir || gerar_python) && raiz_ast && !somente_sintatico)
            {
                /* Gera RI apenas para programas aceitos pela análise semântica */
                ir_gera_programa(raiz_ast);

                if (mostrar_ir)
                {
                    ir_imprime(stdout);
                }

                if (gerar_python)
                {
                    FILE *saida_python = fopen(arquivo_python, "w");

                    if (!saida_python)
                    {
                        perror("erro ao criar arquivo Python");
                        ir_libera();
                        ast_libera(raiz_ast);
                        raiz_ast = NULL;
                        ts_libera();
                        return 1;
                    }

                    if (codegen_gera_python(saida_python) != 0)
                    {
                        fprintf(stderr, "Erro ao gerar codigo Python.\n");
                        fclose(saida_python);
                        ir_libera();
                        ast_libera(raiz_ast);
                        raiz_ast = NULL;
                        ts_libera();
                        return 1;
                    }

                    fclose(saida_python);

                    printf("Codigo Python gerado em: %s\n", arquivo_python);
                }

                ir_libera();
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
        if (erros_semanticos > 0)
        {
            printf("Analise concluida: programa rejeitado (%d erro(s) semantico(s)).\n", erros_semanticos);
        }
        else
        {
            printf("Analise concluida: programa rejeitado.\n");
        }
        return 1;
    }