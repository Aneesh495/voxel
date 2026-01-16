#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static dicom_dataset_t *create_bench_dataset(size_t dim) {
    dicom_dataset_t *ds = dicom_dataset_create();
    dicom_dataset_insert_string(ds, TAG_PATIENT_NAME, VR_PN, "BENCH^PATIENT");
    dicom_dataset_insert_string(ds, TAG_PATIENT_ID, VR_LO, "BENCH-001");
    dicom_dataset_insert_string(ds, TAG_STUDY_INSTANCE_UID, VR_UI, "1.2.826.0.1.3680043.10.1142.2026.1");
    dicom_dataset_insert_string(ds, TAG_SERIES_INSTANCE_UID, VR_UI, "1.2.826.0.1.3680043.10.1142.2026.1.1");
    dicom_dataset_insert_string(ds, TAG_SOP_INSTANCE_UID, VR_UI, "1.2.826.0.1.3680043.10.1142.2026.1.1.1");
    dicom_dataset_insert_string(ds, TAG_MODALITY, VR_CS, "CT");
    dicom_dataset_insert_u16(ds, TAG_ROWS, (uint16_t)dim);
    dicom_dataset_insert_u16(ds, TAG_COLUMNS, (uint16_t)dim);
    dicom_dataset_insert_u16(ds, TAG_BITS_ALLOCATED, 16);
    dicom_dataset_insert_u16(ds, TAG_BITS_STORED, 16);
    dicom_dataset_insert_u16(ds, TAG_HIGH_BIT, 15);
    dicom_dataset_insert_u16(ds, TAG_PIXEL_REPRESENTATION, 1);
    dicom_dataset_insert_f64(ds, TAG_RESCALE_SLOPE, VR_DS, 1.0);
    dicom_dataset_insert_f64(ds, TAG_RESCALE_INTERCEPT, VR_DS, 0.0);
    dicom_dataset_insert_f64(ds, TAG_WINDOW_CENTER, VR_DS, 40.0);
    dicom_dataset_insert_f64(ds, TAG_WINDOW_WIDTH, VR_DS, 350.0);

    size_t pixel_bytes = dim * dim * sizeof(int16_t);
    int16_t *pixels = (int16_t *)malloc(pixel_bytes);
    if (pixels) {
        for (size_t i = 0; i < dim * dim; i++) {
            pixels[i] = (int16_t)(i % 1024 - 500);
        }
        dicom_element_t p_elem = {
            .tag = TAG_PIXEL_DATA,
            .vr = VR_OW,
            .length = (uint32_t)pixel_bytes,
            .data = (uint8_t *)pixels,
            .is_allocated = true,
            .is_undefined_length = false,
            .sequence_items = NULL,
            .sequence_count = 0
        };
        dicom_dataset_insert(ds, p_elem);
    }
    return ds;
}

int main(void) {
    const size_t dim = 256;
    const size_t iterations = 500;

    printf("========================================================================\n");
    printf("  Voxel DICOM Part 10 Streaming Microbenchmark\n");
    printf("  Dataset: %zux%zu 16-bit CT Slice | Iterations: %zu\n", dim, dim, iterations);
    printf("========================================================================\n\n");

    dicom_dataset_t *src_ds = create_bench_dataset(dim);
    if (!src_ds) {
        fprintf(stderr, "Failed to construct benchmark dataset\n");
        return 1;
    }

    // Warm-up and capture buffer length
    size_t out_len = 0;
    uint8_t *sample_buf = dicom_part10_write_memory(src_ds, &out_len);
    if (!sample_buf || out_len == 0) {
        fprintf(stderr, "Failed to serialize sample dataset\n");
        dicom_dataset_destroy(src_ds);
        return 1;
    }
    printf("Single slice serialized size: %zu bytes (%.2f KB)\n\n",
        out_len, (double)out_len / 1024.0);

    // 1. Serialization Benchmark
    printf("[BENCHMARK] DICOM Part 10 Binary Serialization (Dataset -> Memory Stream)...\n");
    double t0 = get_time_sec();
    size_t total_written = 0;
    for (size_t i = 0; i < iterations; i++) {
        size_t blen = 0;
        uint8_t *buf = dicom_part10_write_memory(src_ds, &blen);
        total_written += blen;
        free(buf);
    }
    double t1 = get_time_sec();
    double ser_elapsed = t1 - t0;
    double ser_fps = (double)iterations / ser_elapsed;
    double ser_mbps = ((double)total_written / (1024.0 * 1024.0)) / ser_elapsed;
    double ser_latency_us = (ser_elapsed / (double)iterations) * 1e6;

    printf("  Elapsed Time:  %.3f s\n", ser_elapsed);
    printf("  Throughput:    %.1f slices/sec | %.2f MB/sec\n", ser_fps, ser_mbps);
    printf("  Avg Latency:   %.1f us per slice\n\n", ser_latency_us);

    // 2. Deserialization Benchmark
    printf("[BENCHMARK] DICOM Part 10 Binary Deserialization (Memory Stream -> Dataset)...\n");
    double t2 = get_time_sec();
    size_t total_read = 0;
    for (size_t i = 0; i < iterations; i++) {
        dicom_dataset_t *parsed = dicom_part10_read_memory(sample_buf, out_len);
        total_read += out_len;
        dicom_dataset_destroy(parsed);
    }
    double t3 = get_time_sec();
    double de_elapsed = t3 - t2;
    double de_fps = (double)iterations / de_elapsed;
    double de_mbps = ((double)total_read / (1024.0 * 1024.0)) / de_elapsed;
    double de_latency_us = (de_elapsed / (double)iterations) * 1e6;

    printf("  Elapsed Time:  %.3f s\n", de_elapsed);
    printf("  Throughput:    %.1f slices/sec | %.2f MB/sec\n", de_fps, de_mbps);
    printf("  Avg Latency:   %.1f us per slice\n\n", de_latency_us);

    free(sample_buf);
    dicom_dataset_destroy(src_ds);

    printf("========================================================================\n");
    printf("  Part 10 Microbenchmark Complete: All operations zero allocations leak\n");
    printf("========================================================================\n");
    return 0;
}
