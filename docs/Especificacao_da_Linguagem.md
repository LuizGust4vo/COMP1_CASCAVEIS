# Especificação da Linguagem

## 1. Objetivo

Este documento define o subconjunto da linguagem C suportado pelo compilador C para Python desenvolvido pela Equipe 05 - Cascaveis na disciplina de Compiladores 1.

A especificação apresenta as construções da linguagem suportadas, os tokens reconhecidos, as limitações do compilador e exemplos de código C com suas respectivas traduções esperadas para Python.

---

## 2. Escopo da Linguagem

O compilador suportará um subconjunto da linguagem C, focado em lógica imperativa básica, operações matemáticas, controle de fluxo e funções.

---

### 2.1. Tipos de dados

Serão suportados os seguintes tipos primitivos:

- `int`
- `float`
- `char`
- `void`

---

### 2.2. Estruturas de controle

#### Condicionais

- `if`
- `else`

#### Estruturas de repetição

- `while`
- `for`

---

### 2.3. Funções

O compilador deverá suportar:

- Declaração e definição de funções;
- Chamada de funções;
- Retorno de valores utilizando `return`.

---

### 2.4. Operadores

#### Aritméticos

- `+` — soma
- `-` — subtração
- `*` — multiplicação
- `/` — divisão

#### Atribuição

- `=` — atribuição simples

#### Relacionais

- `==` — igualdade
- `!=` — diferença
- `<` — menor
- `>` — maior
- `<=` — menor ou igual
- `>=` — maior ou igual

#### Lógicos

- `&&` — E lógico
- `||` — OU lógico
- `!` — NÃO lógico

---

### 2.5. Literais

Serão reconhecidos:

- números inteiros em base decimal;
- números de ponto flutuante;
- caracteres literais;
- strings literais.

Exemplos:

```c
10
3.14
'x'
"Hello World"
```

---

### 2.6. Identificadores

Nomes de variáveis e funções devem seguir o padrão:

```text
[a-zA-Z_][a-zA-Z0-9_]*
```
---

### 2.7. Delimitadores de bloco e expressão

- `{ }` — chaves, delimitam blocos de código
- `( )` — parênteses, delimitam expressões, condições, parâmetros e chamadas de funções
- `,` — vírgula, separa parâmetros e argumentos
- `;` — ponto e vírgula, encerra instruções

---

### 2.8. Comentários

Serão suportados:

- `// comentário` — comentário de linha única
- `/* ... */` — comentário em bloco 

---

## 3. Especificação Léxica e Tokens

Esta seção define os tokens reconhecidos pelo analisador léxico, com base no escopo definido na seção 2. Os nomes de token utilizados aqui correspondem diretamente aos símbolos declarados em [`src/parser.y`](../src/parser.y) e reconhecidos em [`src/lexer.l`](../src/lexer.l).

### 3.1. Palavras reservadas

| Token | Lexema | Descrição |
|-------|--------|-----------|
| `INT` | `int` | tipo inteiro |
| `FLOAT` | `float` | tipo ponto flutuante |
| `CHAR` | `char` | tipo caractere |
| `VOID` | `void` | ausência de tipo de retorno |
| `IF` | `if` | condicional |
| `ELSE` | `else` | ramo alternativo do condicional |
| `WHILE` | `while` | laço de repetição com teste no início |
| `FOR` | `for` | laço de repetição com inicialização/atualização |
| `RETURN` | `return` | retorno de valor de função |

Palavras reservadas têm prioridade sobre a regra de identificadores: no analisador léxico, as regras das palavras-chave são verificadas antes da regra genérica de identificador, de modo que `int`, por exemplo, nunca é lido como identificador.

### 3.2. Identificadores

| Token | Expressão regular | Exemplo |
|-------|--------------------|---------|
| `ID` | `[a-zA-Z_][a-zA-Z0-9_]*` | `total`, `_aux`, `soma2` |

### 3.3. Literais

| Token | Expressão regular | Exemplo |
|-------|--------------------|---------|
| `NUM_INT` | `[0-9]+` | `10`, `0`, `42` |
| `NUM_FLOAT` | `[0-9]+\.[0-9]+` | `3.14`, `0.5` |
| `CHAR_LIT` | `'([^'\\\n]|\\.)'` | `'x'`, `'\n'` |
| `STRING_LIT` | delimitado por `"..."`, tratado com estado exclusivo no Flex (`STRING_STATE`) para permitir detectar strings não terminadas | `"Hello World"` |

