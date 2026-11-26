int fatorial(int n) {
    int acc = 1;
    int i = 1;

    while (i <= n) {
        acc = acc * i;
        i = i + 1;
    }

    return acc;
}

int main() {
    int base = 5;
    int fat = 0;

    if (base > 0 && base < 10) {
        fat = fatorial(base);
    } else {
        fat = 1;
    }

    return fat;
}