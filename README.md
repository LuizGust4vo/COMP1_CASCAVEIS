# COMP1_CASCAVEIS

* Repositório da Equipe 05 - Cascaveis, criado para a disciplina de compiladores 1 do curso de Engenharia de Software da UnB
  
* Repositório do professor: https://github.com/sergioaafreitas/COMP1

## Equipe de Desenvolvimento

<center>
<table style="margin-left: auto; margin-right: auto;">
    <tr>
        <td align="center">
            <a href="https://github.com/iicaroll">
                <img style="border-radius: 50%;" src="https://github.com/iicaroll.png" width="150px;"/>
                <h5 class="text-center">Ana Caroline</h5>
            </a>
        </td>
        <td align="center">
            <a href="https://github.com/femathrl0">
                <img style="border-radius: 50%;" src="https://github.com/femathrl0.png" width="150px;"/>
                <h5 class="text-center">Felipe Matheus</h5>
            </a>
        </td>
        <td align="center">
            <a href="https://github.com/G0LD3N-B3RG">
                <img style="border-radius: 50%;" src="https://github.com/G0LD3N-B3RG.png" width="150px;"/>
                <h5 class="text-center">Gabriel Goldenberg</h5>
            </a>
        </td>
        <td align="center">
            <a href="https://github.com/LuizGust4vo">
                <img style="border-radius: 50%;" src="https://github.com/LuizGust4vo.png" width="150px;"/>
                <h5 class="text-center">Luiz Almeida</h5>
            </a>
        </td>
        <td align="center">
            <a href="https://github.com/PedroAraujo004">
                <img style="border-radius: 50%;" src="https://github.com/PedroAraujo004.png" width="150px;"/>
                <h5 class="text-center">Pedro Araujo</h5>
            </a>
        </td>
</table>

</center>

## Sobre o compilador

Compilador de um **subconjunto da linguagem C para Python**, construído com Flex (análise léxica) e Bison (análise sintática) em C. O programa de entrada passa por todas as etapas clássicas de compilação e, se for aceito, é traduzido para um arquivo `.py` executável.

```
código C ──► Léxico (Flex) ──► Sintático (Bison) ──► AST + Tabela de Símbolos
         ──► Análise Semântica ──► Código Intermediário (três endereços) ──► Python
```

| Etapa | Arquivos | Papel |
|---|---|---|
| Léxico | `src/lexer.l` | Reconhece tokens e reporta erros léxicos (caracteres inválidos, strings/comentários não terminados). |
| Sintático | `src/parser.y` | Valida a gramática e constrói a AST. |
| AST | `src/ast.c`, `src/ast.h` | Árvore sintática abstrata usada pelas etapas seguintes. |
| Tabela de símbolos | `src/tabela_simbolos.c`, `src/tabela_simbolos.h` | Variáveis, parâmetros e funções com controle de escopos. |
| Semântico | `src/semantico.c`, `src/semantico.h` | Tipos, declarações, escopos, chamadas e retornos. |
| Código intermediário | `src/ir.c`, `src/ir.h` | Gera a RI em código de três endereços. |
| Otimização (opcional) | `src/otimizador.c`, `src/otimizador.h` | Dobra de constantes, simplificações, propagação, eliminação de cópias e de código morto (`--otimizar`). |
| Código final | `src/codegen.c`, `src/codegen.h` | Traduz a RI para Python. |
| Orquestração | `src/main.c` | Lê a entrada, executa as etapas e trata as opções de linha de comando. |

Um programa só avança para a próxima etapa se não houver erros nas anteriores: **programas com erro léxico, sintático ou semântico nunca geram código intermediário nem Python**.

## Requisitos

- `gcc` (ou outro compilador C compatível)
- `flex`
- `bison`
- `make`
- `python3` (para executar o código gerado e a suíte de testes)

No Linux (Debian/Ubuntu):

```bash
sudo apt install build-essential flex bison python3
```

No Fedora:

```bash
sudo dnf install gcc make flex bison python3
```

> Para instruções detalhadas no Linux ou Windows, consulte o [Guia de Configuração do Ambiente de Desenvolvimento](docs/guias/Guia%20-%20Configuração%20do%20ambiente%20de%20desenvolvimento.md). No Windows, recomenda-se o uso do WSL.

## Compilação

```bash
make clean
make build
```

O comando gera o executável `lexer_test` (`lexer_test.exe` no Windows).

## Uso

```bash
./lexer_test [opções] [arquivo.c]
```

Sem arquivo, o programa é lido da entrada padrão (`./lexer_test < arquivo.c`).