Strings são tratadas por um estado exclusivo do Flex em vez de uma única expressão regular: ao encontrar `"` o analisador entra no estado `STRING_STATE` e permanece nele até encontrar outra `"` (fecha o token), uma quebra de linha ou o fim do arquivo (reporta erro léxico de "string não terminada" com a linha em que a string foi aberta). Essa abordagem é o que permite validar o caso de teste [`string_nao_terminada.c`](../tests/invalidos/string_nao_terminada.c).

### 3.4. Operadores

| Token | Lexema | Categoria |
|-------|--------|-----------|
| `PLUS` | `+` | aritmético |
| `MINUS` | `-` | aritmético |
| `TIMES` | `*` | aritmético |
| `DIVIDE` | `/` | aritmético |
| `ASSIGN` | `=` | atribuição |
| `EQ` | `==` | relacional |
| `NE` | `!=` | relacional |
| `LT` | `<` | relacional |
| `GT` | `>` | relacional |
| `LE` | `<=` | relacional |
| `GE` | `>=` | relacional |
| `AND` | `&&` | lógico |
| `OR` | `\|\|` | lógico |
| `NOT` | `!` | lógico |

Os operadores de dois caracteres (`==`, `!=`, `<=`, `>=`, `&&`, `\|\|`) são declarados no `.l` antes dos operadores de um caractere para que o Flex sempre prefira o casamento mais longo (ex.: `==` nunca é lido como dois `ASSIGN`).

### 3.5. Delimitadores

| Token | Lexema |
|-------|--------|
| `LBRACE` | `{` |
| `RBRACE` | `}` |
| `LPAREN` | `(` |
| `RPAREN` | `)` |
| `COMMA` | `,` |
| `SEMI` | `;` |

### 3.6. Comentários e espaços em branco

Comentários e espaços em branco são consumidos pelo analisador léxico e não geram tokens:

- `// ...` até o fim da linha (comentário de linha única);
- `/* ... */`, tratado com um estado exclusivo do Flex (`COMENTARIO`) que suporta múltiplas linhas e reporta erro léxico caso o comentário não seja fechado antes do fim do arquivo;
- espaços, tabulações, `\r` e quebras de linha (`[ \t\r\n]+`).

### 3.7. Caracteres não reconhecidos

Qualquer caractere que não corresponda a nenhuma das regras anteriores é tratado por uma regra de captura (`.`) no final do arquivo `.l`, que reporta o caractere e a linha em um erro léxico (`Erro lexico: caractere nao reconhecido ...`) e incrementa um contador global de erros léxicos, sem interromper a leitura do restante do arquivo. Isso cobre o caso de teste [`caracteres_nao_reconhecidos.c`](../tests/invalidos/caracteres_nao_reconhecidos.c).

### 3.8. Verificação de viabilidade no Flex/Bison

Um protótipo funcional foi implementado em [`src/lexer.l`](../src/lexer.l) (Flex) e [`src/parser.y`](../src/parser.y) (Bison), com ponto de entrada em [`src/main.c`](../src/main.c), e integrado ao `Makefile` do projeto (`make build`, `make test-validos`, `make test-invalidos`).

Resultado da verificação:

- **Conflitos de gramática:** `bison -d -v` não reportou nenhum conflito shift/reduce ou reduce/reduce. Em particular, a gramática não sofre da ambiguidade clássica do "dangling else", pois blocos (`bloco`) exigem chaves obrigatórias após `if`/`else`/`while`/`for` — não existe a forma "comando simples sem chaves" que causa esse conflito em outras gramáticas de C.
- **Build:** o pipeline Flex → Bison → gcc compila sem erros (`make build`); os únicos avisos são sobre funções internas não utilizadas geradas automaticamente pelo Flex (`input`, `yyunput`), que não afetam o funcionamento do analisador.
- **Caso válido:** o exemplo combinado da seção 5.8 (`fatorial` + `principal`, reaproveitado em [`tests/validos/exemplo_fatorial.c`](../tests/validos/exemplo_fatorial.c)) é aceito pelo analisador (`make test-validos`).
- **Casos inválidos:** os cinco casos de teste em `tests/invalidos/` (seção 6) são corretamente rejeitados (`make test-invalidos`), com mensagens de erro indicando a linha correta. Testes adicionais colocando as mesmas construções inválidas dentro do corpo de uma função confirmaram que:
  - caracteres não reconhecidos (`@`) geram erro léxico na linha correta antes do erro sintático subsequente;
  - operadores não suportados (`+=`) geram erro sintático na linha correta;
  - strings não terminadas geram erro léxico apontando a linha em que a string foi aberta;
  - comentários de bloco não terminados geram erro léxico apontando a linha em que o erro foi detectado.

