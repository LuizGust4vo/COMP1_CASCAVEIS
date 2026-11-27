int eh_primo(int n) {
    if (n < 2) {
        return 0;
    }
    int d = 2;
    while (d * d <= n) {
        if ((n / d) * d == n) {
            return 0;
        }
        d = d + 1;
    }
    return 1;
}

int main() {
    int qtd = 0;
    int soma = 0;
    for (int n = 1; n <= 15; n = n + 1) {
        if (eh_primo(n)) {
            qtd = qtd + 1;
            soma = soma + n;
        }
    }
    printf(qtd);
    printf(soma);
    return 0;
}
