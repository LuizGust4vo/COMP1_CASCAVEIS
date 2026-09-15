# Issue #7 — Validação da integração Flex–Bison

## Escopo e alterações

Esta entrega cobre a integração solicitada no terceiro item da
[issue #7](https://github.com/LuizGust4vo/COMP1_CASCAVEIS/issues/7).
A gramática está descrita em [validação da gramática](validacao_issue_7_gramatica.md).

O fluxo já existia na `main` no commit `5dfd681`. Em `src/main.c`, a declaração
manual de `yyparse()` foi substituída pela inclusão de `parser.tab.h`, para
usar a interface gerada pelo Bison. O arquivo aberto por `fopen` agora é
fechado após `yyparse()`, tanto na aceitação quanto na rejeição. A entrada
padrão não é fechada explicitamente. As regras do lexer foram preservadas.

## Fluxo conferido

1. `main` associa o arquivo informado a `yyin`, ou utiliza a entrada padrão.
2. `main` chama `yyparse()`, declarado no cabeçalho gerado pelo Bison.
3. O parser solicita tokens chamando `yylex()`.
4. O lexer inclui o mesmo cabeçalho e retorna os tokens esperados pelo parser.
5. O Flex contabiliza linhas com `yylineno` e erros com `erros_lexicos`.
6. `yyerror()` relata erros sintáticos usando a linha de detecção.
7. `main` aceita somente quando `yyparse()` retorna 0 e `erros_lexicos` é 0.

Erros léxicos podem permitir que a leitura continue, mas impedem a aceitação
final. O parser não implementa recuperação para continuar após erro sintático.
Uma construção ausente pode ser detectada no token ou na linha seguinte.

## Compilação e execução

Comandos executados na raiz, em Linux, com Bison 3.8.2, Flex 2.6.4 e GCC 13.3.0:

```bash
make clean
make build
make test
```

A compilação a partir dos fontes funcionou. Permanecem apenas os avisos de
`input` e `yyunput` não utilizados, provenientes do código gerado pelo Flex.
Não houve alteração no Makefile ou nos arquivos da suíte.

Como `make test` utiliza `|| true` no Linux, os códigos de saída de cada
arquivo foram conferidos separadamente: quatro válidos retornaram 0 e dez
inválidos retornaram 1.

Para reproduzir a execução por arquivo e por redirecionamento:

```bash
./lexer_test tests/validos/exemplo_fatorial.c
echo $?
./lexer_test < tests/validos/exemplo_fatorial.c
echo $?
```

Ambas produziram a mesma saída de aceitação e retorno 0.

| Verificação | Resultado observado |
|---|---|
| Caminho inexistente | Erro de abertura e retorno 1 |
| `tests/invalidos/caracteres_nao_reconhecidos.c` | Erros léxicos, rejeição e retorno 1 |
| `tests/invalidos/string_nao_terminada.c` | String não terminada, rejeição e retorno 1 |
| `int f(){ return 0 }` pela entrada padrão | Erro sintático, rejeição e retorno 1 |
| Entrada padrão vazia | Erro sintático, rejeição e retorno 1 |
| `int f(){ return @ 0; }` pela entrada padrão | Erro léxico e retorno 1, mesmo com sintaxe restante válida |

Exemplo para conferir propagação de erro léxico sem criar arquivo de teste:

```bash
printf '%s\n' 'int f(){ return @ 0; }' | ./lexer_test
echo $?
```

## Critério de aceite

O parser compila pelo Bison, consome tokens do Flex e aceita/rejeita estruturas
simples conforme a gramática. Os registros desta issue se limitam à análise
léxica/sintática e à integração; não implementam análise semântica, AST ou
geração de Python, nem substituem o trabalho de testes da issue #8.
