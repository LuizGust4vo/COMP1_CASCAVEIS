# Representação Intermediária (RI) do Compilador C → Python


## 1. Decisão

A RI adotada é o **código de três endereços (CTE)** em forma **linear**. Em memória ele é representado por **quádruplas** `(op, arg1, arg2, result)` e, em texto, é exibido com uma instrução por linha. Cada instrução tem no máximo um operador, o que torna a geração a partir da AST direta (percurso em pós-ordem) e facilita as otimizações da Issue 5.2 e a geração do Python final (Issue 5.1).

A RI cobre todo o subconjunto aceito pelo `parser.y`: tipos `int`, `float`, `char` e `void`; operadores aritméticos, relacionais e lógicos (incluindo `-` e `!` unários); declarações e atribuições; `if/else`; `while`; `for`; funções (com protótipos), chamadas e `return`; e a função nativa `printf`, que só recebe literais de string como argumento direto (seção 7.1.3 da especificação). Há um pré-requisito na seção 9: hoje o parser reconhece `while` e `for`, mas não os coloca na AST.

## 2. Elementos da RI

### 2.1 Operandos

Um operando é um **identificador** (variável ou parâmetro do código-fonte), um **temporário** ou uma **constante**. Constantes: inteiro (`42`), decimal (`3.14`), char (`'a'`) e string (`"texto"`). Strings só aparecem como argumento direto de chamada (`param "texto"`).

### 2.2 Temporários

- Formato `t<n>`: `t1`, `t2`, `t3`, …
- Cada temporário recebe valor uma única vez, exceto o que guarda o resultado de `&&` e `||` (seção 6), atribuído nos dois ramos do operador.
- Todo resultado intermediário vai para um temporário novo; não há reaproveitamento.
- A numeração segue a ordem de criação, reinicia no início de cada função e pula qualquer `t<n>` que já seja nome de variável ou parâmetro da função (identificadores como `t1` são válidos na linguagem).

### 2.3 Labels

- Formato `L<n>`: `L1`, `L2`, …, escritos em linha própria (`L1:`).
- Os labels de um construto são numerados quando o construto é iniciado (pré-ordem), antes de gerar suas partes internas. Isso deixa a saída determinística para os testes da Issue 4.4.
- O contador reinicia a cada função, e um salto nunca sai da função em que está.

### 2.4 Nomes de variáveis

Variáveis e parâmetros mantêm o nome do código-fonte, sem renomeação. A análise semântica já rejeita nomes repetidos em escopos aninhados (sombreamento), nomes iguais aos de funções e nomes reservados do Python (regras 4, 5, 8 e 13 da seção 7.2.3 da especificação), então cada nome designa uma única variável por função, que é o que o Python exige. O nome vem de `AstNo.texto` e o tipo, de `AstNo.simbolo`.

## 3. Conjunto de instruções

| Instrução | Descrição |
| --- | --- |
| `decl <tipo> x` | Declaração sem inicializador (`int x;`). Declarações com inicializador não geram `decl`: só a atribuição. |
| `x = y` | Cópia ou atribuição; `y` é um operando. |
| `x = y <op> z` | Operação binária aritmética ou relacional (operadores na seção 4). |
| `x = -y` e `x = !y` | Operação unária: negação aritmética e negação lógica. |
| `L:` | Define o label `L`. |
| `goto L` | Salto incondicional para `L`. |
| `ifFalse y goto L` | Salta para `L` se `y` for falso (0). |
| `if y goto L` | Salta para `L` se `y` for verdadeiro (diferente de 0). |
| `func f(<tipo> p1, ...) -> <tipo>` | Início da função `f`. Protótipos (`int f(int a);`) não geram código. |
| `endfunc` | Fim da função. |
| `param y` | Registra o argumento `y` para a próxima chamada. Constantes de string só aparecem aqui. |
| `x = call f, n` | Chama `f` (inclusive `printf`) com os `n` últimos `param`; o resultado vai para `x`. |
| `call f, n` | Chamada sem uso do resultado (funções `void` e `printf`). |
| `return y` e `return` | Retorno com valor e sem valor. |

## 4. Convenções

**Operadores.**

- Aritméticos: `+`, `-`, `*`, `/` (divisão real), `//` (divisão inteira)
- Relacionais: `==`, `!=`, `<`, `>`, `<=`, `>=` (resultado 1 ou 0)
- Unários: `-` (aritmético) e `!` (lógico)
- `&&` e `||` não são instruções: são gerados com saltos (seção 6), para manter o curto-circuito que a especificação garante na seção 5.5.4.

