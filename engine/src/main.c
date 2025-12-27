#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

static volatile bool g_running = true;

static void handle_sigint(int sig) {
    (void)sig;
    g_running = false;
}

static void print_banner(void) {
    printf("========================================================================\n");
    printf("  voxel: High-Performance DICOM 3.0 PACS & Volumetric Tomography Engine \n");
    printf("  Version %s | Dual DIMSE (11112) & DICOMweb (8042)\n", VOXEL_VERSION_STRING);
    printf("========================================================================\n\n");
}

static void print_usage(const char *prog) {
    print_banner();
    printf("Usage: %s <subcommand> [options]\n\n", prog);
    printf("Subcommands:\n");
    printf("  server            Start dual-protocol PACS daemon (DIMSE + DICOMweb)\n");
    printf("    --dicom-port P  DIMSE TCP port (default: 11112)\n");
    printf("    --http-port P   HTTP/1.1 DICOMweb port (default: 8042)\n");
    printf("    --ae-title T    Application Entity Title (default: VOXEL_PACS)\n");
    printf("    --storage-dir D Storage root directory (default: data)\n");
    printf("    --web-root W    Static clinical web workstation root (default: web)\n\n");
    printf("  dump <file.dcm>   Inspect and print DICOM Part 10 dataset tag tree\n");
    printf("  mpr <options>     Render 2D orthogonal MPR slice from volumetric scan\n");
    printf("    --input <path>  Input DICOM slice or volume series directory\n");
    printf("    --plane <name>  axial | coronal | sagittal (default: axial)\n");
    printf("    --slice <num>   Slice index along projection axis (default: 32)\n");
    printf("    --preset <name> soft | bone | lung | brain (default: soft)\n");
    printf("    --output <out>  Output PNG filename (default: mpr_slice.png)\n\n");
    printf("  phantom           Synthesize 3D volumetric clinical CT head phantom\n");
    printf("    --output-dir D  Destination directory (default: data/phantom)\n");
    printf("    --slices N      Number of axial slices to generate (default: 64)\n\n");
    printf("  version           Display engine capabilities and build info\n");
}

static int cmd_dump(const char *filepath) {
    if (!filepath) {
        fprintf(stderr, "Error: Please specify DICOM filepath to dump.\n");
        return 1;
    }

    printf("Reading DICOM Part 10 file: %s\n", filepath);
    dicom_dataset_t *ds = dicom_part10_read_file(filepath);
    if (!ds) {
        fprintf(stderr, "Error: Failed to parse DICOM dataset from %s\n", filepath);
        return 1;
    }

    printf("Dataset contains %zu data elements:\n", ds->count);
    printf("------------------------------------------------------------------------\n");
    printf("Tag         VR  Len     Keyword                        Value\n");
    printf("------------------------------------------------------------------------\n");

    for (size_t i = 0; i < ds->count; i++) {
        const dicom_element_t *el = &ds->elements[i];
        const char *vr_s = dicom_vr_to_str(el->vr);
        const char *kw = dicom_dict_get_keyword(el->tag);

        char val_summary[128] = {0};
        if (el->tag.group == 0x7FE0) {
            snprintf(val_summary, sizeof(val_summary), "<Pixel Data: %u bytes>", el->length);
        } else if (dicom_vr_is_string(el->vr) && el->data && el->length > 0) {
            size_t c_len = (el->length < sizeof(val_summary) - 1) ? el->length : (sizeof(val_summary) - 1);
            memcpy(val_summary, el->data, c_len);
            while (c_len > 0 && (val_summary[c_len - 1] == ' ' || val_summary[c_len - 1] == '\0')) c_len--;
            val_summary[c_len] = '\0';
        } else if ((el->vr == VR_US || el->vr == VR_UL) && el->data && el->length >= 2) {
            uint32_t num = (el->length >= 4) ?
                ((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8) | ((uint32_t)el->data[2] << 16) | ((uint32_t)el->data[3] << 24)) :
                ((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8));
            snprintf(val_summary, sizeof(val_summary), "%u", num);
        } else {
            snprintf(val_summary, sizeof(val_summary), "<binary: %u bytes>", el->length);
        }

        printf("(%04X,%04X)  %-2s  %-6u  %-30s %s\n",
            el->tag.group, el->tag.element, vr_s, el->length, kw, val_summary);
    }
    printf("------------------------------------------------------------------------\n");

    dicom_dataset_destroy(ds);
    return 0;
}

