#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

storage_hierarchy_t *storage_hierarchy_create(const char *base_dir) {
    storage_hierarchy_t *h = (storage_hierarchy_t *)calloc(1, sizeof(storage_hierarchy_t));
    if (!h) return NULL;
    h->patient_capacity = 16;
    h->patients = (storage_patient_t **)calloc(h->patient_capacity, sizeof(storage_patient_t *));
    if (!h->patients) {
        free(h);
        return NULL;
    }
    strncpy(h->base_directory, base_dir ? base_dir : "data", 255);
    return h;
}

static void destroy_instance(storage_instance_t *inst) {
    if (inst) free(inst);
}

static void destroy_series(storage_series_t *s) {
    if (!s) return;
    for (size_t i = 0; i < s->instance_count; i++) {
        destroy_instance(s->instances[i]);
    }
    free(s->instances);
    free(s);
}

static void destroy_study(storage_study_t *st) {
    if (!st) return;
    for (size_t i = 0; i < st->series_count; i++) {
        destroy_series(st->series[i]);
    }
    free(st->series);
    free(st);
}

static void destroy_patient(storage_patient_t *p) {
    if (!p) return;
    for (size_t i = 0; i < p->study_count; i++) {
        destroy_study(p->studies[i]);
    }
    free(p->studies);
    free(p);
}

void storage_hierarchy_destroy(storage_hierarchy_t *h) {
    if (!h) return;
    for (size_t i = 0; i < h->patient_count; i++) {
        destroy_patient(h->patients[i]);
    }
    free(h->patients);
    free(h);
}

static storage_patient_t *find_or_create_patient(storage_hierarchy_t *h, const char *pat_id, const char *pat_name) {
    for (size_t i = 0; i < h->patient_count; i++) {
        if (strcmp(h->patients[i]->patient_id, pat_id) == 0) {
            return h->patients[i];
        }
    }
    if (h->patient_count >= h->patient_capacity) {
        size_t ncap = h->patient_capacity * 2;
        h->patients = (storage_patient_t **)realloc(h->patients, ncap * sizeof(storage_patient_t *));
        h->patient_capacity = ncap;
    }
    storage_patient_t *p = (storage_patient_t *)calloc(1, sizeof(storage_patient_t));
    strncpy(p->patient_id, pat_id ? pat_id : "UNKNOWN", 64);
    strncpy(p->patient_name, pat_name ? pat_name : "ANONYMOUS", 64);
    p->study_capacity = 8;
    p->studies = (storage_study_t **)calloc(p->study_capacity, sizeof(storage_study_t *));
    h->patients[h->patient_count++] = p;
    return p;
}

static storage_study_t *find_or_create_study(storage_patient_t *p, const char *study_uid) {
    for (size_t i = 0; i < p->study_count; i++) {
        if (strcmp(p->studies[i]->study_instance_uid, study_uid) == 0) {
            return p->studies[i];
        }
    }
    if (p->study_count >= p->study_capacity) {
        size_t ncap = p->study_capacity * 2;
        p->studies = (storage_study_t **)realloc(p->studies, ncap * sizeof(storage_study_t *));
        p->study_capacity = ncap;
    }
    storage_study_t *st = (storage_study_t *)calloc(1, sizeof(storage_study_t));
    strncpy(st->study_instance_uid, study_uid, 64);
    st->series_capacity = 8;
    st->series = (storage_series_t **)calloc(st->series_capacity, sizeof(storage_series_t *));
    p->studies[p->study_count++] = st;
    return st;
}

static storage_series_t *find_or_create_series(storage_study_t *st, const char *series_uid, const char *modality) {
    for (size_t i = 0; i < st->series_count; i++) {
        if (strcmp(st->series[i]->series_instance_uid, series_uid) == 0) {
            return st->series[i];
        }
    }
    if (st->series_count >= st->series_capacity) {
        size_t ncap = st->series_capacity * 2;
        st->series = (storage_series_t **)realloc(st->series, ncap * sizeof(storage_series_t *));
        st->series_capacity = ncap;
    }
    storage_series_t *s = (storage_series_t *)calloc(1, sizeof(storage_series_t));
    strncpy(s->series_instance_uid, series_uid, 64);
    strncpy(s->modality, modality ? modality : "OT", 16);
    s->instance_capacity = 64;
    s->instances = (storage_instance_t **)calloc(s->instance_capacity, sizeof(storage_instance_t *));
    st->series[st->series_count++] = s;
    return s;
}

