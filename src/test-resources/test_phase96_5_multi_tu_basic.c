// Phase 96.5.5: Multi-TU Test Program - Basic Cross-Module Field Caching
// Tests simple cross-module pointer field access optimization

// TU1: mesh.c - Mesh data structure and access functions
#include <stdio.h>

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int vertices;  // Pointer to vertex data (simulated as int)
    Point center;
    int index;
} Mesh;

// Global mesh array
Mesh meshes[8];

// Function in TU1: Load mesh vertices
int load_mesh_vertices(int mesh_id) {
    int verts = 0;

    // This accesses meshes[mesh_id].vertices repeatedly
    // Should be cached in Phase 96.5.4
    for (int i = 0; i < 10; i++) {
        verts += meshes[mesh_id].vertices;
    }

    return verts;
}

// Function in TU1: Get mesh center
Point get_mesh_center(int mesh_id) {
    // Accesses mesh center point
    return meshes[mesh_id].center;
}

// TU2: renderer.c (simulated in same file) - Rendering functions
extern int load_mesh_vertices(int mesh_id);
extern Point get_mesh_center(int mesh_id);

// Function in TU2: Render mesh
void render_mesh(int mesh_id) {
    // Access same mesh.vertices field from different module
    // Cross-module caching should coordinate this

    Point center = get_mesh_center(mesh_id);
    int verts = load_mesh_vertices(mesh_id);

    printf("Mesh %d: center (%d, %d), verts: %d\n",
           mesh_id, center.x, center.y, verts);
}

// TU3: main.c (simulated) - Main program
int main(void) {
    // Initialize meshes
    for (int i = 0; i < 8; i++) {
        meshes[i].vertices = i * 100;
        meshes[i].center.x = i * 10;
        meshes[i].center.y = i * 20;
        meshes[i].index = i;
    }

    // Render multiple meshes (exercises cross-module caching)
    for (int i = 0; i < 8; i++) {
        render_mesh(i);
    }

    printf("Cross-module field caching test completed\n");

    return 0;
}