Conclusão: a especificação léxica é viável de ser implementada em Flex/Bison dentro do escopo definido pela equipe, sem necessidade de revisar as decisões de escopo já tomadas.

---

## 4. Regras Sintáticas

Esta seção define como os elementos léxicos podem ser combinados para formar construções válidas da linguagem. As regras apresentadas utilizam uma notação simplificada de gramática e servirão como referência para a implementação do analisador sintático no Bison.

---

### 4.1. Estrutura do programa

Um programa é formado por uma ou mais funções.

```text
programa ::= funcao
           | programa funcao
```

Uma função possui tipo de retorno, identificador e parâmetros opcionais, podendo ser declarada ou definida.

```text
tipo ::= "int"
       | "float"
       | "char"

tipo_retorno ::= tipo
               | "void"

declaracao_funcao ::= tipo_retorno identificador "(" parametros_opcionais ")" ";"

definicao_funcao ::= tipo_retorno identificador "(" parametros_opcionais ")" bloco

funcao ::= declaracao_funcao
         | definicao_funcao
```

---

### 4.2. Blocos e comandos

Blocos são delimitados por chaves e podem conter zero ou mais comandos.

```text
bloco ::= "{" comandos "}"

comandos ::= vazio
           | comandos comando
```

Os comandos reconhecidos pela gramática são:

```text
comando ::= declaracao
          | atribuicao
          | chamada_funcao ";"
          | condicional
          | repeticao
          | retorno
```

---

### 4.3. Declarações e atribuições

Declarações poderão ocorrer com ou sem inicialização.

```text
declaracao ::= tipo identificador ";"
             | tipo identificador "=" expressao ";"
```

Atribuições modificam o valor associado a um identificador.

```text
atribuicao ::= identificador "=" expressao ";"
```

---

### 4.4. Estruturas de controle

As estruturas condicionais seguem as formas:

```text
condicional ::= "if" "(" expressao ")" bloco
              | "if" "(" expressao ")" bloco "else" bloco
```

As estruturas de repetição seguem as formas:

```text
repeticao ::= "while" "(" expressao ")" bloco
            | "for" "(" inicializacao_for ";" expressao ";" atualizacao_for ")" bloco

inicializacao_for ::= tipo identificador "=" expressao
                    | identificador "=" expressao

atualizacao_for ::= identificador "=" expressao
```

---

### 4.5. Expressões

Expressões podem ser formadas por literais, identificadores, chamadas de função, expressões entre parênteses e operações entre expressões.

```text
expressao ::= literal
            | identificador
            | chamada_funcao
            | "(" expressao ")"
            | "-" expressao
            | "!" expressao
            | expressao operador_binario expressao
```

A precedência dos operadores deverá ser considerada pelo analisador sintático para evitar interpretações ambíguas. Da maior para a menor precedência:

```text
! - (unário)
* /
+ -
< <= > >=
== !=
&&
||
```

Parênteses podem ser utilizados para alterar explicitamente a ordem de avaliação.

---

### 4.6. Parâmetros e argumentos

Os parâmetros de uma função são opcionais e separados por vírgulas.

```text
parametros_opcionais ::= vazio
                       | parametros

parametros ::= parametro
             | parametros "," parametro

parametro ::= tipo identificador
```

De forma semelhante, chamadas de função podem possuir zero ou mais argumentos.

```text
chamada_funcao ::= identificador "(" argumentos_opcionais ")"

argumentos_opcionais ::= vazio
                       | argumentos

argumentos ::= expressao
             | argumentos "," expressao
```

