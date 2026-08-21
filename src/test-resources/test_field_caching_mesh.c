// Test: Field caching optimization for pointer-heavy struct access
// Demonstrates Phase 96.5 cross-module field caching benefits

#include <stdio.h>

// Mesh structure with pointer fields (typical graphics use case)
struct Mesh {
    int* vertices;      // Pointer to vertex data
    int* normals;       // Pointer to normal data
    int* texcoords;     // Pointer to texture coordinates
    int vertexCount;    // Count of vertices
};

// Accessing multiple fields in sequence (cache opportunity)
int mesh_compute_bounds(struct Mesh* mesh) {
    int minX = 32767, maxX = -32768;

    // Sequential field access pattern: vertices, then vertexCount
    // Cache optimizer should cache mesh->vertices pointer
    for (int i = 0; i < mesh->vertexCount; i++) {
        int x = mesh->vertices[i];
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
    }

    return maxX - minX;
}

// Field reuse pattern (cache retention opportunity)
void mesh_apply_transform(struct Mesh* mesh, int tx, int ty) {
    // Vertices accessed multiple times - should cache pointer
    for (int i = 0; i < mesh->vertexCount; i++) {
        mesh->vertices[i] += tx;  // First access
        mesh->vertices[i];        // Cached reuse
    }
}

// Multiple struct access in loop (hot struct detection)
int process_meshes(struct Mesh* meshes, int meshCount) {
    int totalVerts = 0;

    // Each mesh's pointer fields accessed in loop
    // Detector should mark Mesh as "hot struct"
    for (int m = 0; m < meshCount; m++) {
        // vertices pointer accessed in loop context
        for (int v = 0; v < meshes[m].vertexCount; v++) {
            totalVerts++;
        }
    }

    return totalVerts;
}

int main() {
    printf("Field caching test\n");
    return 0;
}
