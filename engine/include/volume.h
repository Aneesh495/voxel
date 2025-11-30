#ifndef VOXEL_VOLUME_H
#define VOXEL_VOLUME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double x;
    double y;
    double z;
} vec3_t;

// MPR Orthogonal Planes
typedef enum {
    MPR_PLANE_AXIAL = 0,    // Transverse (Z fixed)
    MPR_PLANE_CORONAL = 1,  // Frontal (Y fixed)
    MPR_PLANE_SAGITTAL = 2  // Lateral (X fixed)
} mpr_plane_t;

// Clinical Window/Level Presets
typedef enum {
    PRESET_SOFT_TISSUE, // C: 40, W: 350
    PRESET_BONE,        // C: 400, W: 2000
    PRESET_LUNG,        // C: -600, W: 1500
    PRESET_BRAIN,       // C: 40, W: 80
    PRESET_LIVER,       // C: 30, W: 150
    PRESET_MEDIASTINUM  // C: 50, W: 450
} clinical_preset_t;

typedef struct {
    double center;
    double width;
} window_level_t;

// 3D Volumetric Regular Grid
typedef struct {
    size_t dim_x;
    size_t dim_y;
    size_t dim_z;
    double spacing_x;
    double spacing_y;
    double spacing_z;
    vec3_t origin;
    vec3_t row_dir;
    vec3_t col_dir;
    vec3_t slice_dir;
    int16_t *voxels; // Contiguous flat array: dim_x * dim_y * dim_z
} volume_grid_t;

// Projection modes
typedef enum {
    PROJECTION_MIP,   // Maximum Intensity Projection
    PROJECTION_MINIP, // Minimum Intensity Projection
    PROJECTION_AIP    // Average Intensity Projection
} projection_mode_t;

// Volume Grid API
volume_grid_t *volume_grid_create(size_t dx, size_t dy, size_t dz, double sx, double sy, double sz);
void volume_grid_destroy(volume_grid_t *g);

int16_t volume_grid_get(const volume_grid_t *g, size_t x, size_t y, size_t z);
void volume_grid_set(volume_grid_t *g, size_t x, size_t y, size_t z, int16_t val);

// Trilinear interpolation
double volume_sample_trilinear(const volume_grid_t *g, double x, double y, double z);
double volume_sample_nearest(const volume_grid_t *g, double x, double y, double z);

// Window/Level contrast transform
window_level_t volume_get_preset(clinical_preset_t preset);
uint8_t volume_hu_to_display(double hu, window_level_t wl);

// MPR 2D slice rendering
bool volume_render_mpr_slice(
    const volume_grid_t *g,
    mpr_plane_t plane,
    size_t slice_idx,
    window_level_t wl,
    uint8_t *out_pixels,
    size_t *out_w,
    size_t *out_h
);

// Ray-cast projection rendering
bool volume_render_projection(
    const volume_grid_t *g,
    mpr_plane_t plane,
    projection_mode_t mode,
    size_t start_depth,
    size_t depth_count,
    window_level_t wl,
    uint8_t *out_pixels,
    size_t *out_w,
    size_t *out_h
);

// Synthetic anatomical CT scan generator (head phantom with bone, brain, ventricles, sinuses)
volume_grid_t *volume_generate_synthetic_head(size_t dim_x, size_t dim_y, size_t dim_z);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_VOLUME_H