static int cmd_phantom(int argc, char **argv) {
    const char *out_dir = "data/phantom";
    size_t num_slices = 64;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (strcmp(argv[i], "--slices") == 0 && i + 1 < argc) {
            num_slices = (size_t)atoi(argv[++i]);
        }
    }

    printf("Synthesizing 3D Anatomical CT Head Phantom (%zu slices)...\n", num_slices);
    mkdir("data", 0755);
    mkdir(out_dir, 0755);

    volume_grid_t *g = volume_generate_synthetic_head(256, 256, num_slices);
    if (!g) {
        fprintf(stderr, "Error: Failed to generate synthetic head volume.\n");
        return 1;
    }

    const char *study_uid = "1.2.826.0.1.3680043.10.1142.20261107.1";
    const char *series_uid = "1.2.826.0.1.3680043.10.1142.20261107.1.1";

    for (size_t z = 0; z < num_slices; z++) {
        dicom_dataset_t *ds = dicom_dataset_create();

        char inst_uid[128];
        snprintf(inst_uid, sizeof(inst_uid), "%s.%zu", series_uid, z + 1);

        // File Meta Information
        dicom_dataset_insert_string(ds, TAG_MEDIA_STORAGE_SOP_CLASS_UID, VR_UI, UID_CT_IMAGE_STORAGE);
        dicom_dataset_insert_string(ds, TAG_MEDIA_STORAGE_SOP_INSTANCE_UID, VR_UI, inst_uid);
        dicom_dataset_insert_string(ds, TAG_TRANSFER_SYNTAX_UID, VR_UI, UID_EXPLICIT_VR_LITTLE_ENDIAN);
        dicom_dataset_insert_string(ds, TAG_IMPLEMENTATION_CLASS_UID, VR_UI, VOXEL_IMPLEMENTATION_CLASS_UID);
        dicom_dataset_insert_string(ds, TAG_IMPLEMENTATION_VERSION_NAME, VR_SH, VOXEL_IMPLEMENTATION_VERSION_NAME);

        // Clinical Tags
        dicom_dataset_insert_string(ds, TAG_SOP_CLASS_UID, VR_UI, UID_CT_IMAGE_STORAGE);
        dicom_dataset_insert_string(ds, TAG_SOP_INSTANCE_UID, VR_UI, inst_uid);
        dicom_dataset_insert_string(ds, TAG_PATIENT_NAME, VR_PN, "PHANTOM^ANATOMICAL^HEAD");
        dicom_dataset_insert_string(ds, TAG_PATIENT_ID, VR_LO, "VOXEL-PHANTOM-001");
        dicom_dataset_insert_string(ds, TAG_STUDY_DATE, VR_DA, "20261107");
        dicom_dataset_insert_string(ds, TAG_STUDY_TIME, VR_TM, "120000");
        dicom_dataset_insert_string(ds, TAG_ACCESSION_NUMBER, VR_SH, "ACC-2026-001");
        dicom_dataset_insert_string(ds, TAG_MODALITY, VR_CS, "CT");
        dicom_dataset_insert_string(ds, TAG_STUDY_DESCRIPTION, VR_LO, "3D Volumetric Head CT Reconstruction");
        dicom_dataset_insert_string(ds, TAG_SERIES_DESCRIPTION, VR_LO, "Axial 0.8mm Reconstruction");
        dicom_dataset_insert_string(ds, TAG_STUDY_INSTANCE_UID, VR_UI, study_uid);
        dicom_dataset_insert_string(ds, TAG_SERIES_INSTANCE_UID, VR_UI, series_uid);
        dicom_dataset_insert_u16(ds, TAG_SERIES_NUMBER, 1);
        dicom_dataset_insert_u16(ds, TAG_INSTANCE_NUMBER, (uint16_t)(z + 1));

        // Geometry
        dicom_dataset_insert_u16(ds, TAG_ROWS, (uint16_t)g->dim_y);
        dicom_dataset_insert_u16(ds, TAG_COLUMNS, (uint16_t)g->dim_x);
        dicom_dataset_insert_string(ds, TAG_PHOTOMETRIC_INTERPRETATION, VR_CS, "MONOCHROME2");
        dicom_dataset_insert_u16(ds, TAG_BITS_ALLOCATED, 16);
        dicom_dataset_insert_u16(ds, TAG_BITS_STORED, 16);
        dicom_dataset_insert_u16(ds, TAG_HIGH_BIT, 15);
        dicom_dataset_insert_u16(ds, TAG_PIXEL_REPRESENTATION, 1); // 2's complement signed

        dicom_dataset_insert_string(ds, TAG_PIXEL_SPACING, VR_DS, "0.800000\\0.800000");
        char ipp[128];
        snprintf(ipp, sizeof(ipp), "-102.400000\\-102.400000\\%.6f", (double)z * g->spacing_z);
        dicom_dataset_insert_string(ds, TAG_IMAGE_POSITION_PATIENT, VR_DS, ipp);
        dicom_dataset_insert_string(ds, TAG_IMAGE_ORIENTATION_PATIENT, VR_DS, "1.000000\\0.000000\\0.000000\\0.000000\\1.000000\\0.000000");

        // Rescale & Windowing
        dicom_dataset_insert_f64(ds, TAG_RESCALE_SLOPE, VR_DS, 1.0);
        dicom_dataset_insert_f64(ds, TAG_RESCALE_INTERCEPT, VR_DS, 0.0);
        dicom_dataset_insert_f64(ds, TAG_WINDOW_CENTER, VR_DS, 40.0);
        dicom_dataset_insert_f64(ds, TAG_WINDOW_WIDTH, VR_DS, 350.0);

        // Copy slice pixels
        size_t slice_pix = g->dim_x * g->dim_y;
        size_t slice_bytes = slice_pix * sizeof(int16_t);
        int16_t *pix_buf = (int16_t *)malloc(slice_bytes);
        if (pix_buf) {
            for (size_t y = 0; y < g->dim_y; y++) {
                for (size_t x = 0; x < g->dim_x; x++) {
                    pix_buf[y * g->dim_x + x] = volume_grid_get(g, x, y, z);
                }
            }
            dicom_element_t p_elem = {
                .tag = TAG_PIXEL_DATA,
                .vr = VR_OW,
                .length = (uint32_t)slice_bytes,
                .data = (uint8_t *)pix_buf,
                .is_allocated = true,
                .is_undefined_length = false,
                .sequence_items = NULL,
                .sequence_count = 0
            };
            dicom_dataset_insert(ds, p_elem);
        }

        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/slice_%03zu.dcm", out_dir, z + 1);
        dicom_part10_write_file(file_path, ds);
        dicom_dataset_destroy(ds);
    }

    volume_grid_destroy(g);
    printf("Successfully generated %zu DICOM Part 10 CT slices in: %s\n", num_slices, out_dir);
    return 0;
}

