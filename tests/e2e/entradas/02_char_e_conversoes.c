int codigo_char(char c) {
    return c;
}

int main() {
    char c1 = 'd';
    char c2 = 'b';
    int diff = c1 - c2;
    int quoc = c1 / c2;
    int cod = codigo_char('A');
    int maior = c1 > c2;
    int igual = c2 == 'b';

    printf(diff);
    printf(quoc);
    printf(cod);
    printf(maior);
    printf(igual);
    printf(c1);
    return 0;
}
