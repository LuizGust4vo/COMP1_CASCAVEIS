#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "ast.h"

/* Retorna o número de erros semânticos encontrados (0 = ok). */
int semantico_analisa(const AstNo *raiz);

#endif
