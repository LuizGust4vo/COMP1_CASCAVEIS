int soma_pares(int limite) {
    int soma = 0;
    for (int i = 0; i <= limite; i = i + 1) {
        if (i - (i / 2) * 2 == 0) {
            soma = soma + i;
        } else {
            soma = soma;
        }
    }
    return soma;
}