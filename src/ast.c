/*
 * ast.c - construcao, impressao e liberacao dos nos da AST.
 */

#include <stdlib.h>
#include <string.h>
#include "ast.h"

static int nos_alocados = 0;

static void falta_memoria(void) {
    fprintf(stderr, "Erro interno: memoria insuficiente\n");
    exit(1);
}

static AstNo *novo_no(AstTipoNo tipo, int linha) {
    AstNo *no = calloc(1, sizeof(AstNo));
    if (!no) {
        falta_memoria();
    }
    no->tipo = tipo;
    no->linha = linha;
    nos_alocados++;
    return no;
}

static char *copia_texto(const char *texto) {
    char *copia = malloc(strlen(texto) + 1);
    if (!copia) {
        falta_memoria();
    }
    strcpy(copia, texto);
    return copia;
}

AstNo *ast_programa(AstNo *funcoes) {
    AstNo *no = novo_no(AST_PROGRAMA, 1);
    no->filho[0] = funcoes;
    return no;
}

AstNo *ast_funcao(AstTipoDado retorno, const char *nome, AstNo *parametros, AstNo *corpo, int linha) {
    AstNo *no = novo_no(AST_FUNCAO, linha);
    no->tipo_dado = retorno;
    no->texto = copia_texto(nome);
    no->filho[0] = parametros;
    no->filho[1] = corpo;
    return no;
}

AstNo *ast_parametro(AstTipoDado tipo, const char *nome, int linha) {
    AstNo *no = novo_no(AST_PARAMETRO, linha);
    no->tipo_dado = tipo;
    no->texto = copia_texto(nome);
    return no;
}

AstNo *ast_bloco(AstNo *comandos, int linha) {
    AstNo *no = novo_no(AST_BLOCO, linha);
    no->filho[0] = comandos;
    return no;
}

AstNo *ast_declaracao(AstTipoDado tipo, const char *nome, AstNo *inicializacao, int linha) {
    AstNo *no = novo_no(AST_DECLARACAO, linha);
    no->tipo_dado = tipo;
    no->texto = copia_texto(nome);
    no->filho[0] = inicializacao;
    return no;
}

AstNo *ast_atribuicao(const char *nome, AstNo *valor, int linha) {
    AstNo *no = novo_no(AST_ATRIBUICAO, linha);
    no->texto = copia_texto(nome);
    no->filho[0] = valor;
    return no;
}

AstNo *ast_retorno(AstNo *valor, int linha) {
    AstNo *no = novo_no(AST_RETORNO, linha);
    no->filho[0] = valor;
    return no;
}

AstNo *ast_if(AstNo *condicao, AstNo *entao, AstNo *senao, int linha) {
    AstNo *no = novo_no(AST_IF, linha);
    no->filho[0] = condicao;
    no->filho[1] = entao;
    no->filho[2] = senao;
    return no;
}

AstNo *ast_chamada(const char *nome, AstNo *argumentos, int linha) {
    AstNo *no = novo_no(AST_CHAMADA, linha);
    no->texto = copia_texto(nome);
    no->filho[0] = argumentos;
    return no;
}

AstNo *ast_binario(AstOperador op, AstNo *esquerda, AstNo *direita, int linha) {
    AstNo *no = novo_no(AST_BINARIO, linha);
    no->op = op;
    no->filho[0] = esquerda;
    no->filho[1] = direita;
    return no;
}

AstNo *ast_unario(AstOperador op, AstNo *operando, int linha) {
    AstNo *no = novo_no(AST_UNARIO, linha);
    no->op = op;
    no->filho[0] = operando;
    return no;
}

AstNo *ast_num_int(int valor, int linha) {
    AstNo *no = novo_no(AST_NUM_INT, linha);
    no->valor_int = valor;
    return no;
}

AstNo *ast_num_float(double valor, int linha) {
    AstNo *no = novo_no(AST_NUM_FLOAT, linha);
    no->valor_float = valor;
    return no;
}

AstNo *ast_char_lit(const char *texto, int linha) {
    AstNo *no = novo_no(AST_CHAR_LIT, linha);
    no->texto = copia_texto(texto);
    return no;
}

AstNo *ast_string_lit(const char *texto, int linha) {
    AstNo *no = novo_no(AST_STRING_LIT, linha);
    no->texto = copia_texto(texto);
    return no;
}

AstNo *ast_id(const char *nome, int linha) {
    AstNo *no = novo_no(AST_ID, linha);
    no->texto = copia_texto(nome);
    return no;
}

AstNo *ast_lista_anexa(AstNo *lista, AstNo *item) {
    if (!lista) {
        return item;
    }
    AstNo *ultimo = lista;
    while (ultimo->proximo) {
        ultimo = ultimo->proximo;
    }
    ultimo->proximo = item;
    return lista;
}

void ast_libera(AstNo *no) {
    while (no) {
        AstNo *proximo = no->proximo;
        for (int i = 0; i < 3; i++) {
            ast_libera(no->filho[i]);
        }
        free(no->texto);
        free(no);
        nos_alocados--;
        no = proximo;
    }
}

int ast_nos_alocados(void) {
    return nos_alocados;
}

const char *ast_nome_tipo_dado(AstTipoDado tipo) {
    switch (tipo) {
        case TIPO_INT:   return "int";
        case TIPO_FLOAT: return "float";
        case TIPO_CHAR:  return "char";
        case TIPO_VOID:  return "void";
    }
    return "?";
}

const char *ast_simbolo_operador(AstOperador op) {
    switch (op) {
        case OP_SOMA:        return "+";
        case OP_SUB:         return "-";
        case OP_MUL:         return "*";
        case OP_DIV:         return "/";
        case OP_MENOR:       return "<";
        case OP_MAIOR:       return ">";
        case OP_MENOR_IGUAL: return "<=";
        case OP_MAIOR_IGUAL: return ">=";
        case OP_IGUAL:       return "==";
        case OP_DIFERENTE:   return "!=";
        case OP_E:           return "&&";
        case OP_OU:          return "||";
        case OP_NEGATIVO:    return "-";
        case OP_NAO:         return "!";
    }
    return "?";
}
