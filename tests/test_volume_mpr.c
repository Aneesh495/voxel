#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static void test_volume_grid_access(void) {
    printf("[TEST] Volume Grid Access & Out-of-Bounds Air Invariant... ");
    volume_grid_t *g = volume_grid_create(10, 10, 10, 1.0, 1.0, 1.0);
    assert(g != NULL);

    // Initialized to -1000 HU (air)
    assert(volume_grid_get(g, 0, 0, 0) == -1000);
    assert(volume_grid_get(g, 5, 5, 5) == -1000);

    // Set and get
    volume_grid_set(g, 3, 4, 5, 42);
    assert(volume_grid_get(g, 3, 4, 5) == 42);

    // Out of bounds returns air (-1000)
    assert(volume_grid_get(g, 10, 5, 5) == -1000);
    assert(volume_grid_get(g, 5, 100, 5) == -1000);

    volume_grid_destroy(g);
    printf("PASSED\n");
}

static void test_trilinear_interpolation(void) {
    printf("[TEST] Trilinear Interpolation Analytical Accuracy... ");
    // Create 4x4x4 grid with linear function: V(x, y, z) = 10*x + 20*y + 30*z
    volume_grid_t *g = volume_grid_create(4, 4, 4, 1.0, 1.0, 1.0);
    for (size_t z = 0; z < 4; z++) {
        for (size_t y = 0; y < 4; y++) {
            for (size_t x = 0; x < 4; x++) {
                int16_t val = (int16_t)(10 * x + 20 * y + 30 * z);
                volume_grid_set(g, x, y, z, val);
            }
        }
    }

    // Exact grid points
    double p0 = volume_sample_trilinear(g, 1.0, 2.0, 3.0);
    assert(fabs(p0 - (10 * 1 + 20 * 2 + 30 * 3)) < 1e-6);

    // Fractional midpoints: (1.5, 1.5, 1.5) -> 10*1.5 + 20*1.5 + 30*1.5 = 15 + 30 + 45 = 90
    double p_mid = volume_sample_trilinear(g, 1.5, 1.5, 1.5);
    assert(fabs(p_mid - 90.0) < 1e-6);

    // Quarter point: (0.25, 0.5, 0.75) -> 2.5 + 10 + 22.5 = 35.0
    double p_q = volume_sample_trilinear(g, 0.25, 0.5, 0.75);
    assert(fabs(p_q - 35.0) < 1e-6);

    volume_grid_destroy(g);
    printf("PASSED\n");
}

static void test_window_level_contrast(void) {
    printf("[TEST] Window/Level Contrast Transfer Function... ");
    window_level_t wl = { .center = 40.0, .width = 350.0 };

    // Below lower threshold -> 0
    double lower = wl.center - 0.5 - (wl.width - 1.0) / 2.0;
    assert(volume_hu_to_display(lower - 10.0, wl) == 0);

    // Above upper threshold -> 255
    double upper = wl.center - 0.5 + (wl.width - 1.0) / 2.0;
    assert(volume_hu_to_display(upper + 10.0, wl) == 255);

    // At exact center -> 127 or 128
    uint8_t mid = volume_hu_to_display(wl.center, wl);
    assert(mid >= 127 && mid <= 128);

    // Clinical preset sanity checks
    window_level_t bone = volume_get_preset(PRESET_BONE);
    assert(bone.center == 400.0 && bone.width == 2000.0);

    window_level_t lung = volume_get_preset(PRESET_LUNG);
    assert(lung.center == -600.0 && lung.width == 1500.0);

    printf("PASSED\n");
}

static void test_mpr_slice_rendering(void) {
    printf("[TEST] Multi-Planar Reconstruction Dimensions & Sampling... ");
    volume_grid_t *g = volume_generate_synthetic_head(64, 64, 32);
    assert(g != NULL);

    window_level_t wl = { .center = 40.0, .width = 350.0 };
    uint8_t pixels[64 * 64];
    size_t w = 0, h = 0;

    // Axial: width = 64, height = 64
    assert(volume_render_mpr_slice(g, MPR_PLANE_AXIAL, 16, wl, pixels, &w, &h));
    assert(w == 64 && h == 64);

    // Coronal: width = 64 (dim_x), height = 32 (dim_z)
    assert(volume_render_mpr_slice(g, MPR_PLANE_CORONAL, 32, wl, pixels, &w, &h));
    assert(w == 64 && h == 32);

    // Sagittal: width = 64 (dim_y), height = 32 (dim_z)
    assert(volume_render_mpr_slice(g, MPR_PLANE_SAGITTAL, 32, wl, pixels, &w, &h));
    assert(w == 64 && h == 32);

    volume_grid_destroy(g);
    printf("PASSED\n");
}

static void test_projections(void) {
    printf("[TEST] Ray-Cast MIP & MinIP Projections... ");
    volume_grid_t *g = volume_grid_create(4, 4, 4, 1.0, 1.0, 1.0);
    // Put a bright focal point (+1000 HU) at (2, 2, 2)
    volume_grid_set(g, 2, 2, 2, 1000);

    window_level_t wl = { .center = 400.0, .width = 2000.0 };
    uint8_t pixels[16];
    size_t w = 0, h = 0;

    // Maximum Intensity Projection along Z
    assert(volume_render_projection(g, MPR_PLANE_AXIAL, PROJECTION_MIP, 0, 4, wl, pixels, &w, &h));
    assert(w == 4 && h == 4);
    // Pixel (2, 2) must capture the 1000 HU peak value
    uint8_t peak_disp = volume_hu_to_display(1000.0, wl);
    assert(pixels[2 * 4 + 2] == peak_disp);

    // Background pixels (air -1000 HU)
    uint8_t bg_disp = volume_hu_to_display(-1000.0, wl);
    assert(pixels[0] == bg_disp);

    volume_grid_destroy(g);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Volumetric Reconstruction & MPR Unit Test Suite ===\n");
    test_volume_grid_access();
    test_trilinear_interpolation();
    test_window_level_contrast();
    test_mpr_slice_rendering();
    test_projections();
    printf("=== All Volumetric MPR Unit Tests Passed! ===\n\n");
    return 0;
}
