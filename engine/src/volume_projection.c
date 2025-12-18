#include "volume.h"

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
) {
    if (!g || !out_pixels || !out_w || !out_h) return false;

    switch (plane) {
        case MPR_PLANE_AXIAL: {
            size_t w = g->dim_x;
            size_t h = g->dim_y;
            *out_w = w;
            *out_h = h;
            size_t max_z = (start_depth + depth_count < g->dim_z) ? (start_depth + depth_count) : g->dim_z;

            for (size_t y = 0; y < h; y++) {
                for (size_t x = 0; x < w; x++) {
                    double max_hu = -10000.0;
                    double min_hu = 10000.0;
                    double sum_hu = 0.0;
                    double count = 0.0;

                    for (size_t z = start_depth; z < max_z; z++) {
                        double val = (double)volume_grid_get(g, x, y, z);
                        if (val > max_hu) max_hu = val;
                        if (val < min_hu) min_hu = val;
                        sum_hu += val;
                        count += 1.0;
                    }

                    double final_hu = max_hu;
                    if (mode == PROJECTION_MINIP) {
                        final_hu = min_hu;
                    } else if (mode == PROJECTION_AIP) {
                        final_hu = (count > 0.0) ? (sum_hu / count) : -1000.0;
                    }

                    out_pixels[y * w + x] = volume_hu_to_display(final_hu, wl);
                }
            }
            return true;
        }

        case MPR_PLANE_CORONAL: {
            size_t w = g->dim_x;
            size_t h = g->dim_z;
            *out_w = w;
            *out_h = h;
            size_t max_y = (start_depth + depth_count < g->dim_y) ? (start_depth + depth_count) : g->dim_y;

            for (size_t r = 0; r < h; r++) {
                size_t z = h - 1 - r;
                for (size_t x = 0; x < w; x++) {
                    double max_hu = -10000.0;
                    double min_hu = 10000.0;
                    double sum_hu = 0.0;
                    double count = 0.0;

                    for (size_t y = start_depth; y < max_y; y++) {
                        double val = (double)volume_grid_get(g, x, y, z);
                        if (val > max_hu) max_hu = val;
                        if (val < min_hu) min_hu = val;
                        sum_hu += val;
                        count += 1.0;
                    }

                    double final_hu = max_hu;
                    if (mode == PROJECTION_MINIP) {
                        final_hu = min_hu;
                    } else if (mode == PROJECTION_AIP) {
                        final_hu = (count > 0.0) ? (sum_hu / count) : -1000.0;
                    }

                    out_pixels[r * w + x] = volume_hu_to_display(final_hu, wl);
                }
            }
            return true;
        }

        case MPR_PLANE_SAGITTAL: {
            size_t w = g->dim_y;
            size_t h = g->dim_z;
            *out_w = w;
            *out_h = h;
            size_t max_x = (start_depth + depth_count < g->dim_x) ? (start_depth + depth_count) : g->dim_x;

            for (size_t r = 0; r < h; r++) {
                size_t z = h - 1 - r;
                for (size_t y = 0; y < w; y++) {
                    double max_hu = -10000.0;
                    double min_hu = 10000.0;
                    double sum_hu = 0.0;
                    double count = 0.0;

                    for (size_t x = start_depth; x < max_x; x++) {
                        double val = (double)volume_grid_get(g, x, y, z);
                        if (val > max_hu) max_hu = val;
                        if (val < min_hu) min_hu = val;
                        sum_hu += val;
                        count += 1.0;
                    }

                    double final_hu = max_hu;
                    if (mode == PROJECTION_MINIP) {
                        final_hu = min_hu;
                    } else if (mode == PROJECTION_AIP) {
                        final_hu = (count > 0.0) ? (sum_hu / count) : -1000.0;
                    }

                    out_pixels[r * w + y] = volume_hu_to_display(final_hu, wl);
                }
            }
            return true;
        }

        default:
            return false;
    }
}
