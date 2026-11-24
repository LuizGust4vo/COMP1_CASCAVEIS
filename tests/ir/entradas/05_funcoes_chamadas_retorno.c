int soma(int a, int b);

int dobro(int v) {
    int r = v * 2;
    return r;
}

int soma(int a, int b) {
    if (a > b) {
        return a + b;
    }
    return b + a;
}

void log_noop(int x) {
    int local = x + 1;
}

int main() {
    int res = soma(dobro(2), dobro(3));
    log_noop(res);
    return res;
}