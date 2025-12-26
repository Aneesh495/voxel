#include "dicomweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of PNG encoder
uint8_t *png_encode_grayscale(const uint8_t *pixels, size_t width, size_t height, size_t *out_png_len);

char *dicomweb_wado_get_study_metadata(const storage_hierarchy_t *h, const char *study_uid) {
    storage_study_t *st = storage_find_study(h, study_uid);
    if (!st) return strdup("[]");

    size_t cap = 4096;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return NULL;

    buf[len++] = '[';
    bool first = true;

    for (size_t ser = 0; ser < st->series_count; ser++) {
        const storage_series_t *s = st->series[ser];
        for (size_t inst = 0; inst < s->instance_count; inst++) {
            const storage_instance_t *instance = s->instances[inst];

            if (instance->file_path[0] != '\0') {
                dicom_dataset_t *ds = dicom_part10_read_file(instance->file_path);
                if (ds) {
                    char *json = dicom_dataset_to_json(ds);
                    if (json) {
                        if (!first) {
                            if (len + 2 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                            buf[len++] = ',';
                        }
                        first = false;

                        size_t jlen = strlen(json);
                        while (len + jlen + 4 >= cap) {
                            cap *= 2;
                            buf = (char *)realloc(buf, cap);
                        }
                        memcpy(buf + len, json, jlen);
                        len += jlen;
                        free(json);
                    }
                    dicom_dataset_destroy(ds);
                }
            }
        }
    }

    if (len + 2 >= cap) { cap += 8; buf = (char *)realloc(buf, cap); }
    buf[len++] = ']';
    buf[len] = '\0';
    return buf;
}

uint8_t *dicomweb_wado_render_instance_png(
    const storage_hierarchy_t *h,
    const char *study_uid,
    const char *series_uid,
    const char *instance_uid,
    window_level_t wl,
    size_t *out_png_len
) {
    storage_instance_t *inst = storage_find_instance(h, instance_uid);
    if (!inst || inst->file_path[0] == '\0') return NULL;

    dicom_dataset_t *ds = dicom_part10_read_file(inst->file_path);
    if (!ds) return NULL;

    dicom_element_t *pixel_elem = dicom_dataset_get(ds, TAG_PIXEL_DATA);
    if (!pixel_elem || !pixel_elem->data || pixel_elem->length == 0) {
        dicom_dataset_destroy(ds);
        return NULL;
    }

    uint16_t rows = 0, cols = 0;
    dicom_dataset_get_u16(ds, TAG_ROWS, &rows);
    dicom_dataset_get_u16(ds, TAG_COLUMNS, &cols);

    if (rows == 0 || cols == 0) {
        rows = 256;
        cols = 256;
    }

    double slope = 1.0, intercept = 0.0;
    dicom_dataset_get_f64(ds, TAG_RESCALE_SLOPE, &slope);
    dicom_dataset_get_f64(ds, TAG_RESCALE_INTERCEPT, &intercept);

    uint8_t *display_pixels = (uint8_t *)malloc((size_t)rows * (size_t)cols);
    if (!display_pixels) {
        dicom_dataset_destroy(ds);
        return NULL;
    }

    const int16_t *raw16 = (const int16_t *)pixel_elem->data;
    size_t total_pix = (size_t)rows * (size_t)cols;
    size_t max_raw = pixel_elem->length / 2;

    for (size_t i = 0; i < total_pix; i++) {
        double raw_val = (i < max_raw) ? (double)raw16[i] : -1000.0;
        double hu = raw_val * slope + intercept;
        display_pixels[i] = volume_hu_to_display(hu, wl);
    }

    uint8_t *png = png_encode_grayscale(display_pixels, cols, rows, out_png_len);
    free(display_pixels);
    dicom_dataset_destroy(ds);
    return png;
}

uint8_t *dicomweb_wado_render_mpr_png(
    const volume_grid_t *g,
    mpr_plane_t plane,
    size_t slice_idx,
    window_level_t wl,
    size_t *out_png_len
) {
    if (!g) return NULL;

    size_t max_pix = g->dim_x * g->dim_y;
    if (g->dim_x * g->dim_z > max_pix) max_pix = g->dim_x * g->dim_z;
    if (g->dim_y * g->dim_z > max_pix) max_pix = g->dim_y * g->dim_z;

    uint8_t *pixels = (uint8_t *)malloc(max_pix);
    if (!pixels) return NULL;

    size_t w = 0, h = 0;
    if (!volume_render_mpr_slice(g, plane, slice_idx, wl, pixels, &w, &h)) {
        free(pixels);
        return NULL;
    }

    uint8_t *png = png_encode_grayscale(pixels, w, h, out_png_len);
    free(pixels);
    return png;
}
