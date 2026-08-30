# Guia de Execução e Testes do Analisador Léxico

Este documento detalha como compilar o projeto, executar a bateria de testes automatizados e orienta a equipe sobre exatamente quais linhas e variáveis alterar no arquivo `Makefile` caso ocorram modificações na estrutura de diretórios ou nos nomes dos arquivos.

## Como Executar os Testes

O `Makefile` detecta automaticamente o sistema operacional (Windows ou Linux/macOS) e ajusta os comandos internos. No terminal, na raiz do projeto, utilize os comandos abaixo:

*   **Rodar todos os testes (Válidos e Inválidos):**
    ```bash
    make
    # ou
    make all
    ```
*   **Rodar apenas os testes válidos (Caminho feliz):**
    ```bash
    make test-validos
    ```
*   **Rodar apenas os testes inválidos (QA / Limites):**
    ```bash
    make test-invalidos
    ```
*   **Executar em modo interativo (Manual):**
    ```bash
    make run
    ```
*   **Limpar arquivos gerados:**
    ```bash
    make clean
    ```

---

## O Que Alterar no Makefile (Guia por Integrante)

Caso você precise alterar nomes de arquivos ou diretórios no repositório, abra o arquivo `Makefile` na raiz e modifique **exatamente** a variável correspondente nas linhas iniciais de configuração:

```makefile
# configuracoes gerais
CC = gcc
FLEX = flex
LEX_SRC = src/lexer.l
TEST_DIR_VALIDOS = tests/validos
TEST_DIR_INVALIDOS = tests/invalidos