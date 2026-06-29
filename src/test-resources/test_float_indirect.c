// Test: F32 indirect access — pointer deref, struct member, array element

float g_float = 3.14;

// Test 1: pointer dereference (read/write)
void test_pointer() {
    float x = 1.5;
    float *p = &x;
    float y = *p;       // LOAD F32 via pointer
    *p = 2.5;           // STORE F32 via pointer
    float z = *p;       // read back
}

// Test 2: struct member access
struct HasFloat {
    int a;
    float f;
    int b;
};

void test_struct() {
    struct HasFloat s;
    s.a = 42;
    s.f = 3.14;         // STORE F32 to struct member
    s.b = 99;
    float x = s.f;      // LOAD F32 from struct member
}

// Test 3: struct pointer member access
void test_struct_ptr() {
    struct HasFloat s;
    struct HasFloat *p = &s;
    p->f = 6.28;        // STORE F32 via struct pointer
    float x = p->f;     // LOAD F32 via struct pointer
}

// Test 4: array element access
void test_array() {
    float arr[3];
    arr[0] = 1.0;       // STORE F32 to array element
    arr[1] = 2.0;
    arr[2] = 3.0;
    float x = arr[1];   // LOAD F32 from array element
}

// Test 5: array via pointer
void test_array_ptr() {
    float arr[3];
    float *p = arr;
    p[0] = 10.0;        // STORE F32 via pointer arithmetic
    p[1] = 20.0;
    float x = p[1];     // LOAD F32 via pointer arithmetic
}

int main() {
    test_pointer();
    test_struct();
    test_struct_ptr();
    test_array();
    test_array_ptr();
    return 0;
}