---

### 4.7. Retorno de funções

O comando de retorno é representado por:

```text
retorno ::= "return" expressao ";"
```

Funções que não retornam valor não exigem a presença desse comando.

---

### 4.8. Relação com o analisador sintático

O Flex será responsável por transformar o código-fonte em uma sequência de tokens. O Bison utilizará essa sequência para verificar se as construções obedecem às regras sintáticas definidas nesta seção.

As regras aqui apresentadas são uma especificação da sintaxe aceita pelo compilador e poderão ser refinadas durante a implementação da gramática em `parser.y`.

---

## 5. Exemplos de Tradução C → Python

Esta parte cobre exemplos mínimos e combinados para cada construção definida no escopo da linguagem fonte. Cada exemplo traz: código C, tradução Python esperada, e observações sobre decisões de tradução.

---

### 5.1. Tipos de Dados Primitivos

#### 5.1.1. Declaração e inicialização

**C**
```c
int a = 10;
float b = 3.14;
char c = 'x';
```

**Python**
```python
a = 10
b = 3.14
c = 'x'
```

**Observações:** Python não tem declaração de tipo. A informação de tipo do C pode ser descartada na tradução ou preservada como type hint (`a: int = 10`), dependendo da decisão do grupo sobre geração de código. Aqui assumimos descarte simples.

#### 5.1.2. `void` em funções

**C**
```c
void imprime_nada() {
    int x = 5;
}
```

**Python**
```python
def imprime_nada():
    x = 5
```

**Observações:** `void` não tem equivalente direto, a ausência de `return` em Python já implica retorno `None`, então o tipo simplesmente é omitido na assinatura.

---

### 5.2. Estruturas Condicionais (`if`, `else`)

**C**
```c
int classifica(int n) {
    if (n > 0) {
        return 1;
    } else {
        return -1;
    }
}
```

**Python**
```python
def classifica(n):
    if n > 0:
        return 1
    else:
        return -1
```

**Observações:** Tradução direta. `{}` viram indentação + `:`. Parênteses na condição são opcionais em Python, mas podem ser mantidos sem prejuízo (`if (n > 0):` também é válido).

#### 5.2.1. `if` sem `else`

**C**
```c
int checa(int x) {
    if (x == 0) {
        return 100;
    }
    return 0;
}
```

**Python**
```python
def checa(x):
    if x == 0:
        return 100
    return 0
```

---

### 5.3. Laços de Repetição

#### 5.3.1. `while`

**C**
```c
int soma_ate(int n) {
    int total = 0;
    int i = 1;
    while (i <= n) {
        total = total + i;
        i = i + 1;
    }
    return total;
}
```

**Python**
```python
def soma_ate(n):
    total = 0
    i = 1
    while i <= n:
        total = total + i
        i = i + 1
    return total
```

**Observações:** Mapeamento 1:1. Nenhuma armadilha aqui, já que o escopo não inclui `++`/`--` nem operadores compostos (`+=`), então tudo fica explícito com `=`.

#### 5.3.2. `for` — regra geral de tradução: sempre vira `while`


**C**
```c
int soma_array_simulada(int n) {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        total = total + i;
    }
    return total;
}
```

**Python**
```python
def soma_array_simulada(n):
    total = 0
    i = 0
    while i < n:
        total = total + i
        i = i + 1
    return total
```

#### 5.3.3. `for` — caso com passo diferente de 1

**C**
```c
int conta_regressiva(int n) {
    int passos = 0;
    for (int i = n; i > 0; i = i - 2) {
        passos = passos + 1;
    }
    return passos;
}
```

**Python**
```python
def conta_regressiva(n):
    passos = 0
    i = n
    while i > 0:
        passos = passos + 1
        i = i - 2
    return passos
```

**Observações:** Como a regra é sempre `for → while`, esse caso é tratado exatamente igual ao anterior — não exige nenhum reconhecimento especial de padrão. Essa é a vantagem da regra escolhida: um único algoritmo de tradução cobre qualquer `for`, incluindo passos negativos, condições compostas, ou `step` que não seja uma constante simples.

---

### 5.4. Funções

#### 5.4.1. Declaração, parâmetros e `return`

**C**
```c
int soma(int a, int b) {
    return a + b;
}
```

