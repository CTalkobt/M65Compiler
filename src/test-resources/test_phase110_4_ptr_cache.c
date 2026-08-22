// Test Phase 110.4: Pointer Chasing Optimization
// Tests caching of frequently-dereferenced pointers

#include <stddef.h>

// =============================================================================
// SIMPLE STRUCT DEFINITIONS
// =============================================================================

struct Node {
    int value;
    struct Node* next;
    int count;
};

struct Point {
    int x;
    int y;
    int z;
};

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

// =============================================================================
// SINGLE POINTER DEREFERENCING
// =============================================================================

// Simple case: pointer dereferenced 3 times (should cache)
int test_single_ptr_triple(struct Node* p) {
    int sum = 0;
    sum += p->value;     // First dereference
    sum += p->count;     // Second dereference
    sum += p->value;     // Third dereference (might be redundant)
    return sum;
}

// Pointer dereferenced twice (break-even, might not cache)
int test_single_ptr_double(struct Node* p) {
    return p->value + p->count;  // Two dereferences
}

// Pointer dereferenced once (definitely don't cache)
int test_single_ptr_single(struct Node* p) {
    return p->value;  // Single dereference
}

// =============================================================================
// LOOP-BASED POINTER CHASING
// =============================================================================

// Traverse linked list with cached pointer
int test_list_traverse_cached(struct Node* head) {
    int sum = 0;
    struct Node* p = head;
    while (p != NULL) {
        sum += p->value;    // Dereference 1 per iteration
        sum += p->count;    // Dereference 2 per iteration
        p = p->next;        // Dereference 3 per iteration
    }
    return sum;
    // With caching: cache p at loop start, reuse cached dereferences
}

// Array of structs with pointer access
int test_array_ptr_loop(struct Point* points, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        struct Point* p = &points[i];
        sum += p->x;        // Dereference 1
        sum += p->y;        // Dereference 2
        sum += p->z;        // Dereference 3
    }
    return sum;
}

// =============================================================================
// NESTED POINTER DEREFERENCING
// =============================================================================

// Pointer to struct containing pointer
int test_nested_ptr(struct Node* head) {
    int sum = 0;
    if (head != NULL) {
        struct Node* p = head->next;  // First dereference
        if (p != NULL) {
            sum += p->value;          // Second dereference
            sum += p->count;          // Third dereference
        }
    }
    return sum;
}

// Multiple pointer levels
struct Wrapper {
    struct Node* node;
    int id;
};

int test_multi_level_ptr(struct Wrapper* w) {
    int sum = 0;
    sum += w->node->value;   // Two dereferences
    sum += w->node->count;   // Two dereferences (same nesting)
    sum += w->id;            // Direct field
    return sum;
}

// =============================================================================
// LOOP PATTERNS WITH POINTER ARRAYS
// =============================================================================

// Iterate through array of pointers
int test_ptr_array_loop(struct Node** nodes, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        struct Node* p = nodes[i];  // Dereference array
        sum += p->value;             // Dereference struct
        sum += p->count;             // Dereference struct again
    }
    return sum;
}

// List with frequent field accesses
int test_frequent_access(struct Node* head) {
    int sum = 0;
    struct Node* p = head;
    int iterations = 0;
    while (p != NULL && iterations < 5) {
        sum += p->value;      // Access 1
        sum += p->count;      // Access 2
        p = p->next;          // Advance
        iterations++;
        // Multiple accesses to same fields per iteration
        // This should definitely be cached
    }
    return sum;
}

// =============================================================================
// COLOR STRUCT ITERATION (REAL-WORLD PATTERN)
// =============================================================================

// Process color array
int test_color_array(struct Color* colors, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        struct Color* c = &colors[i];
        sum += c->r + c->g + c->b;  // Multiple component accesses
    }
    return sum;
}

// Blend colors
struct Color blend_colors(struct Color* c1, struct Color* c2) {
    struct Color result;
    result.r = (c1->r + c2->r) / 2;  // c1 dereferenced twice
    result.g = (c1->g + c2->g) / 2;  // c1 dereferenced twice more
    result.b = (c1->b + c2->b) / 2;  // c1 dereferenced twice more
    result.a = (c1->a + c2->a) / 2;  // c1 dereferenced twice more
    return result;
    // c1 dereferenced 8 times total - definitely cache!
}

// =============================================================================
// NON-CACHEABLE PATTERNS
// =============================================================================

// Pointer modified in loop (can't cache)
int test_ptr_modified(struct Node* head) {
    int sum = 0;
    struct Node* p = head;
    while (p != NULL) {
        sum += p->value;
        p = p->next;
        p = NULL;  // Pointer modified! Can't cache
    }
    return sum;
}