static int cmd_mpr(int argc, char **argv) {
    const char *plane_str = "axial";
    size_t slice_num = 32;
    const char *preset_str = "soft";
    const char *out_file = "mpr_slice.png";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--plane") == 0 && i + 1 < argc) plane_str = argv[++i];
        else if (strcmp(argv[i], "--slice") == 0 && i + 1 < argc) slice_num = (size_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--preset") == 0 && i + 1 < argc) preset_str = argv[++i];
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) out_file = argv[++i];
    }

    mpr_plane_t plane = MPR_PLANE_AXIAL;
    if (strcmp(plane_str, "coronal") == 0) plane = MPR_PLANE_CORONAL;
    else if (strcmp(plane_str, "sagittal") == 0) plane = MPR_PLANE_SAGITTAL;

    clinical_preset_t preset = PRESET_SOFT_TISSUE;
    if (strcmp(preset_str, "bone") == 0) preset = PRESET_BONE;
    else if (strcmp(preset_str, "lung") == 0) preset = PRESET_LUNG;
    else if (strcmp(preset_str, "brain") == 0) preset = PRESET_BRAIN;

    window_level_t wl = volume_get_preset(preset);

    printf("Generating 3D volume for MPR reconstruction...\n");
    volume_grid_t *g = volume_generate_synthetic_head(256, 256, 64);
    if (!g) {
        fprintf(stderr, "Error: Volume generation failed.\n");
        return 1;
    }

    size_t png_len = 0;
    uint8_t *png = dicomweb_wado_render_mpr_png(g, plane, slice_num, wl, &png_len);
    if (!png) {
        fprintf(stderr, "Error: MPR slice rendering failed.\n");
        volume_grid_destroy(g);
        return 1;
    }

    FILE *f = fopen(out_file, "wb");
    if (!f) {
        fprintf(stderr, "Error: Failed to open output file: %s\n", out_file);
        free(png);
        volume_grid_destroy(g);
        return 1;
    }

    fwrite(png, 1, png_len, f);
    fclose(f);
    free(png);
    volume_grid_destroy(g);

    printf("Rendered %s MPR slice #%zu (Preset: %s) -> %s (%zu bytes)\n",
        plane_str, slice_num, preset_str, out_file, png_len);
    return 0;
}