**Divisão.** O gerador escolhe `//` quando os dois operandos são `int` ou `char` e `/` quando algum é `float` (tabela da seção 7.1.1 da especificação). Isso é necessário porque o `/` do Python sempre devolve `float`.

**Atribuição.** Se o lado direito é só um operando, a cópia é direta (`a = 2`). Se é uma expressão, o resultado vai para um temporário e depois é copiado para a variável (`x = t4`). Eliminar essas cópias é trabalho da Issue 5.2.

**Declaração.** `int x;` gera `decl int x`. `int x = e;` gera só a atribuição de `e` a `x`, sem `decl`.

**Saltos.** Toda condição (de `if`, `while` e `for`) é avaliada até virar um operando, que é testado com `ifFalse`. Todo salto referencia um label da mesma função. O gerador só emite os padrões da seção 6.

**Chamadas.** Os argumentos são avaliados da esquerda para a direita, cada um até virar um operando. Só depois são emitidos os `param`, na mesma ordem, imediatamente antes do `call`, de modo que não há outra chamada entre eles. O `n` do `call` é o número de argumentos. Chamadas de funções `void` e de `printf` usam a forma `call f, n`, sem atribuição.

**Funções.** Os parâmetros são declarados no cabeçalho `func`. Funções não `void` retornam com `return <operando>`.

**`for`.** Não tem instrução própria: é gerado na forma de `while` (seção 6), coerente com a decisão do grupo de traduzir todo `for` para `while`.

## 5. Estrutura em memória (para a Issue 4.3)

```c
typedef enum {
    IR_SOMA, IR_SUB, IR_MUL, IR_DIV, IR_DIV_INT,
    IR_MENOR, IR_MAIOR, IR_MENOR_IGUAL, IR_MAIOR_IGUAL, IR_IGUAL, IR_DIFERENTE,
    IR_NEGATIVO, IR_NAO,
    IR_ATRIB, IR_DECL,
    IR_LABEL, IR_GOTO, IR_IF, IR_IF_FALSE,
    IR_FUNC, IR_FPARAM, IR_FIM_FUNC,
    IR_PARAM, IR_CALL, IR_RETURN
} IrOp;

typedef struct Quadrupla {
    IrOp op;
    char *arg1;
    char *arg2;
    char *resultado;
    struct Quadrupla *proximo;
} Quadrupla;
```

O prefixo `IR_` evita colisão com `OP_SOMA`, `OP_SUB`, `OP_MUL` e `OP_DIV`, já definidos em `ast.h`. A correspondência com `AstOperador` é direta (`OP_SOMA` → `IR_SOMA`, `OP_MENOR` → `IR_MENOR`, `OP_NEGATIVO` → `IR_NEGATIVO`, `OP_NAO` → `IR_NAO`); `OP_DIV` vira `IR_DIV` ou `IR_DIV_INT` conforme os tipos, e `OP_E` e `OP_OU` não têm instrução própria porque viram saltos. Os campos são strings (nomes, temporários ou literais) e os que não se aplicam ficam `NULL`.

| Forma textual | op | arg1 | arg2 | resultado |
| --- | --- | --- | --- | --- |
| `x = y <op> z` | `IR_SOMA` … `IR_DIFERENTE` | y | z | x |
| `x = -y` / `x = !y` | `IR_NEGATIVO` / `IR_NAO` | y | — | x |
| `x = y` | `IR_ATRIB` | y | — | x |
| `decl T x` | `IR_DECL` | T | — | x |
| `L:` | `IR_LABEL` | — | — | L |
| `goto L` | `IR_GOTO` | — | — | L |
| `ifFalse y goto L` / `if y goto L` | `IR_IF_FALSE` / `IR_IF` | y | — | L |
| `func f(...) -> T` | `IR_FUNC` | T | — | f |
| parâmetro formal `T p` | `IR_FPARAM` | T | — | p |
| `endfunc` | `IR_FIM_FUNC` | — | — | — |
| `param y` | `IR_PARAM` | y | — | — |
| `x = call f, n` | `IR_CALL` | f | n | x (vazio em `void` e `printf`) |
| `return y` | `IR_RETURN` | y (ou vazio) | — | — |

O cabeçalho `func` com parâmetros é guardado como um `IR_FUNC` seguido de um `IR_FPARAM` por parâmetro. A impressão em texto (formato da seção 3) deve ficar em uma única função de dump, usada pelos testes da Issue 4.4.

