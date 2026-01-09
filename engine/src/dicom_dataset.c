#include "dicom.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

dicom_dataset_t *dicom_dataset_create(void) {
    dicom_dataset_t *ds = (dicom_dataset_t *)calloc(1, sizeof(dicom_dataset_t));
    if (!ds) return NULL;
    ds->capacity = 16;
    ds->elements = (dicom_element_t *)calloc(ds->capacity, sizeof(dicom_element_t));
    if (!ds->elements) {
        free(ds);
        return NULL;
    }
    return ds;
}

void dicom_dataset_destroy(dicom_dataset_t *ds) {
    if (!ds) return;
    for (size_t i = 0; i < ds->count; i++) {
        if (ds->elements[i].is_allocated && ds->elements[i].data) {
            free(ds->elements[i].data);
        }
        if (ds->elements[i].sequence_items) {
            for (size_t s = 0; s < ds->elements[i].sequence_count; s++) {
                dicom_dataset_destroy(ds->elements[i].sequence_items[s]);
            }
            free(ds->elements[i].sequence_items);
        }
    }
    free(ds->elements);
    free(ds);
}

static int find_element_index(const dicom_dataset_t *ds, dicom_tag_t tag) {
    if (!ds || ds->count == 0) return -1;
    int low = 0;
    int high = (int)ds->count - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        dicom_tag_t mid_tag = ds->elements[mid].tag;

        if (TAG_EQUAL(mid_tag, tag)) {
            return mid;
        }
        if (TAG_LESS(mid_tag, tag)) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

dicom_element_t *dicom_dataset_get(const dicom_dataset_t *ds, dicom_tag_t tag) {
    int idx = find_element_index(ds, tag);
    if (idx >= 0) {
        return &ds->elements[idx];
    }
    return NULL;
}

bool dicom_dataset_insert(dicom_dataset_t *ds, dicom_element_t elem) {
    if (!ds) return false;

    int existing_idx = find_element_index(ds, elem.tag);
    if (existing_idx >= 0) {
        // Replace existing
        if (ds->elements[existing_idx].is_allocated && ds->elements[existing_idx].data) {
            free(ds->elements[existing_idx].data);
        }
        ds->elements[existing_idx] = elem;
        return true;
    }

    if (ds->count >= ds->capacity) {
        size_t new_cap = ds->capacity * 2;
        dicom_element_t *new_elems = (dicom_element_t *)realloc(ds->elements, new_cap * sizeof(dicom_element_t));
        if (!new_elems) return false;
        ds->elements = new_elems;
        ds->capacity = new_cap;
    }

    // Find insertion index to maintain sorted tag ordering
    size_t insert_idx = ds->count;
    for (size_t i = 0; i < ds->count; i++) {
        if (TAG_LESS(elem.tag, ds->elements[i].tag)) {
            insert_idx = i;
            break;
        }
    }

    // Shift elements
    for (size_t i = ds->count; i > insert_idx; i--) {
        ds->elements[i] = ds->elements[i - 1];
    }

    ds->elements[insert_idx] = elem;
    ds->count++;
    return true;
}

bool dicom_dataset_insert_string(dicom_dataset_t *ds, dicom_tag_t tag, dicom_vr_t vr, const char *str) {
    if (!ds || !str) return false;
    size_t len = strlen(str);
    size_t padded_len = (len % 2 != 0) ? (len + 1) : len;

    uint8_t *buf = (uint8_t *)calloc(1, padded_len + 1);
    if (!buf) return false;
    memcpy(buf, str, len);

    if (len % 2 != 0) {
        buf[len] = (vr == VR_UI) ? 0x00 : ' ';
    }

    dicom_element_t elem = {
        .tag = tag,
        .vr = vr,
        .length = (uint32_t)padded_len,
        .data = buf,
        .is_allocated = true,
        .is_undefined_length = false,
        .sequence_items = NULL,
        .sequence_count = 0
    };

    return dicom_dataset_insert(ds, elem);
}

bool dicom_dataset_insert_u16(dicom_dataset_t *ds, dicom_tag_t tag, uint16_t val) {
    uint8_t *buf = (uint8_t *)malloc(2);
    if (!buf) return false;
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);

    dicom_element_t elem = {
        .tag = tag,
        .vr = VR_US,
        .length = 2,
        .data = buf,
        .is_allocated = true,
        .is_undefined_length = false,
        .sequence_items = NULL,
        .sequence_count = 0
    };
    return dicom_dataset_insert(ds, elem);
}