// Pointer dereferenced once per loop (no benefit from caching)
int test_ptr_once_per_loop(struct Node* head) {
    int sum = 0;
    struct Node* p = head;
    while (p != NULL) {
        sum += p->value;  // Only one dereference per iteration
        p = p->next;      // Counted separately
    }
    return sum;
}

// Volatile pointer (can't cache)
int test_volatile_ptr(volatile struct Node* p) {
    int sum = 0;
    sum += p->value;   // Volatile - can't cache
    sum += p->value;   // Must re-read each time
    return sum;
}

// =============================================================================
// REAL-WORLD PATTERNS
// =============================================================================

// Game entity processing
struct Entity {
    int health;
    int armor;
    int ammo;
    struct Entity* next;
};

int process_entities(struct Entity* head) {
    int total_health = 0;
    struct Entity* e = head;
    while (e != NULL) {
        if (e->health > 0) {
            total_health += e->health;
            int damage = 10 - e->armor;
            if (damage < 0) damage = 0;
        }
        e = e->next;
    }
    return total_health;
}

// Data structure lookup
struct Entry {
    int key;
    int value;
    struct Entry* next;
};

int lookup_value(struct Entry* table, int key) {
    struct Entry* e = table;
    while (e != NULL) {
        if (e->key == key) {
            return e->value;  // Found it
        }
        e = e->next;
    }
    return -1;  // Not found
}

// Batch processing
struct Batch {
    int id;
    int count;
    int priority;
    struct Batch* next;
};

int process_batches(struct Batch* batches) {
    int total = 0;
    struct Batch* b = batches;
    while (b != NULL) {
        total += b->count * b->priority;  // Multiple accesses to same struct
        total += b->id;                   // Another access
        b = b->next;
    }
    return total;
}

// =============================================================================
// MATRIX ELEMENT ACCESS (POINTER-HEAVY)
// =============================================================================

struct Matrix {
    int rows;
    int cols;
    int* data;
};

int sum_matrix(struct Matrix* m) {
    int sum = 0;
    for (int i = 0; i < m->rows; i++) {        // m dereferenced
        for (int j = 0; j < m->cols; j++) {    // m dereferenced again in inner loop
            sum += m->data[i * m->cols + j];  // m dereferenced yet again
        }
    }
    return sum;
    // m is dereferenced many times - definitely candidate for caching
}

// =============================================================================
// CACHE BENEFIT MEASUREMENT
// =============================================================================

// High-benefit case: 5+ dereferences
int test_high_benefit(struct Node* p) {
    int sum = 0;
    sum += p->value;    // 1
    sum += p->count;    // 2
    sum += p->value;    // 3
    sum += p->count;    // 4
    sum += p->value;    // 5
    return sum;
    // Benefit: (5-1)*2 - 4 = 4 bytes saved
}

// Medium-benefit case: 3 dereferences
int test_medium_benefit(struct Node* p) {
    int sum = 0;
    sum += p->value;    // 1
    sum += p->count;    // 2
    sum += p->value;    // 3
    return sum;
    // Benefit: (3-1)*2 - 4 = 0 bytes (break-even)
}

// Low-benefit case: 2 dereferences
int test_low_benefit(struct Node* p) {
    return p->value + p->count;  // 2 dereferences
    // Benefit: (2-1)*2 - 4 = -2 bytes (don't cache)
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    int total = 0;

    // Create test data
    struct Node n3 = {3, NULL, 3};
    struct Node n2 = {2, &n3, 2};
    struct Node n1 = {1, &n2, 1};

    // Test single pointer dereferencing
    total += test_single_ptr_triple(&n1);     // 1+1+1 = 3
    total += test_single_ptr_double(&n1);     // 1+1 = 2

    // Test list traversal
    total += test_list_traverse_cached(&n1);  // Sum of values+counts

    // Test nested pointers
    total += test_nested_ptr(&n1);            // 1+1 = 2

    // Test multi-level pointers
    struct Wrapper w = {&n1, 42};
    total += test_multi_level_ptr(&w);        // 1+1+42

    // Test real-world patterns
    total += process_entities(&n1);           // Process entities

    // Test matrix access
    int data[] = {1, 2, 3, 4, 5, 6};
    struct Matrix m = {2, 3, data};
    total += sum_matrix(&m);                  // Sum of matrix

    // Test cache benefit cases
    total += test_high_benefit(&n1);          // 5 dereferences
    total += test_medium_benefit(&n1);        // 3 dereferences

    return total;  // Should sum to reasonable value
}
