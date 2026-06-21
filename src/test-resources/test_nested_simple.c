void parent() {
    int x = 42;
    void nested() {
        x = 43;
    }
    nested();
}

int main() {
    parent();
    return 0;
}