bool dicom_dataset_insert_u32(dicom_dataset_t *ds, dicom_tag_t tag, uint32_t val) {
    uint8_t *buf = (uint8_t *)malloc(4);
    if (!buf) return false;
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
    buf[2] = (uint8_t)((val >> 16) & 0xFF);
    buf[3] = (uint8_t)((val >> 24) & 0xFF);

    dicom_element_t elem = {
        .tag = tag,
        .vr = VR_UL,
        .length = 4,
        .data = buf,
        .is_allocated = true,
        .is_undefined_length = false,
        .sequence_items = NULL,
        .sequence_count = 0
    };
    return dicom_dataset_insert(ds, elem);
}

bool dicom_dataset_insert_f64(dicom_dataset_t *ds, dicom_tag_t tag, dicom_vr_t vr, double val) {
    char str_buf[64];
    snprintf(str_buf, sizeof(str_buf), "%.6f", val);
    return dicom_dataset_insert_string(ds, tag, vr, str_buf);
}

const char *dicom_dataset_get_string(const dicom_dataset_t *ds, dicom_tag_t tag) {
    dicom_element_t *el = dicom_dataset_get(ds, tag);
    if (!el || !el->data || el->length == 0) return NULL;

    // Thread-local rotating ring buffer to allow multiple get_string calls in one expression
    static _Thread_local char s_ring[16][512];
    static _Thread_local size_t s_idx = 0;

    char *s_tmp = s_ring[s_idx % 16];
    s_idx++;

    size_t copy_len = (el->length < 511) ? el->length : 511;
    memcpy(s_tmp, el->data, copy_len);
    while (copy_len > 0 && (s_tmp[copy_len - 1] == ' ' || s_tmp[copy_len - 1] == '\0')) {
        copy_len--;
    }
    s_tmp[copy_len] = '\0';
    return s_tmp;
}

bool dicom_dataset_get_u16(const dicom_dataset_t *ds, dicom_tag_t tag, uint16_t *out_val) {
    dicom_element_t *el = dicom_dataset_get(ds, tag);
    if (!el || !el->data || el->length < 2) return false;
    *out_val = (uint16_t)el->data[0] | ((uint16_t)el->data[1] << 8);
    return true;
}

bool dicom_dataset_get_u32(const dicom_dataset_t *ds, dicom_tag_t tag, uint32_t *out_val) {
    dicom_element_t *el = dicom_dataset_get(ds, tag);
    if (!el || !el->data || el->length < 4) return false;
    *out_val = (uint32_t)el->data[0] |
               ((uint32_t)el->data[1] << 8) |
               ((uint32_t)el->data[2] << 16) |
               ((uint32_t)el->data[3] << 24);
    return true;
}

bool dicom_dataset_get_i32(const dicom_dataset_t *ds, dicom_tag_t tag, int32_t *out_val) {
    dicom_element_t *el = dicom_dataset_get(ds, tag);
    if (!el || !el->data || el->length == 0) return false;
    if (el->vr == VR_SL && el->length >= 4) {
        *out_val = (int32_t)((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8) | ((uint32_t)el->data[2] << 16) | ((uint32_t)el->data[3] << 24));
        return true;
    }
    if (el->vr == VR_UL && el->length >= 4) {
        *out_val = (int32_t)((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8) | ((uint32_t)el->data[2] << 16) | ((uint32_t)el->data[3] << 24));
        return true;
    }
    if (el->vr == VR_US && el->length >= 2) {
        *out_val = (int32_t)((uint16_t)el->data[0] | ((uint16_t)el->data[1] << 8));
        return true;
    }
    if (el->vr == VR_SS && el->length >= 2) {
        *out_val = (int32_t)(int16_t)((uint16_t)el->data[0] | ((uint16_t)el->data[1] << 8));
        return true;
    }
    const char *str = dicom_dataset_get_string(ds, tag);
    if (!str) return false;
    *out_val = (int32_t)atoi(str);
    return true;
}