## 6. Padrões de geração

`<cond>` é o código que avalia a condição e deixa o resultado no operando `c`; `<a>` e `<b>` são os códigos das duas metades de `&&` e `||`, com resultados nos operandos `a` e `b`. Em `&&` e `||`, o texto depois de `;` é só explicação e não faz parte da RI.

**if**

```
<cond>
ifFalse c goto L1
<then>
L1:
```

**if / else** (a gramática exige chaves no `else`, então um `else if` é um `if` dentro de `else { ... }`)

```
<cond>
ifFalse c goto L1
<then>
goto L2
L1:
<else>
L2:
```

**while**

```
L1:
<cond>
ifFalse c goto L2
<corpo>
goto L1
L2:
```

**for (init; cond; passo)**

```
<init>
L1:
<cond>
ifFalse c goto L2
<corpo>
<passo>
goto L1
L2:
```

**a && b** (`r` é o próprio temporário de `a` quando `a` já é um temporário; senão, um temporário novo)

```
<a>
r = a                 ; omitido quando r é o temporário de a
ifFalse r goto L1
<b>
r = b
L1:
```

**a || b**

```
<a>
r = a                 ; omitido quando r é o temporário de a
if r goto L1
<b>
r = b
L1:
```

O label de um `&&` ou `||` é criado quando o operador é visitado, depois dos labels do `if` ou `while` que o contém.

**chamada**

```
<avalia os argumentos em a1 ... an>
param a1
...
param an
x = call f, n
```

## 7. Exemplos (C → RI)

### 7.1 Expressões

```c
x = a + b * c - d / e;     // todos int
```

```
t1 = b * c
t2 = a + t1
t3 = d // e
t4 = t2 - t3
x = t4
```

```c
float f = 2.5 * x + 1;     // x é float
```

```
t1 = 2.5 * x
t2 = t1 + 1
f = t2
```

```c
r = (a < b) && !(c == d);
```

```
t1 = a < b
ifFalse t1 goto L1
t2 = c == d
t3 = !t2
t1 = t3
L1:
r = t1
```

```c
r = a > 0 || b > 0;
```

```
t1 = a > 0
if t1 goto L1
t2 = b > 0
t1 = t2
L1:
r = t1
```

```c
y = -a * 2;
```

```
t1 = -a
t2 = t1 * 2
y = t2
```

### 7.2 if / else

```c
if (a > b) { m = a; } else { m = b; }
```

```
t1 = a > b
ifFalse t1 goto L1
m = a
goto L2
L1:
m = b
L2:
```

```c
if (x == 0) { y = 1; }
```

```
t1 = x == 0
ifFalse t1 goto L1
y = 1
L1:
```

```c
if (x > 0 && x < 10) { y = 1; }
```

```
t1 = x > 0
ifFalse t1 goto L2
t2 = x < 10
t1 = t2
L2:
ifFalse t1 goto L1
y = 1
L1:
```

```c
if (x < 0) { s = 1; } else { if (x == 0) { s = 2; } else { s = 3; } }
```

```
t1 = x < 0
ifFalse t1 goto L1
s = 1
goto L2
L1:
t2 = x == 0
ifFalse t2 goto L3
s = 2
goto L4
L3:
s = 3
L4:
L2:
```

### 7.3 while e for

```c
while (i < n) { s = s + i; i = i + 1; }
```

```
L1:
t1 = i < n
ifFalse t1 goto L2
t2 = s + i
s = t2
t3 = i + 1
i = t3
goto L1
L2:
```

```c
for (int i = 0; i < n; i = i + 1) { s = s + i; }
```

```
i = 0
L1:
t1 = i < n
ifFalse t1 goto L2
t2 = s + i
s = t2
t3 = i + 1
i = t3
goto L1
L2:
```

### 7.4 Funções, chamadas e return

```c
int soma(int a, int b) { int r = a + b; return r; }
```

```
func soma(int a, int b) -> int
t1 = a + b
r = t1
return r
endfunc
```

```c
void zera(int x) { x = 0; return; }
```

```
func zera(int x) -> void
x = 0
return
endfunc
```

```c
int main() {
    int a = 2;
    int b = 3;
    int y = soma(a + 1, b * 2);
    zera(a);
    return 0;
}
```