| Opção | Efeito |
|---|---|
| *(nenhuma)* | Executa análise léxica, sintática e semântica e informa se o programa foi aceito. |
| `--sintatico` | Executa apenas as análises léxica e sintática. |
| `--ast` | Exibe a AST do programa aceito. |
| `--simbolos` | Exibe a tabela de símbolos. |
| `--ir` | Exibe o código intermediário (três endereços). |
| `--python <saida.py>` | Gera o código Python final no arquivo indicado. |
| `--otimizar` | Aplica as otimizações da RI antes de imprimi-la ou gerar o Python, e imprime um resumo em `stderr`. |

As opções podem ser combinadas. O código de saída é `0` quando o programa é aceito e `1` quando é rejeitado.

### Exemplo completo: C → Python → execução

```bash
./lexer_test --python primos.py tests/e2e/entradas/06_programa_integrado.c
python3 primos.py
```

Saída esperada:

```
6
41
```

Para visualizar a representação intermediária do mesmo programa, sem e com otimizações:

```bash
./lexer_test --ir tests/e2e/entradas/06_programa_integrado.c
./lexer_test --otimizar --ir tests/e2e/entradas/06_programa_integrado.c
```

### Casos de erro

```bash
./lexer_test tests/invalidos/caracteres_nao_reconhecidos.c        # erro léxico
./lexer_test tests/invalidos/erro_sintatico_expressao.c           # erro sintático
./lexer_test tests/semanticos/invalidos/variavel_nao_declarada.c  # erro semântico
```

Em todos os casos o programa é rejeitado com a mensagem correspondente, e `--python` não gera arquivo.

## Testes

| Comando | O que testa |
|---|---|
| `make test` | Suíte completa (todos os alvos abaixo). |
| `make test-validos` / `make test-invalidos` | Análise léxica e sintática (`tests/validos`, `tests/invalidos`). |
| `make test-ast` | Construção da AST. |
| `make test-simbolos` | Tabela de símbolos e escopos. |
| `make test-semanticos` | Programas semanticamente válidos e inválidos (`tests/semanticos`). |
| `make test-ir` | Gerador de código intermediário. |
| `make test-consolidado` | Executor `tests/executar_testes.py`: códigos de saída, gabaritos de RI (`tests/ir`) e testes end-to-end. |
| `make test-e2e` | Apenas os testes end-to-end. |
| `make test-otimizacao` | Otimizações da RI: gabaritos de `tests/otimizacao` e testes end-to-end com `--otimizar`. |

Os testes **end-to-end** compilam cada programa de `tests/e2e/entradas/` para Python, executam o arquivo gerado e comparam a saída com `tests/e2e/esperados/`. Também verificam que nenhum programa de `tests/invalidos/` ou `tests/semanticos/invalidos/` gera código Python.

Para adicionar um caso end-to-end, crie `tests/e2e/entradas/<nome>.c` e a saída esperada em `tests/e2e/esperados/<nome>.out`.

## Linguagem suportada

- **Tipos:** `int`, `float`, `char` e `void` (retorno de funções).
- **Controle:** `if`/`else`, `while` e `for`.
- **Funções:** definição, protótipos, parâmetros, chamadas (inclusive recursivas) e `return`.
- **Operadores:** aritméticos (`+ - * /`, `-` unário), relacionais (`== != < <= > >=`), lógicos (`&& || !`) e atribuição (`=`).
- **Saída:** a função nativa `printf`, que recebe um único argumento (expressão ou literal de string) e vira `print` no Python.
- **Comentários** de linha (`//`) e de bloco (`/* */`).

Semântica de C preservada no Python gerado:

- Divisão inteira trunca em direção a zero (`-7 / 2 == -3`), como em C, e não arredonda para baixo como o `//` do Python.
- Valores `char` participam de operações numéricas pelo seu código (via `ord`).
- `&&` e `||` mantêm o curto-circuito: o segundo operando só é avaliado quando necessário.

A especificação completa está em [docs/especificacao_da_Linguagem.md](docs/especificacao_da_Linguagem.md) e a representação intermediária em [docs/representacao_intermediaria.md](docs/representacao_intermediaria.md).

## Limitações conhecidas

Não fazem parte do subconjunto suportado:

- Diretivas do pré-processador (`#include`, `#define`).
- Variáveis globais, arrays, ponteiros, `struct`.
- `break`, `continue`, `switch`, `do`/`while`.
- Operador `%`, atribuições compostas (`+=` etc.), `++`/`--`, operadores bit a bit e de deslocamento.
- Conversões explícitas de tipo (*casts*).
- `printf` com string de formato e múltiplos argumentos.
- Leitura de entrada (`scanf`).

Esses casos são rejeitados nas etapas de análise (ver [`tests/invalidos/`](tests/invalidos/)).

## Documentação

- [Especificação da linguagem](docs/especificacao_da_Linguagem.md)
- [Representação intermediária](docs/representacao_intermediaria.md)
- [Guias da disciplina](docs/guias/)
- Pontos de controle: [P1](docs/p1/) e [P2](docs/p2/)

## Licença ©

Este projeto está sob licença [MIT](LICENSE) ©