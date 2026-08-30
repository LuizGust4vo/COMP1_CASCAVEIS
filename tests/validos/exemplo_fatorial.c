int fatorial(int n) {
    int resultado = 1;
    int i = 1;
    while (i <= n) {
        resultado = resultado * i;
        i = i + 1;
    }
    return resultado;
}

int principal() {
    int x = 5;
    if (x > 0 && x < 10) {
        return fatorial(x);
    } else {
        return -1;
    }
}
