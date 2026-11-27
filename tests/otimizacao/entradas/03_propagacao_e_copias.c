int dobro(int x) {
    return x * 2;
}

int main() {
    int t1 = 10;
    int limite = 2 * 3;
    int soma = 0;
    int i = 0;
    while (i < limite) {
        soma = soma + dobro(i);
        i = i + 1;
    }
    int e = (1 < 2) && (t1 > 5);
    int o = (2 > 3) || 0;
    int x = dobro(t1 + 1);
    printf(soma);
    printf(e);
    printf(o);
    printf(x);
    printf(t1);
    return 0;
}
