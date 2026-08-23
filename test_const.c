void modify(int *p) {
    *p = 99;
}
int main() {
    const int x = 5;
    modify(&x);
    return 0;
}
