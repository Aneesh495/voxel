#include "dicom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dicom_dataset_t *dicom_part10_read_memory(const uint8_t *buffer, size_t length) {
    if (!buffer || length < 132) return NULL;

    // Check for standard 128-byte preamble + "DICM" prefix
    size_t cursor = 0;
    if (buffer[128] == 'D' && buffer[129] == 'I' && buffer[130] == 'C' && buffer[131] == 'M') {
        cursor = 132;
    } else {
        cursor = 0;
    }

    dicom_dataset_t *ds = dicom_dataset_create();
    if (!ds) return NULL;

    bool is_explicit = true;
    bool is_big_endian = false;
    (void)is_big_endian;

    while (cursor + 6 <= length) {
        uint16_t group = (uint16_t)buffer[cursor] | ((uint16_t)buffer[cursor + 1] << 8);
        uint16_t element = (uint16_t)buffer[cursor + 2] | ((uint16_t)buffer[cursor + 3] << 8);
        dicom_tag_t tag = MAKE_TAG(group, element);

        // Group 0002 is strictly Explicit VR Little Endian
        if (group != 0x0002 && cursor >= 132) {
            // Check TransferSyntaxUID from dataset if available
            dicom_element_t *ts_elem = dicom_dataset_get(ds, TAG_TRANSFER_SYNTAX_UID);
            if (ts_elem && ts_elem->data) {
                const char *ts_str = dicom_dataset_get_string(ds, TAG_TRANSFER_SYNTAX_UID);
                if (ts_str && strcmp(ts_str, UID_IMPLICIT_VR_LITTLE_ENDIAN) == 0) {
                    is_explicit = false;
                } else if (ts_str && strcmp(ts_str, UID_EXPLICIT_VR_BIG_ENDIAN) == 0) {
                    is_big_endian = true;
                }
            }
        }

        dicom_vr_t vr = VR_UN;
        uint32_t val_len = 0;

        if (is_explicit || group == 0x0002) {
            if (cursor + 6 > length) break;
            char vr_chars[3] = { (char)buffer[cursor + 4], (char)buffer[cursor + 5], '\0' };
            vr = dicom_vr_from_str(vr_chars);

            if (dicom_vr_has_explicit_32bit_len(vr)) {
                if (cursor + 12 > length) break;
                val_len = (uint32_t)buffer[cursor + 8] |
                          ((uint32_t)buffer[cursor + 9] << 8) |
                          ((uint32_t)buffer[cursor + 10] << 16) |
                          ((uint32_t)buffer[cursor + 11] << 24);
                cursor += 12;
            } else {
                val_len = (uint32_t)buffer[cursor + 6] | ((uint32_t)buffer[cursor + 7] << 8);
                cursor += 8;
            }
        } else {
            // Implicit VR Little Endian: 4-byte length follows tag
            if (cursor + 8 > length) break;
            val_len = (uint32_t)buffer[cursor + 4] |
                      ((uint32_t)buffer[cursor + 5] << 8) |
                      ((uint32_t)buffer[cursor + 6] << 16) |
                      ((uint32_t)buffer[cursor + 7] << 24);
            vr = dicom_dict_get_vr(tag);
            cursor += 8;
        }

        if (val_len == 0xFFFFFFFF) {
            // Undefined length: scan for sequence delimitation item (FFFE, E0DD)
            size_t start_pos = cursor;
            bool found_delim = false;
            while (cursor + 8 <= length) {
                uint16_t dg = (uint16_t)buffer[cursor] | ((uint16_t)buffer[cursor + 1] << 8);
                uint16_t de = (uint16_t)buffer[cursor + 2] | ((uint16_t)buffer[cursor + 3] << 8);
                if (dg == 0xFFFE && (de == 0xE0DD || de == 0xE00D)) {
                    found_delim = true;
                    cursor += 8;
                    break;
                }
                cursor++;
            }
            size_t actual_len = found_delim ? (cursor - 8 - start_pos) : (length - start_pos);
            uint8_t *data_copy = (uint8_t *)malloc(actual_len + 1);
            if (data_copy) {
                memcpy(data_copy, buffer + start_pos, actual_len);
                data_copy[actual_len] = '\0';
                dicom_element_t el = {
                    .tag = tag,
                    .vr = vr,
                    .length = (uint32_t)actual_len,
                    .data = data_copy,
                    .is_allocated = true,
                    .is_undefined_length = true,
                    .sequence_items = NULL,
                    .sequence_count = 0
                };
                dicom_dataset_insert(ds, el);
            }
        } else {
            if (cursor + val_len > length) {
                val_len = (uint32_t)(length - cursor);
            }
            uint8_t *data_copy = (uint8_t *)malloc(val_len + 1);
            if (data_copy) {
                memcpy(data_copy, buffer + cursor, val_len);
                data_copy[val_len] = '\0';
                dicom_element_t el = {
                    .tag = tag,
                    .vr = vr,
                    .length = val_len,
                    .data = data_copy,
                    .is_allocated = true,
                    .is_undefined_length = false,
                    .sequence_items = NULL,
                    .sequence_count = 0
                };
                dicom_dataset_insert(ds, el);
            }
            cursor += val_len;
        }
    }

    return ds;
}

