int primeiro_par(int n) {
    int i = 1;
    while (i <= n) {
        if ((i / 2) * 2 == i) {
            return i;
            i = i + 100;
        }
        i = i + 1;
    }
    return 0;
    printf("nunca");
}

int sinal(int v) {
    if (v < 0) {
        return 0 - 1;
    } else {
        return 1;
        v = v * 2;
    }
    return 0;
}

int main() {
    printf(primeiro_par(7));
    printf(sinal(0 - 5));
    printf(sinal(3));
    return 0;
    printf(99);
}
