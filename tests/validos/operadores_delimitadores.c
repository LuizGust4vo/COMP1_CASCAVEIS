int soma(int x, int y) {
    return x + y;
}

int principal() {
    int a = 1;
    int b = 2;
    int c = 0;

    c = a + b;
    c = c - 1;
    c = c * 2;
    c = c / 1;

    if (a == 1 && b != 0) {
        c = soma(a, b);
    }

    if (a <= b || b >= a) {
        c = !c;
    }

    if (a < b) {
        c = a;
    }

    if (b > a) {
        c = b;
    }

    return c;
}
