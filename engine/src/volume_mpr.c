#include "volume.h"
#include <math.h>
#include <string.h>

window_level_t volume_get_preset(clinical_preset_t preset) {
    switch (preset) {
        case PRESET_SOFT_TISSUE: return (window_level_t){ .center = 40.0, .width = 350.0 };
        case PRESET_BONE:        return (window_level_t){ .center = 400.0, .width = 2000.0 };
        case PRESET_LUNG:        return (window_level_t){ .center = -600.0, .width = 1500.0 };
        case PRESET_BRAIN:       return (window_level_t){ .center = 40.0, .width = 80.0 };
        case PRESET_LIVER:       return (window_level_t){ .center = 30.0, .width = 150.0 };
        case PRESET_MEDIASTINUM: return (window_level_t){ .center = 50.0, .width = 450.0 };
        default:                 return (window_level_t){ .center = 40.0, .width = 350.0 };
    }
}

uint8_t volume_hu_to_display(double hu, window_level_t wl) {
    if (wl.width <= 1.0) {
        return (hu >= wl.center) ? 255 : 0;
    }
    double lower = wl.center - 0.5 - (wl.width - 1.0) / 2.0;
    double upper = wl.center - 0.5 + (wl.width - 1.0) / 2.0;

    if (hu <= lower) return 0;
    if (hu > upper) return 255;

    double normalized = (hu - (wl.center - 0.5)) / (wl.width - 1.0) + 0.5;
    double val = normalized * 255.0;
    if (val < 0.0) return 0;
    if (val > 255.0) return 255;
    return (uint8_t)val;
}

double volume_sample_trilinear(const volume_grid_t *g, double x, double y, double z) {
    if (!g || !g->voxels) return -1000.0;
    if (x < 0.0 || x > (double)(g->dim_x - 1) ||
        y < 0.0 || y > (double)(g->dim_y - 1) ||
        z < 0.0 || z > (double)(g->dim_z - 1)) {
        return -1000.0;
    }

    size_t x0 = (size_t)x;
    size_t y0 = (size_t)y;
    size_t z0 = (size_t)z;

    size_t x1 = (x0 + 1 < g->dim_x) ? (x0 + 1) : x0;
    size_t y1 = (y0 + 1 < g->dim_y) ? (y0 + 1) : y0;
    size_t z1 = (z0 + 1 < g->dim_z) ? (z0 + 1) : z0;

    double u = x - (double)x0;
    double v = y - (double)y0;
    double w = z - (double)z0;

    double c000 = (double)volume_grid_get(g, x0, y0, z0);
    double c100 = (double)volume_grid_get(g, x1, y0, z0);
    double c010 = (double)volume_grid_get(g, x0, y1, z0);
    double c110 = (double)volume_grid_get(g, x1, y1, z0);
    double c001 = (double)volume_grid_get(g, x0, y0, z1);
    double c101 = (double)volume_grid_get(g, x1, y0, z1);
    double c011 = (double)volume_grid_get(g, x0, y1, z1);
    double c111 = (double)volume_grid_get(g, x1, y1, z1);

    double c00 = c000 * (1.0 - u) + c100 * u;
    double c10 = c010 * (1.0 - u) + c110 * u;
    double c01 = c001 * (1.0 - u) + c101 * u;
    double c11 = c011 * (1.0 - u) + c111 * u;

    double c0 = c00 * (1.0 - v) + c10 * v;
    double c1 = c01 * (1.0 - v) + c11 * v;

    return c0 * (1.0 - w) + c1 * w;
}

double volume_sample_nearest(const volume_grid_t *g, double x, double y, double z) {
    if (!g || !g->voxels) return -1000.0;
    size_t rx = (size_t)(x + 0.5);
    size_t ry = (size_t)(y + 0.5);
    size_t rz = (size_t)(z + 0.5);
    return (double)volume_grid_get(g, rx, ry, rz);
}

bool volume_render_mpr_slice(
    const volume_grid_t *g,
    mpr_plane_t plane,
    size_t slice_idx,
    window_level_t wl,
    uint8_t *out_pixels,
    size_t *out_w,
    size_t *out_h
) {
    if (!g || !out_pixels || !out_w || !out_h) return false;

    switch (plane) {
        case MPR_PLANE_AXIAL: {
            size_t z = (slice_idx < g->dim_z) ? slice_idx : (g->dim_z / 2);
            size_t w = g->dim_x;
            size_t h = g->dim_y;
            *out_w = w;
            *out_h = h;

            for (size_t r = 0; r < h; r++) {
                for (size_t c = 0; c < w; c++) {
                    double hu = (double)volume_grid_get(g, c, r, z);
                    out_pixels[r * w + c] = volume_hu_to_display(hu, wl);
                }
            }
            return true;
        }

        case MPR_PLANE_CORONAL: {
            size_t y = (slice_idx < g->dim_y) ? slice_idx : (g->dim_y / 2);
            size_t w = g->dim_x;
            size_t h = g->dim_z;
            *out_w = w;
            *out_h = h;

            for (size_t r = 0; r < h; r++) {
                size_t z = h - 1 - r;
                for (size_t c = 0; c < w; c++) {
                    double hu = (double)volume_grid_get(g, c, y, z);
                    out_pixels[r * w + c] = volume_hu_to_display(hu, wl);
                }
            }
            return true;
        }

        case MPR_PLANE_SAGITTAL: {
            size_t x = (slice_idx < g->dim_x) ? slice_idx : (g->dim_x / 2);
            size_t w = g->dim_y;
            size_t h = g->dim_z;
            *out_w = w;
            *out_h = h;

            for (size_t r = 0; r < h; r++) {
                size_t z = h - 1 - r;
                for (size_t c = 0; c < w; c++) {
                    double hu = (double)volume_grid_get(g, x, c, z);
                    out_pixels[r * w + c] = volume_hu_to_display(hu, wl);
                }
            }
            return true;
        }

        default:
            return false;
    }
}
