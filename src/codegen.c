#include <stdio.h>
#include "codegen.h"
#include "ir.h"

static const char *operador_python(IrOp op)
{
    switch (op)
    {
    case IR_SOMA:
        return "+";
    case IR_SUB:
        return "-";
    case IR_MUL:
        return "*";
    case IR_DIV:
        return "/";
    case IR_DIV_INT:
        return "//";

    case IR_MENOR:
        return "<";
    case IR_MAIOR:
        return ">";
    case IR_MENOR_IGUAL:
        return "<=";
    case IR_MAIOR_IGUAL:
        return ">=";
    case IR_IGUAL:
        return "==";
    case IR_DIFERENTE:
        return "!=";

    default:
        return NULL;
    }
}

int codegen_gera_python(FILE *saida)
{
    const Quadrupla *q = ir_primeira();

    while (q)
    {
        switch (q->op)
        {
        case IR_DECL:
            fprintf(saida, "%s = None\n", q->resultado);
            break;

        case IR_ATRIB:
            fprintf(saida, "%s = %s\n",
                    q->resultado,
                    q->arg1);
            break;

        case IR_SOMA:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
        case IR_DIV_INT:
        case IR_MENOR:
        case IR_MAIOR:
        case IR_MENOR_IGUAL:
        case IR_MAIOR_IGUAL:
        case IR_IGUAL:
        case IR_DIFERENTE:
            fprintf(saida, "%s = %s %s %s\n",
                    q->resultado,
                    q->arg1,
                    operador_python(q->op),
                    q->arg2);
            break;

        case IR_NEGATIVO:
            fprintf(saida, "%s = -%s\n",
                    q->resultado,
                    q->arg1);
            break;

        case IR_NAO:
            fprintf(saida, "%s = not %s\n",
                    q->resultado,
                    q->arg1);
            break;

        case IR_RETURN:
            if (q->arg1)
            {
                fprintf(saida, "return %s\n", q->arg1);
            }
            else
            {
                fprintf(saida, "return\n");
            }
            break;

        default:
            /*
             * As demais instruções da RI serão implementadas
             * nos próximos passos.
             */
            break;
        }

        q = q->proximo;
    }

    return 0;
}