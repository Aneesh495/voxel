#include "volume.h"
#include <stdlib.h>
#include <string.h>

volume_grid_t *volume_grid_create(size_t dx, size_t dy, size_t dz, double sx, double sy, double sz) {
    if (dx == 0 || dy == 0 || dz == 0) return NULL;
    volume_grid_t *g = (volume_grid_t *)calloc(1, sizeof(volume_grid_t));
    if (!g) return NULL;

    g->dim_x = dx;
    g->dim_y = dy;
    g->dim_z = dz;
    g->spacing_x = (sx > 0.0) ? sx : 1.0;
    g->spacing_y = (sy > 0.0) ? sy : 1.0;
    g->spacing_z = (sz > 0.0) ? sz : 1.0;

    g->origin = (vec3_t){ 0.0, 0.0, 0.0 };
    g->row_dir = (vec3_t){ 1.0, 0.0, 0.0 };
    g->col_dir = (vec3_t){ 0.0, 1.0, 0.0 };
    g->slice_dir = (vec3_t){ 0.0, 0.0, 1.0 };

    size_t total_voxels = dx * dy * dz;
    g->voxels = (int16_t *)malloc(total_voxels * sizeof(int16_t));
    if (!g->voxels) {
        free(g);
        return NULL;
    }

    // Default initialization to air (-1000 HU)
    for (size_t i = 0; i < total_voxels; i++) {
        g->voxels[i] = -1000;
    }

    return g;
}

void volume_grid_destroy(volume_grid_t *g) {
    if (g) {
        if (g->voxels) {
            free(g->voxels);
        }
        free(g);
    }
}

int16_t volume_grid_get(const volume_grid_t *g, size_t x, size_t y, size_t z) {
    if (!g || !g->voxels) return -1000;
    if (x >= g->dim_x || y >= g->dim_y || z >= g->dim_z) return -1000;
    size_t idx = z * (g->dim_x * g->dim_y) + y * g->dim_x + x;
    return g->voxels[idx];
}

void volume_grid_set(volume_grid_t *g, size_t x, size_t y, size_t z, int16_t val) {
    if (!g || !g->voxels) return;
    if (x >= g->dim_x || y >= g->dim_y || z >= g->dim_z) return;
    size_t idx = z * (g->dim_x * g->dim_y) + y * g->dim_x + x;
    g->voxels[idx] = val;
}