```
func main() -> int
a = 2
b = 3
t1 = a + 1
t2 = b * 2
param t1
param t2
t3 = call soma, 2
y = t3
param a
call zera, 1
return 0
endfunc
```

```c
int fat(int n) {
    if (n <= 1) { return 1; }
    return n * fat(n - 1);
}
```

```
func fat(int n) -> int
t1 = n <= 1
ifFalse t1 goto L1
return 1
L1:
t2 = n - 1
param t2
t3 = call fat, 1
t4 = n * t3
return t4
endfunc
```

### 7.5 Declaração sem inicializador, printf e protótipo

```c
int fat(int n);              // protótipo: não gera nada

void saudacao() {
    int contador;
    contador = 3;
    printf("ola");
}
```

```
func saudacao() -> void
decl int contador
contador = 3
param "ola"
call printf, 1
endfunc
```

## 8. Mapeamento para o Python (diretriz para a Issue 5.1)

- `decl <tipo> x` vira `x = None` (seção 7.2.4 da especificação); declarações com inicializador viram só a atribuição.
- Temporários viram variáveis locais (`t1 = ...`) ou são embutidos na expressão.
- `!` vira `not`; os demais operadores mantêm o símbolo (`//` já é o do Python).
- `func` vira `def f(p1, p2):`; `param` + `call` viram `f(a1, a2)`; `call printf, n` vira `print(a1, ...)`; `return` continua `return`.
- `ifFalse y goto L` com salto só para frente (até o label `L`, sem `goto` de volta) vira `if y:` sobre o trecho até `L`; `if y goto L` vira `if not y:`. Se o trecho termina em `goto` para um label mais adiante, o que vem depois do label intermediário é o `else`.
- Padrão do `while` (label, `ifFalse` para o fim e `goto` de volta ao label inicial) vira `while True:` com a avaliação da condição no início e `if not c: break`. Se a condição é uma expressão sem efeito colateral, pode virar `while <expr>:`.
- `&&` e `||` caem no padrão do `if` acima, o que preserva o curto-circuito sem `goto`; um passe posterior pode juntá-los em `a and b` e `a or b`.
- Como o gerador só emite os padrões da seção 6, a estrutura é reconstruída sem `goto`.

Exemplo completo:

```c
int soma_ate(int n) {
    int s = 0;
    for (int i = 1; i <= n; i = i + 1) { s = s + i; }
    return s;
}
```

```
func soma_ate(int n) -> int
s = 0
i = 1
L1:
t1 = i <= n
ifFalse t1 goto L2
t2 = s + i
s = t2
t3 = i + 1
i = t3
goto L1
L2:
return s
endfunc
```

```python
def soma_ate(n):
    s = 0
    i = 1
    while True:
        t1 = i <= n
        if not t1:
            break
        t2 = s + i
        s = t2
        t3 = i + 1
        i = t3
    return s
```

## 9. Pendências e limitações

1. **A AST ainda não representa `while` e `for` (bloqueia a Issue 4.3).** No `parser.y`, as duas regras de `repeticao` só incrementam `construcoes_sem_ast`, liberam as subárvores e devolvem `NULL`. O `AstNo` tem apenas três filhos (`filho[3]`) e o `for` tem quatro partes. Sugestão, coerente com a regra de traduzir todo `for` para `while`: o parser monta um nó `AST_WHILE` (condição e corpo) e transforma `for (init; cond; passo) corpo` em um bloco `init; while (cond) { corpo; passo }`, sem criar um `AST_FOR`. A variável do `for` continua registrada na tabela de símbolos pelo parser.
2. **Tipo das divisões.** O `semantico.c` calcula o tipo de cada expressão em uma função interna, mas não o grava em `AstNo.tipo_dado`. O gerador precisa dele para escolher `/` ou `//`: é preciso expor essa função ou gravar o tipo no nó.
3. **Valor de `&&` e `||`.** O resultado é o do último operando avaliado, como no `and` e `or` do Python, e não necessariamente 0 ou 1. Em condições não há diferença.
4. **Divisão inteira com negativos.** Em C ela trunca para zero e o `//` do Python arredonda para baixo. Se isso importar, o back-end pode gerar `int(a / b)`.
5. **`-` sobre literal.** `-1` vira `t1 = -1` (um `IR_NEGATIVO`, sem simplificação); simplificar é trabalho da Issue 5.2.
6. **Fora do escopo da linguagem:** variáveis globais, arrays, ponteiros, `break`, `continue`, `%` e conversões explícitas entre `int` e `float`.
