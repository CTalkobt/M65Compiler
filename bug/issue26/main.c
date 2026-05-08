void print_error();

void error_handler() {
    print_error();
    while(1) {}
    return;
}

int main() {
    error_handler();
    return 0;
}