static int cmd_server(int argc, char **argv) {
    pacs_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.ae_title, "VOXEL_PACS", 16);
    cfg.dicom_port = 11112;
    cfg.http_port = 8042;
    strncpy(cfg.storage_dir, "data", 255);
    strncpy(cfg.web_root, "web", 255);

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dicom-port") == 0 && i + 1 < argc) cfg.dicom_port = (uint16_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--http-port") == 0 && i + 1 < argc) cfg.http_port = (uint16_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--ae-title") == 0 && i + 1 < argc) strncpy(cfg.ae_title, argv[++i], 16);
        else if (strcmp(argv[i], "--storage-dir") == 0 && i + 1 < argc) strncpy(cfg.storage_dir, argv[++i], 255);
        else if (strcmp(argv[i], "--web-root") == 0 && i + 1 < argc) strncpy(cfg.web_root, argv[++i], 255);
    }

    print_banner();
    printf("Starting voxel PACS daemon:\n");
    printf("  Calling AE Title: %s\n", cfg.ae_title);
    printf("  DIMSE TCP Port:   %u (C-ECHO, C-STORE, C-FIND)\n", cfg.dicom_port);
    printf("  DICOMweb Port:    %u (WADO-RS, QIDO-RS, Web Workstation)\n", cfg.http_port);
    printf("  Storage Path:     %s\n", cfg.storage_dir);
    printf("  Web Root:         %s\n", cfg.web_root);
    printf("  Web Viewer URL:   http://localhost:%u/\n\n", cfg.http_port);

    pacs_server_t *srv = pacs_server_create(&cfg);
    if (!srv) {
        fprintf(stderr, "Error: Failed to initialize PACS server structure.\n");
        return 1;
    }

    if (!pacs_server_start(srv)) {
        fprintf(stderr, "Error: Failed to bind listening sockets (check ports %u, %u).\n",
            cfg.dicom_port, cfg.http_port);
        pacs_server_destroy(srv);
        return 1;
    }

    printf("Daemon running. Press Ctrl+C to terminate gracefully.\n");
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    while (g_running) {
        sleep(1);
    }

    printf("\nShutting down PACS daemon...\n");
    pacs_server_destroy(srv);
    printf("Server stopped cleanly.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "server") == 0) {
        return cmd_server(argc, argv);
    } else if (strcmp(subcmd, "dump") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s dump <file.dcm>\n", argv[0]);
            return 1;
        }
        return cmd_dump(argv[2]);
    } else if (strcmp(subcmd, "mpr") == 0) {
        return cmd_mpr(argc, argv);
    } else if (strcmp(subcmd, "phantom") == 0 || strcmp(subcmd, "generate-sample") == 0) {
        return cmd_phantom(argc, argv);
    } else if (strcmp(subcmd, "version") == 0 || strcmp(subcmd, "--version") == 0) {
        printf("voxel version %s\n", VOXEL_VERSION_STRING);
        printf("Protocol Standard: DICOM 3.0 (NEMA PS3 / ISO 12052) & DICOMweb PS3.18\n");
        printf("Volumetric Engine: Trilinear MPR (Axial, Coronal, Sagittal) & Ray-Cast MIP\n");
        return 0;
    } else {
        print_usage(argv[0]);
        return 1;
    }
}
