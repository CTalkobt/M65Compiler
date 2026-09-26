// Demonstration of enhanced error messages
// This file shows various syntax errors that trigger enhanced suggestions

// Error 1: Missing semicolon at function level
int get_value(void)
{
    int x = 42
    return x;
}

// Error 2: Parameter list error
void add(int a int b) {
    return a + b;
}

// Note: We don't add more errors to keep output manageable
