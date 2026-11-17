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

## Como Executar e Testar o Projeto

Esta seção apresenta os comandos necessários para compilar o projeto, executar a suíte de testes e reproduzir exemplos do fluxo do compilador.

> **Ambiente de desenvolvimento:** para instruções de instalação e configuração das dependências no Linux ou Windows, consulte o [Guia de Configuração do Ambiente de Desenvolvimento](docs/guias/Guia%20-%20Configuração%20do%20ambiente%20de%20desenvolvimento.md). No Windows, o guia recomenda a utilização do WSL para facilitar o uso das ferramentas Flex e Bison.

### Compilação

Para limpar os arquivos gerados e compilar o projeto:

```bash
make clean
make build
```

### Testes

Para executar toda a suíte de testes:

```bash
make test
```

Os testes contemplam análise sintática, AST, tabela de símbolos e verificações semânticas iniciais.

### Demonstração do fluxo integrado

Para executar um programa válido exibindo a AST e a tabela de símbolos:

```bash
./lexer_test --ast --simbolos tests/semanticos/validos/declara_e_retorna.c
```

O programa deve ser aceito e apresentar a árvore sintática abstrata e os símbolos cadastrados.

### Casos de erro

* Variável não declarada:

```bash
./lexer_test tests/semanticos/invalidos/variavel_nao_declarada.c
```

* Uso de variável fora do escopo:

```bash
./lexer_test tests/semanticos/invalidos/uso_fora_do_bloco.c
```

* Incompatibilidade de tipos:

```bash
./lexer_test tests/semanticos/invalidos/float_para_int.c
```

Nos três casos, o programa deve ser rejeitado com a mensagem de erro semântico correspondente.

## Licença ©

Este projeto está sob licença [MIT](LICENSE) ©