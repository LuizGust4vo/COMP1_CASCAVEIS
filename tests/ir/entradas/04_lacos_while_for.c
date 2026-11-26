int somar(int limite) {
    int i = 0;
    int j = 0;
    int total = 0;

    while (i < limite) {
        total = total + i;
        i = i + 1;
    }

    for (j = 0; j < limite; j = j + 1) {
        int k = 0;
        while (k < 2) {
            total = total + 1;
            k = k + 1;
        }
    }

    return total;
}