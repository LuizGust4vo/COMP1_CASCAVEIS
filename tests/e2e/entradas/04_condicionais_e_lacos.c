int classificar(int n) {
    if (n < 0) {
        return -1;
    } else {
        if (n == 0) {
            return 0;
        } else {
            return 1;
        }
    }
}

int main() {
    printf(classificar(-5));
    printf(classificar(0));
    printf(classificar(7));

    int soma;
    soma = 0;
    int k = 1;
    while (k <= 5) {
        soma = soma + k;
        k = k + 1;
    }
    printf(soma);

    int pares = 0;
    for (int i = 0; i < 6; i = i + 2) {
        pares = pares + i;
    }
    printf(pares);

    int grade = 0;
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 4; j = j + 1) {
            grade = grade + 1;
        }
    }
    printf(grade);
    return 0;
}
