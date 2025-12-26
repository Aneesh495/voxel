#include "dicomweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool dicomweb_stow_ingest_multipart(
    storage_hierarchy_t *h,
    const uint8_t *body,
    size_t body_len,
    const char *content_type_header,
    size_t *out_stored_count
) {
    if (!h || !body || body_len == 0 || !out_stored_count) return false;
    *out_stored_count = 0;

    // Scan for multipart boundary or direct Part 10 dataset
    // Check if the body itself begins with or contains a DICM prefix
    for (size_t i = 0; i + 132 <= body_len; i++) {
        if (body[i + 128] == 'D' && body[i + 129] == 'I' && body[i + 130] == 'C' && body[i + 131] == 'M') {
            // Found a valid DICOM Part 10 file stream starting at offset i
            dicom_dataset_t *ds = dicom_part10_read_memory(body + i, body_len - i);
            if (ds) {
                char cas_path[256];
                if (storage_cas_store(h->base_directory, body + i, body_len - i, cas_path, sizeof(cas_path))) {
                    if (storage_ingest_dataset(h, ds, cas_path)) {
                        (*out_stored_count)++;
                    }
                }
                dicom_dataset_destroy(ds);
                return (*out_stored_count > 0);
            }
        }
    }

    // Direct dataset parsing attempt without preamble
    dicom_dataset_t *ds = dicom_part10_read_memory(body, body_len);
    if (ds) {
        char cas_path[256];
        if (storage_cas_store(h->base_directory, body, body_len, cas_path, sizeof(cas_path))) {
            if (storage_ingest_dataset(h, ds, cas_path)) {
                (*out_stored_count)++;
            }
        }
        dicom_dataset_destroy(ds);
        return (*out_stored_count > 0);
    }

    return false;
}
