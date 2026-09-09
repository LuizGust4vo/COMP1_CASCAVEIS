int soma(int a, int b) {
    return a + b;
}

float identidade_float(float x) {
    return x;
}

char identidade_char(char c) {
    return c;
}

void vazio() {
}

int principal() {
    int i = 0;

    if (i < 1) {
        i = i + 1;
    } else {
        i = i - 1;
    }

    while (i < 2) {
        i = i + 1;
    }

    for (i = 0; i < 2; i = i + 1) {
        soma(i, i);
    }

    return i;
}
