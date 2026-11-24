int main() {
    int x = 5;
    int y = 10;
    int r1 = 0;
    int r2 = 0;

    x = x + 1;
    r1 = (x < y) && (y != 0);
    r2 = (x >= y) || (x == 6);

    if (r1 && !r2) {
        x = 1;
    } else {
        x = 2;
    }

    return x;
}