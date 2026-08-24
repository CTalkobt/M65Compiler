// Test IRBuilder semantic error enhancements

// Test 1: Unknown struct type
void test_unknown_struct(void) {
    struct UnknownType x;
    return;
}

// Test 2: Compound literal with unknown struct
void test_unknown_struct_compound(void) {
    struct NotDefined val = (struct NotDefined){42};
    return;
}

// Note: Member not found errors require struct definitions
// which get validated during IR generation