bool dicom_dataset_get_f64(const dicom_dataset_t *ds, dicom_tag_t tag, double *out_val) {
    dicom_element_t *el = dicom_dataset_get(ds, tag);
    if (!el || !el->data || el->length == 0) return false;
    if (el->vr == VR_FD && el->length >= 8) {
        memcpy(out_val, el->data, sizeof(double));
        return true;
    }
    if (el->vr == VR_FL && el->length >= 4) {
        float f;
        memcpy(&f, el->data, sizeof(float));
        *out_val = (double)f;
        return true;
    }
    const char *str = dicom_dataset_get_string(ds, tag);
    if (!str) return false;
    *out_val = atof(str);
    return true;
}

char *dicom_dataset_to_json(const dicom_dataset_t *ds) {
    if (!ds) return NULL;
    size_t buf_cap = 4096;
    size_t buf_len = 0;
    char *buf = (char *)malloc(buf_cap);
    if (!buf) return NULL;

    buf[buf_len++] = '{';
    bool first = true;

    for (size_t i = 0; i < ds->count; i++) {
        const dicom_element_t *el = &ds->elements[i];
        if (el->tag.group == 0x7FE0) continue; // Exclude bulk pixel payload

        if (!first) {
            if (buf_len + 2 >= buf_cap) {
                buf_cap *= 2;
                buf = (char *)realloc(buf, buf_cap);
            }
            buf[buf_len++] = ',';
        }
        first = false;

        char tag_hex[9];
        snprintf(tag_hex, sizeof(tag_hex), "%04X%04X", el->tag.group, el->tag.element);

        char entry_buf[512];
        const char *vr_s = dicom_vr_to_str(el->vr);

        int written = 0;
        if (dicom_vr_is_string(el->vr) && el->data && el->length > 0) {
            char val_copy[256];
            size_t c_len = (el->length < 255) ? el->length : 255;
            memcpy(val_copy, el->data, c_len);
            while (c_len > 0 && (val_copy[c_len - 1] == ' ' || val_copy[c_len - 1] == '\0')) c_len--;
            val_copy[c_len] = '\0';

            if (el->vr == VR_PN) {
                written = snprintf(entry_buf, sizeof(entry_buf),
                    "\"%s\":{\"vr\":\"%s\",\"Value\":[{\"Alphabetic\":\"%s\"}]}",
                    tag_hex, vr_s, val_copy);
            } else {
                written = snprintf(entry_buf, sizeof(entry_buf),
                    "\"%s\":{\"vr\":\"%s\",\"Value\":[\"%s\"]}",
                    tag_hex, vr_s, val_copy);
            }
        } else if ((el->vr == VR_US || el->vr == VR_UL) && el->data && el->length >= 2) {
            uint32_t val = (el->length >= 4) ?
                ((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8) | ((uint32_t)el->data[2] << 16) | ((uint32_t)el->data[3] << 24)) :
                ((uint32_t)el->data[0] | ((uint32_t)el->data[1] << 8));
            written = snprintf(entry_buf, sizeof(entry_buf),
                "\"%s\":{\"vr\":\"%s\",\"Value\":[%u]}",
                tag_hex, vr_s, val);
        } else {
            written = snprintf(entry_buf, sizeof(entry_buf),
                "\"%s\":{\"vr\":\"%s\"}",
                tag_hex, vr_s);
        }

        if (written > 0) {
            while (buf_len + (size_t)written + 4 >= buf_cap) {
                buf_cap *= 2;
                buf = (char *)realloc(buf, buf_cap);
            }
            memcpy(buf + buf_len, entry_buf, written);
            buf_len += (size_t)written;
        }
    }

    if (buf_len + 2 >= buf_cap) {
        buf_cap += 16;
        buf = (char *)realloc(buf, buf_cap);
    }
    buf[buf_len++] = '}';
    buf[buf_len] = '\0';
    return buf;
}
