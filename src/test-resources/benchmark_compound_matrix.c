// Phase 88 Benchmark: Compound Assignment Matrix Operations
// Tests: matrix manipulation with heavy compound assignment chains
// Expected: 15-25% code reduction for nested loops with compound ops

#include <stdio.h>

#define MAT_SIZE 8

// Matrix-vector multiplication with accumulation
void matrix_vector_mult(int mat[8][8], int vec[8], int result[8]) {
    for (int i = 0; i < 8; i++) {
        result[i] = 0;
        for (int j = 0; j < 8; j++) {
            result[i] += mat[i][j] * vec[j];
        }
    }
}

// Element-wise matrix operations
void element_wise_ops(int mat_a[8][8], int mat_b[8][8], int mat_c[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            mat_c[i][j] = mat_a[i][j] + mat_b[i][j];
            mat_c[i][j] *= 2;
            mat_c[i][j] -= mat_a[i][j];
        }
    }
}

// Matrix transposition
void transpose_matrix(int mat[8][8], int result[8][8]) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            result[j][i] = mat[i][j];
        }
    }
}

// Scalar multiplication with accumulation
void scalar_accumulate(int mat[8][8], int scalar) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            mat[i][j] *= scalar;
            mat[i][j] += i + j;
        }
    }
}

// Row/column operations
void row_operations(int mat[8][8]) {
    for (int i = 0; i < 8; i++) {
        int row_sum = 0;
        for (int j = 0; j < 8; j++) {
            row_sum += mat[i][j];
        }
        for (int j = 0; j < 8; j++) {
            mat[i][j] -= row_sum / 8;
        }
    }
}

// Trace calculation (diagonal sum) with accumulation
int calculate_trace(int mat[8][8]) {
    int trace = 0;
    for (int i = 0; i < 8; i++) {
        trace += mat[i][i];
        trace -= (i > 0 ? mat[i-1][i-1] : 0);
    }
    return trace;
}

// Cumulative sum operations
void cumulative_sum(int mat[8][8], int result[8][8]) {
    result[0][0] = mat[0][0];

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            result[i][j] = mat[i][j];
            if (i > 0) result[i][j] += result[i-1][j];
            if (j > 0) result[i][j] += result[i][j-1];
            if (i > 0 && j > 0) result[i][j] -= result[i-1][j-1];
        }
    }
}

// Normalize matrix values
void normalize_matrix(int mat[8][8]) {
    int max_val = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (mat[i][j] > max_val) max_val = mat[i][j];
        }
    }

    if (max_val > 0) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                mat[i][j] *= 255;
                mat[i][j] /= max_val;
            }
        }
    }
}

int main() {
    static int mat_a[8][8];
    static int mat_b[8][8];
    static int mat_c[8][8];
    static int result[8];
    static int vec[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    // Initialize matrices
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            mat_a[i][j] = i * 8 + j;
            mat_b[i][j] = (i + j) % 8;
        }
        vec[i] = i + 1;
    }

    matrix_vector_mult(mat_a, vec, result);
    printf("mvmult done\n");

    element_wise_ops(mat_a, mat_b, mat_c);
    printf("elemwise done\n");

    transpose_matrix(mat_a, mat_c);
    printf("transpose done\n");

    scalar_accumulate(mat_b, 3);
    printf("scalar done\n");

    row_operations(mat_a);
    printf("row_ops done\n");

    int tr = calculate_trace(mat_a);
    printf("trace=%d\n", tr);

    cumulative_sum(mat_a, mat_c);
    printf("cumsum done\n");

    normalize_matrix(mat_a);
    printf("normalize done\n");

    return 0;
}