**Python**
```python
def soma(a, b):
    return a + b
```

#### 5.4.2. Chamada de função

**C**
```c
int quadrado(int x) {
    return x * x;
}

int principal() {
    int r = quadrado(5);
    return r;
}
```

**Python**
```python
def quadrado(x):
    return x * x

def principal():
    r = quadrado(5)
    return r
```

#### 5.4.3. Função `void` sem `return`

**C**
```c
void nao_faz_nada() {
    int x = 1;
    int y = 2;
}
```

**Python**
```python
def nao_faz_nada():
    x = 1
    y = 2
```

---

### 5.5. Operadores

#### 5.5.1. Aritméticos

**C**
```c
int calcula(int a, int b) {
    int soma = a + b;
    int sub = a - b;
    int mul = a * b;
    int div = a / b;
    return div;
}
```

**Python**
```python
def calcula(a, b):
    soma = a + b
    sub = a - b
    mul = a * b
    div = a / b
    return div
```

**Observações — ponto de atenção crítico:** em C, `int / int` faz divisão inteira (trunca). Em Python, `/` **sempre** retorna `float`. Se `a` e `b` forem `int` no C original, a tradução correta para preservar o comportamento é usar `//` (divisão inteira) em Python:

```python
div = a // b
```

O grupo precisa decidir: o compilador vai analisar os tipos das variáveis (via tabela de símbolos) para escolher entre `/` e `//`? Essa é provavelmente a decisão de tradução mais importante do escopo definido, porque afeta a corretude de qualquer programa que divida inteiros.

#### 5.5.2. Atribuição

**C**
```c
int x = 5;
x = 10;
```

**Python**
```python
x = 5
x = 10
```

#### 5.5.3. Relacionais

**C**
```c
int compara(int a, int b) {
    if (a == b) { return 0; }
    if (a != b) { return 1; }
    if (a < b) { return 2; }
    if (a > b) { return 3; }
    if (a <= b) { return 4; }
    if (a >= b) { return 5; }
    return -1;
}
```

**Python**
```python
def compara(a, b):
    if a == b:
        return 0
    if a != b:
        return 1
    if a < b:
        return 2
    if a > b:
        return 3
    if a <= b:
        return 4
    if a >= b:
        return 5
    return -1
```

**Observações:** Todos os operadores relacionais de C têm símbolo idêntico em Python — tradução puramente sintática, sem mudança semântica.

#### 5.5.4. Lógicos

**C**
```c
int logica(int a, int b) {
    if (a > 0 && b > 0) {
        return 1;
    }
    if (a > 0 || b > 0) {
        return 2;
    }
    if (!(a > 0)) {
        return 3;
    }
    return 0;
}
```

**Python**
```python
def logica(a, b):
    if a > 0 and b > 0:
        return 1
    if a > 0 or b > 0:
        return 2
    if not (a > 0):
        return 3
    return 0
```

**Observações:** `&&` → `and`, `||` → `or`, `!` → `not`. Semântica idêntica para os tipos do escopo (sem ponteiros, sem structs), incluindo curto-circuito (short-circuit evaluation), que se comporta igual nas duas linguagens.

---

### 5.6. Literais

**C**
```c
int i = 42;
float f = 3.14;
char* s = "texto";
```

**Python**
```python
i = 42
f = 3.14
s = "texto"
```

**Observações:** Strings literais em C são tecnicamente `char*` (arrays terminados em `\0`); em Python são o tipo nativo `str`. Como o escopo não inclui manipulação de ponteiros nem operações sobre arrays de char, a tradução direta funciona bem, mas vale registrar que semanticamente são representações bem diferentes por baixo dos panos.

---

### 5.7. Comentários

**C**
```c
// comentário de linha
int x = 1; // comentário no fim da linha

/* comentário
   em bloco */
int y = 2;
```

**Python**
```python
# comentário de linha
x = 1  # comentário no fim da linha

# comentário
# em bloco
y = 2
```

**Observações:** Python não tem comentário de bloco nativo equivalente a `/* */` (o mais próximo, strings de documentação com `"""`, tem semântica diferente — é uma string literal, não um comentário puro). A tradução mais segura é converter comentários de bloco em múltiplas linhas de `#`.

---

