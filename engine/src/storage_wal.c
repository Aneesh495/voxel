#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define WAL_MAGIC 0x56585741u // 'VXWA'

static void ensure_parent_dir(const char *filepath) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", filepath);
    char *slash = strrchr(tmp, '/');
    if (slash) {
        *slash = '\0';
        char *p = NULL;
        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(tmp, 0755);
                *p = '/';
            }
        }
        mkdir(tmp, 0755);
    }
}

static uint32_t calc_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        uint32_t byte_val = (crc ^ data[i]) & 0xFFu;
        for (int j = 0; j < 8; j++) {
            if (byte_val & 1) {
                byte_val = (byte_val >> 1) ^ 0xEDB88320u;
            } else {
                byte_val = byte_val >> 1;
            }
        }
        crc = (crc >> 8) ^ byte_val;
    }
    return ~crc;
}

bool storage_wal_append(
    const char *wal_path,
    uint64_t seq,
    uint16_t tx_type,
    const uint8_t *payload,
    size_t payload_len
) {
    if (!wal_path) return false;
    ensure_parent_dir(wal_path);
    FILE *f = fopen(wal_path, "ab");
    if (!f) return false;

    // Header: Magic (4), Seq (8), Type (2), Len (4)
    uint8_t hdr[18];
    uint32_t magic = WAL_MAGIC;
    memcpy(hdr, &magic, 4);
    memcpy(hdr + 4, &seq, 8);
    memcpy(hdr + 12, &tx_type, 2);
    uint32_t plen = (uint32_t)payload_len;
    memcpy(hdr + 14, &plen, 4);

    // Compute CRC over header and payload
    uint32_t crc = calc_crc32(hdr, 18);
    if (payload && payload_len > 0) {
        // Multi-part CRC: compute over combined stream
        size_t total = 18 + payload_len;
        uint8_t *combined = (uint8_t *)malloc(total);
        if (combined) {
            memcpy(combined, hdr, 18);
            memcpy(combined + 18, payload, payload_len);
            crc = calc_crc32(combined, total);
            free(combined);
        }
    }

    fwrite(hdr, 1, 18, f);
    if (payload && payload_len > 0) {
        fwrite(payload, 1, payload_len, f);
    }
    fwrite(&crc, 1, 4, f);
    fflush(f);
    fclose(f);
    return true;
}

bool storage_wal_replay(const char *wal_path, storage_hierarchy_t *h) {
    if (!wal_path || !h) return false;
    FILE *f = fopen(wal_path, "rb");
    if (!f) return true; // Empty / nonexistent log is valid

    uint8_t hdr[18];
    while (fread(hdr, 1, 18, f) == 18) {
        uint32_t magic;
        memcpy(&magic, hdr, 4);
        if (magic != WAL_MAGIC) break;

        uint64_t seq;
        uint16_t tx_type;
        uint32_t plen;
        memcpy(&seq, hdr + 4, 8);
        memcpy(&tx_type, hdr + 12, 2);
        memcpy(&plen, hdr + 14, 4);

        uint8_t *payload = NULL;
        if (plen > 0) {
            payload = (uint8_t *)malloc(plen);
            if (!payload || fread(payload, 1, plen, f) != plen) {
                if (payload) free(payload);
                break;
            }
        }

        uint32_t recorded_crc;
        if (fread(&recorded_crc, 1, 4, f) != 4) {
            if (payload) free(payload);
            break;
        }

        // Verify CRC
        size_t total = 18 + plen;
        uint8_t *combined = (uint8_t *)malloc(total);
        bool valid = false;
        if (combined) {
            memcpy(combined, hdr, 18);
            if (payload && plen > 0) memcpy(combined + 18, payload, plen);
            valid = (calc_crc32(combined, total) == recorded_crc);
            free(combined);
        }

        if (valid && payload && tx_type == 0x0001) {
            // Replay INSERT_INSTANCE: payload contains file path
            char path_buf[256] = {0};
            size_t c_len = (plen < 255) ? plen : 255;
            memcpy(path_buf, payload, c_len);
            dicom_dataset_t *ds = dicom_part10_read_file(path_buf);
            if (ds) {
                storage_ingest_dataset(h, ds, path_buf);
                dicom_dataset_destroy(ds);
            }
        }

        if (payload) free(payload);
    }

    fclose(f);
    return true;
}