dicom_dataset_t *dicom_part10_read_file(const char *filepath) {
    if (!filepath) return NULL;
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return NULL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)size, f);
    fclose(f);

    dicom_dataset_t *ds = dicom_part10_read_memory(buf, read_bytes);
    free(buf);
    return ds;
}

uint8_t *dicom_part10_write_memory(const dicom_dataset_t *ds, size_t *out_len) {
    if (!ds || !out_len) return NULL;

    size_t cap = 132 + 4096;
    for (size_t i = 0; i < ds->count; i++) {
        cap += 12 + ds->elements[i].length;
    }

    uint8_t *buf = (uint8_t *)calloc(1, cap);
    if (!buf) return NULL;

    // 128 bytes preamble + "DICM"
    buf[128] = 'D';
    buf[129] = 'I';
    buf[130] = 'C';
    buf[131] = 'M';
    size_t cursor = 132;

    for (size_t i = 0; i < ds->count; i++) {
        const dicom_element_t *el = &ds->elements[i];

        // Ensure sufficient capacity
        if (cursor + 16 + el->length >= cap) {
            cap = cap * 2 + el->length;
            buf = (uint8_t *)realloc(buf, cap);
        }

        // Tag: group, element
        buf[cursor] = (uint8_t)(el->tag.group & 0xFF);
        buf[cursor + 1] = (uint8_t)((el->tag.group >> 8) & 0xFF);
        buf[cursor + 2] = (uint8_t)(el->tag.element & 0xFF);
        buf[cursor + 3] = (uint8_t)((el->tag.element >> 8) & 0xFF);
        cursor += 4;

        // VR string (2 bytes)
        const char *vr_s = dicom_vr_to_str(el->vr);
        buf[cursor] = (uint8_t)vr_s[0];
        buf[cursor + 1] = (uint8_t)vr_s[1];
        cursor += 2;

        if (dicom_vr_has_explicit_32bit_len(el->vr)) {
            buf[cursor] = 0x00;
            buf[cursor + 1] = 0x00;
            cursor += 2;

            buf[cursor] = (uint8_t)(el->length & 0xFF);
            buf[cursor + 1] = (uint8_t)((el->length >> 8) & 0xFF);
            buf[cursor + 2] = (uint8_t)((el->length >> 16) & 0xFF);
            buf[cursor + 3] = (uint8_t)((el->length >> 24) & 0xFF);
            cursor += 4;
        } else {
            buf[cursor] = (uint8_t)(el->length & 0xFF);
            buf[cursor + 1] = (uint8_t)((el->length >> 8) & 0xFF);
            cursor += 2;
        }

        if (el->data && el->length > 0) {
            memcpy(buf + cursor, el->data, el->length);
            cursor += el->length;
        }
    }

    *out_len = cursor;
    return buf;
}

bool dicom_part10_write_file(const char *filepath, const dicom_dataset_t *ds) {
    if (!filepath || !ds) return false;
    size_t out_len = 0;
    uint8_t *bytes = dicom_part10_write_memory(ds, &out_len);
    if (!bytes) return false;

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        free(bytes);
        return false;
    }

    size_t written = fwrite(bytes, 1, out_len, f);
    fclose(f);
    free(bytes);

    return written == out_len;
}
