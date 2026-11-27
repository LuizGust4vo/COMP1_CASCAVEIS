int efeito_proibido() {
    printf("NAO_DEVE_IMPRIMIR");
    return 1;
}

int efeito_permitido() {
    printf("OK_CHAMOU");
    return 1;
}

int main() {
    int a = 5 && 3;
    int b = 0 || 9;
    int c = 5 && 0;
    int d = !0;
    int e = !42;
    int sc1 = 0 && efeito_proibido();
    int sc2 = 1 || efeito_proibido();
    int sc3 = 1 && efeito_permitido();
    int sc4 = 0 || efeito_permitido();

    printf(a);
    printf(b);
    printf(c);
    printf(d);
    printf(e);
    printf(sc1);
    printf(sc2);
    printf(sc3);
    printf(sc4);
    return 0;
}
