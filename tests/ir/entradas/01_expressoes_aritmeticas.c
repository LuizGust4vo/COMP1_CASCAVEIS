int div_char(char a, char b) {
    return a / b;
}

int main() {
    int a = 2 + 3 * 4;
    int b = (10 - 2) / 2;
    int c = -a + b;
    float f = 7.5 / 2.5;
    char ch1 = 'd';
    char ch2 = 'b';
    int d = div_char(ch1, ch2);
    return c + d;
}