### 5.8. Exemplo Combinado (teste de integração)

**C**
```c
int fatorial(int n) {
    int resultado = 1;
    int i = 1;
    while (i <= n) {
        resultado = resultado * i;
        i = i + 1;
    }
    return resultado;
}

int principal() {
    int x = 5;
    if (x > 0 && x < 10) {
        return fatorial(x);
    } else {
        return -1;
    }
}
```

**Python**
```python
def fatorial(n):
    resultado = 1
    i = 1
    while i <= n:
        resultado = resultado * i
        i = i + 1
    return resultado

def principal():
    x = 5
    if x > 0 and x < 10:
        return fatorial(x)
    else:
        return -1
```

---

## 6. Construções Não Suportadas

O compilador implementará apenas o subconjunto da linguagem C definido nesta especificação. Construções fora desse escopo e entradas que violem as regras léxicas estabelecidas deverão ser identificadas como não suportadas ou inválidas.

Para validar essas limitações, foram definidos casos de teste em [`tests/invalidos/`](../tests/invalidos/).

---

### 6.1. Diretivas do pré-processador

Diretivas do pré-processador C, como `#include` e `#define`, não fazem parte do escopo do compilador.

Exemplo:

```c
#include <stdio.h>
#define PI 3.14
```

Caso de teste: [`diretivas_nao_suportadas.c`](../tests/invalidos/diretivas_nao_suportadas.c)

---

### 6.2. Operadores não suportados

Apenas os operadores definidos no escopo da linguagem serão reconhecidos. Operadores de atribuição composta, incremento e decremento, operadores bit a bit e operadores de deslocamento não serão suportados nesta versão.

Exemplos:

```c
a += 2;
a++;
a & 1;
a << 2;
```

Caso de teste: [`operadores_nao_suportados.c`](../tests/invalidos/operadores_nao_suportados.c)

---

### 6.3. Identificadores inválidos

Identificadores devem seguir a expressão regular:

```text
[a-zA-Z_][a-zA-Z0-9_]*
```

Portanto, identificadores iniciados por números serão considerados inválidos.

Exemplos:

```c
int 123variavel = 10;
void 99funcao() {}
```

Caso de teste: [`identificador_invalido.c`](../tests/invalidos/identificador_invalido.c)

---

### 6.4. Caracteres não reconhecidos

Caracteres que não façam parte dos tokens definidos pela linguagem deverão ser reportados como inválidos pelo analisador léxico.

Exemplos:

```c
int x = @;
char c = $;
float y = ~5.0;
```

Caso de teste: [`caracteres_nao_reconhecidos.c`](../tests/invalidos/caracteres_nao_reconhecidos.c)

---

### 6.5. Strings não terminadas

Strings devem possuir aspas de abertura e fechamento. Uma cadeia iniciada com `"` e não terminada corretamente será considerada uma entrada inválida.

Exemplo:

```c
"texto sem fechar aspas;
```

Caso de teste: [`string_nao_terminada.c`](../tests/invalidos/string_nao_terminada.c)

---

### 6.6. Comportamento esperado

Os arquivos presentes em `tests/invalidos/` representam entradas que não devem ser aceitas normalmente pelo compilador. Durante as etapas de análise léxica e sintática, espera-se que esses casos resultem na identificação e no relato adequado do erro, em vez de serem processados como programas válidos.

---

## 7. Histórico de Versões

| Versão | Data | Autor(es) | Revisor(es) | Descrição |
|--------|------|-----------|-------------|-----------|
| 1.0 | 23/08/2026 | Gabriel Goldenberg | - | Criação do documento |
| 1.1 | 28/08/2026 | Pedro Araujo | Luiz Almeida | Adicionou escopo da linguagem |
| 1.2 | 29/08/2026 | Ana Caroline | - | Desenvolvimento dos exemplos de tradução C → Python |
| 1.3 | 29/08/2026 | Gabriel Goldenberg | Luiz Almeida | Desenvolvimento das construções não suportadas |
| 1.4 | 29/08/2026 | Luiz Almeida | - | Desenvolvimento das regras sintáticas e organização do documento |
| 1.5 | 30/08/2026 | Felipe | - | Especificação léxica e tokens; protótipo Flex/Bison e verificação de viabilidade |

---
