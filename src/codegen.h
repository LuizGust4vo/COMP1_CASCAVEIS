#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

/*
 * Gera código Python a partir da representação intermediária
 * atualmente armazenada pelo môdulo de RI.
 *
 * O arquivo de saída deve estar aberto para escrita.
 *
 * Retorna 0 em caso de sucesso e diferente de 0 em caso de erro.
 */
int codegen_gera_python(FILE *saida);

#endif