#include "volume.h"
#include <math.h>

volume_grid_t *volume_generate_synthetic_head(size_t dim_x, size_t dim_y, size_t dim_z) {
    if (dim_x == 0) dim_x = 256;
    if (dim_y == 0) dim_y = 256;
    if (dim_z == 0) dim_z = 64;

    double sx = 0.8;
    double sy = 0.8;
    double sz = 2.5;

    volume_grid_t *g = volume_grid_create(dim_x, dim_y, dim_z, sx, sy, sz);
    if (!g) return NULL;

    double cx = (double)dim_x / 2.0;
    double cy = (double)dim_y / 2.0;
    double cz = (double)dim_z / 2.0;

    // Radii of anatomical ellipsoids
    double rx_scalp = (double)dim_x * 0.40;
    double ry_scalp = (double)dim_y * 0.46;
    double rz_scalp = (double)dim_z * 0.42;

    double rx_bone_out = rx_scalp * 0.94;
    double ry_bone_out = ry_scalp * 0.94;
    double rz_bone_out = rz_scalp * 0.94;

    double rx_bone_in = rx_bone_out * 0.88;
    double ry_bone_in = ry_bone_out * 0.88;
    double rz_bone_in = rz_bone_out * 0.88;

    for (size_t z = 0; z < dim_z; z++) {
        double dz = ((double)z - cz) / rz_scalp;

        for (size_t y = 0; y < dim_y; y++) {
            double dy = ((double)y - cy) / ry_scalp;

            for (size_t x = 0; x < dim_x; x++) {
                double dx = ((double)x - cx) / rx_scalp;

                double d_scalp = (dx * dx) + (dy * dy) + (dz * dz);
                if (d_scalp > 1.0) {
                    // Air
                    volume_grid_set(g, x, y, z, -1000);
                    continue;
                }

                // Check Skull Outer
                double dx_b = ((double)x - cx) / rx_bone_out;
                double dy_b = ((double)y - cy) / ry_bone_out;
                double dz_b = ((double)z - cz) / rz_bone_out;
                double d_bone_out = (dx_b * dx_b) + (dy_b * dy_b) + (dz_b * dz_b);

                if (d_bone_out > 1.0) {
                    // Scalp soft tissue: +35 HU
                    volume_grid_set(g, x, y, z, 35);
                    continue;
                }

                // Check Skull Inner
                double dx_bi = ((double)x - cx) / rx_bone_in;
                double dy_bi = ((double)y - cy) / ry_bone_in;
                double dz_bi = ((double)z - cz) / rz_bone_in;
                double d_bone_in = (dx_bi * dx_bi) + (dy_bi * dy_bi) + (dz_bi * dz_bi);

                if (d_bone_in > 1.0) {
                    // Cortical Bone: +950 HU
                    volume_grid_set(g, x, y, z, 950);
                    continue;
                }

                // Inside Brain cavity
                // Check Ventricles (CSF: +15 HU)
                // Left & Right lateral ventricles
                double lx = (cx - (double)dim_x * 0.08);
                double rx = (cx + (double)dim_x * 0.08);
                double vy = cy - (double)dim_y * 0.05;

                double d_v_left = pow((double)x - lx, 2) / pow(dim_x * 0.04, 2) +
                                  pow((double)y - vy, 2) / pow(dim_y * 0.12, 2) +
                                  pow((double)z - cz, 2) / pow(dim_z * 0.15, 2);

                double d_v_right = pow((double)x - rx, 2) / pow(dim_x * 0.04, 2) +
                                   pow((double)y - vy, 2) / pow(dim_y * 0.12, 2) +
                                   pow((double)z - cz, 2) / pow(dim_z * 0.15, 2);

                if (d_v_left <= 1.0 || d_v_right <= 1.0) {
                    // Ventricle Cerebrospinal fluid (CSF)
                    volume_grid_set(g, x, y, z, 15);
                    continue;
                }

                // Frontal Sinuses (Air cavity: -950 HU)
                double sy_sinus = cy + (double)dim_y * 0.28;
                double sz_sinus = cz - (double)dim_z * 0.15;
                double d_sinus = pow((double)x - cx, 2) / pow(dim_x * 0.08, 2) +
                                 pow((double)y - sy_sinus, 2) / pow(dim_y * 0.06, 2) +
                                 pow((double)z - sz_sinus, 2) / pow(dim_z * 0.10, 2);

                if (d_sinus <= 1.0) {
                    // Sinus air
                    volume_grid_set(g, x, y, z, -950);
                    continue;
                }

                // Brain parenchyma: differential contrast between Gray Matter (+40 HU) and White Matter (+30 HU)
                double dist_to_in = sqrt(d_bone_in);
                int16_t hu = (dist_to_in > 0.75) ? 42 : 32;

                volume_grid_set(g, x, y, z, hu);
            }
        }
    }

    return g;
}