bool storage_ingest_dataset(storage_hierarchy_t *h, const dicom_dataset_t *ds, const char *raw_file_path) {
    if (!h || !ds) return false;

    const char *pat_id = dicom_dataset_get_string(ds, TAG_PATIENT_ID);
    const char *pat_name = dicom_dataset_get_string(ds, TAG_PATIENT_NAME);
    const char *study_uid = dicom_dataset_get_string(ds, TAG_STUDY_INSTANCE_UID);
    const char *series_uid = dicom_dataset_get_string(ds, TAG_SERIES_INSTANCE_UID);
    const char *sop_uid = dicom_dataset_get_string(ds, TAG_SOP_INSTANCE_UID);
    const char *sop_class = dicom_dataset_get_string(ds, TAG_SOP_CLASS_UID);
    const char *modality = dicom_dataset_get_string(ds, TAG_MODALITY);

    if (!study_uid || !series_uid || !sop_uid) return false;

    storage_patient_t *p = find_or_create_patient(h, pat_id ? pat_id : "UNKNOWN", pat_name ? pat_name : "ANONYMOUS");
    storage_study_t *st = find_or_create_study(p, study_uid);

    const char *s_date = dicom_dataset_get_string(ds, TAG_STUDY_DATE);
    if (s_date) strncpy(st->study_date, s_date, 16);
    const char *s_desc = dicom_dataset_get_string(ds, TAG_STUDY_DESCRIPTION);
    if (s_desc) strncpy(st->study_description, s_desc, 64);
    const char *s_acc = dicom_dataset_get_string(ds, TAG_ACCESSION_NUMBER);
    if (s_acc) strncpy(st->accession_number, s_acc, 64);

    storage_series_t *s = find_or_create_series(st, series_uid, modality ? modality : "CT");
    const char *ser_desc = dicom_dataset_get_string(ds, TAG_SERIES_DESCRIPTION);
    if (ser_desc) strncpy(s->series_description, ser_desc, 64);

    // Check if instance already exists
    for (size_t i = 0; i < s->instance_count; i++) {
        if (strcmp(s->instances[i]->sop_instance_uid, sop_uid) == 0) {
            return true; // Already indexed
        }
    }

    if (s->instance_count >= s->instance_capacity) {
        size_t ncap = s->instance_capacity * 2;
        s->instances = (storage_instance_t **)realloc(s->instances, ncap * sizeof(storage_instance_t *));
        s->instance_capacity = ncap;
    }

    storage_instance_t *inst = (storage_instance_t *)calloc(1, sizeof(storage_instance_t));
    strncpy(inst->sop_instance_uid, sop_uid, 64);
    if (sop_class) strncpy(inst->sop_class_uid, sop_class, 64);
    if (raw_file_path) strncpy(inst->file_path, raw_file_path, 255);

    dicom_dataset_get_u16(ds, TAG_ROWS, &inst->rows);
    dicom_dataset_get_u16(ds, TAG_COLUMNS, &inst->cols);
    dicom_dataset_get_i32(ds, TAG_INSTANCE_NUMBER, &inst->instance_number);

    s->instances[s->instance_count++] = inst;
    return true;
}

storage_study_t *storage_find_study(const storage_hierarchy_t *h, const char *study_uid) {
    if (!h || !study_uid) return NULL;
    for (size_t p = 0; p < h->patient_count; p++) {
        for (size_t s = 0; s < h->patients[p]->study_count; s++) {
            if (strcmp(h->patients[p]->studies[s]->study_instance_uid, study_uid) == 0) {
                return h->patients[p]->studies[s];
            }
        }
    }
    return NULL;
}

storage_series_t *storage_find_series(const storage_hierarchy_t *h, const char *study_uid, const char *series_uid) {
    storage_study_t *st = storage_find_study(h, study_uid);
    if (!st || !series_uid) return NULL;
    for (size_t i = 0; i < st->series_count; i++) {
        if (strcmp(st->series[i]->series_instance_uid, series_uid) == 0) {
            return st->series[i];
        }
    }
    return NULL;
}

storage_instance_t *storage_find_instance(const storage_hierarchy_t *h, const char *sop_uid) {
    if (!h || !sop_uid) return NULL;
    for (size_t p = 0; p < h->patient_count; p++) {
        for (size_t s = 0; s < h->patients[p]->study_count; s++) {
            storage_study_t *st = h->patients[p]->studies[s];
            for (size_t ser = 0; ser < st->series_count; ser++) {
                storage_series_t *series = st->series[ser];
                for (size_t inst = 0; inst < series->instance_count; inst++) {
                    if (strcmp(series->instances[inst]->sop_instance_uid, sop_uid) == 0) {
                        return series->instances[inst];
                    }
                }
            }
        }
    }
    return NULL;
}

bool storage_cas_store(const char *base_dir, const uint8_t *data, size_t len, char *out_path, size_t out_path_max) {
    if (!data || len == 0 || !out_path) return false;
    mkdir(base_dir ? base_dir : "data", 0755);

    // Generate simple deterministic hash name
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }

    snprintf(out_path, out_path_max, "%s/%08x_%zu.dcm", base_dir ? base_dir : "data", hash, len);

    FILE *f = fopen(out_path, "wb");
    if (!f) return false;
    fwrite(data, 1, len, f);
    fclose(f);
    return true;
}
