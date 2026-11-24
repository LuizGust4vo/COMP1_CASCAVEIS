int classificar(int n) {
    int res = 0;

    if (n > 0) {
        if (n > 10) {
            res = 2;
        } else {
            res = 1;
        }
    } else {
        res = -1;
    }

    if (n == 0) {
        res = 0;
    }

    return res;
}