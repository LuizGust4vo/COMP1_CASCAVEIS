# Issue #7 — Validação da gramática inicial

## Escopo e resultado

A revisão parte da `main` no commit `5dfd681`, que já contém tokens,
precedências e regras básicas implementadas. Essas regras foram preservadas.
A alteração em `src/parser.y` adiciona `%expect 0`: a geração pelo Bison passa
a falhar se forem introduzidos conflitos não resolvidos. Não foram adicionadas
construções à linguagem.

Esta entrega cobre os dois primeiros itens da
[issue #7](https://github.com/LuizGust4vo/COMP1_CASCAVEIS/issues/7).
A integração está registrada em [validação da integração](validacao_issue_7_integracao.md).

## Tokens

Os 34 tokens declarados por `%token` correspondem aos retornados pelo lexer:

| Categoria | Tokens |
|---|---|
| Tipos | `INT FLOAT CHAR VOID` |
| Controle | `IF ELSE WHILE FOR RETURN` |
| Identificadores e literais | `ID NUM_INT NUM_FLOAT CHAR_LIT STRING_LIT` |
| Comparações | `EQ NE LE GE LT GT` |
| Lógicos | `AND OR NOT` |
| Atribuição e aritmética | `ASSIGN PLUS MINUS TIMES DIVIDE` |
| Delimitadores | `LBRACE RBRACE LPAREN RPAREN COMMA SEMI` |

`UMINUS` é um símbolo auxiliar de precedência do parser, não um token retornado
pelo Flex. O menos unário recebe essa precedência por `%prec UMINUS`.

## Precedência e gramática

A ordem crescente é `OR`, `AND`, `EQ NE`, `LT GT LE GE`, `PLUS MINUS`,
`TIMES DIVIDE`, `NOT UMINUS`. Os binários usam `%left`; os unários usam
`%right`. Parênteses agrupam expressões explicitamente.

Foram conferidas as produções de programa, declaração/definição de função,
tipos, parâmetros, blocos, comandos, declaração de variável, atribuição e
expressão contra a seção 4 da [especificação](Especificacao_da_Linguagem.md).
O programa exige ao menos uma função; variáveis são declaradas dentro de
funções. As regras de controle já existentes foram preservadas.

## Reprodução da verificação

Na raiz do repositório, em Linux:

```bash
make build
bison --report=solved --report-file=/tmp/issue-7-parser.output \
  -o /tmp/issue-7-parser.c src/parser.y
```

Ambiente utilizado: Bison 3.8.2, Flex 2.6.4 e GCC 13.3.0.
A geração terminou com código 0, sem conflitos não resolvidos. O relatório
registra decisões resolvidas pelas precedências, por exemplo:

- `PLUS < TIMES`: deslocamento para multiplicação antes da redução de soma.
- `%left MINUS`: redução para associatividade à esquerda.
- `TIMES < UMINUS`: redução do menos unário antes da multiplicação.

Essas decisões resolvidas são esperadas e não violam `%expect 0`.
A aceitação de uma expressão sozinha não prova sua ordem de avaliação.

As entradas abaixo foram enviadas pela entrada padrão, sem adicionar arquivos
à suíte da issue #8. Para reproduzir cada linha, use:

```bash
printf '%s\n' 'CODIGO_DA_TABELA' | ./lexer_test
echo $?
```

| Entrada | Retorno observado |
|---|---|
| `void f(){}` | 0 — aceito |
| `int soma(int a, int b);` | 0 — aceito |
| `int f(){ int x; x = 2 + 3 * 4; return x; }` | 0 — aceito |
| `int f(){ return 1; } int g(){ return f(); }` | 0 — aceito |
| `int f(){ return !(1 < 2) || -3 * (2 + 4); }` | 0 — aceito |
| `int f(){ int = 1; }` | 1 — rejeitado |
| `int f(){ int x = ; }` | 1 — rejeitado |
| `int f(int a int b);` | 1 — rejeitado |

Também foi aceita uma função com declarações de `int`, `float` e `char`,
com e sem inicialização. Os quatro arquivos válidos e dez inválidos existentes
mantiveram os resultados esperados, conferidos pelos códigos de saída.

## Limites

O parser reconhece sintaxe; ainda não constrói AST, verifica tipos ou gera
Python. Portanto, esta validação não comprova semântica nem execução das
expressões. Não foram criados testes permanentes nem alterado o Makefile.
