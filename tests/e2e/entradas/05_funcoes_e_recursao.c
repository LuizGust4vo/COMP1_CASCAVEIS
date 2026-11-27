int fatorial(int n);

void vazia() {
}

void saudacao() {
    printf("inicio");
}

int fatorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * fatorial(n - 1);
}

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int soma_tripla(int a, int b, int c) {
    return a + b + c;
}

int main() {
    vazia();
    saudacao();
    printf(fatorial(5));
    printf(fibonacci(7));
    printf(soma_tripla(fatorial(3), fibonacci(5), 10));
    printf("fim");
    return 0;
}
