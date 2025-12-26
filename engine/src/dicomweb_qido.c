#include "dicomweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *dicomweb_qido_query_studies(const storage_hierarchy_t *h, const char *query_params) {
    if (!h) return strdup("[]");

    size_t cap = 2048;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return NULL;

    buf[len++] = '[';
    bool first = true;

    for (size_t p = 0; p < h->patient_count; p++) {
        const storage_patient_t *pat = h->patients[p];
        for (size_t s = 0; s < pat->study_count; s++) {
            const storage_study_t *st = pat->studies[s];

            if (!first) {
                if (len + 2 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                buf[len++] = ',';
            }
            first = false;

            char item[1024];
            int n = snprintf(item, sizeof(item),
                "{"
                "\"00100010\":{\"vr\":\"PN\",\"Value\":[{\"Alphabetic\":\"%s\"}]},"
                "\"00100020\":{\"vr\":\"LO\",\"Value\":[\"%s\"]},"
                "\"0020000D\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
                "\"00080020\":{\"vr\":\"DA\",\"Value\":[\"%s\"]},"
                "\"00080050\":{\"vr\":\"SH\",\"Value\":[\"%s\"]},"
                "\"00081030\":{\"vr\":\"LO\",\"Value\":[\"%s\"]}"
                "}",
                pat->patient_name, pat->patient_id, st->study_instance_uid,
                st->study_date, st->accession_number, st->study_description);

            if (n > 0) {
                while (len + (size_t)n + 4 >= cap) {
                    cap *= 2;
                    buf = (char *)realloc(buf, cap);
                }
                memcpy(buf + len, item, n);
                len += (size_t)n;
            }
        }
    }

    if (len + 2 >= cap) { cap += 8; buf = (char *)realloc(buf, cap); }
    buf[len++] = ']';
    buf[len] = '\0';
    return buf;
}

char *dicomweb_qido_query_series(const storage_hierarchy_t *h, const char *study_uid, const char *query_params) {
    storage_study_t *st = storage_find_study(h, study_uid);
    if (!st) return strdup("[]");

    size_t cap = 2048;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return NULL;

    buf[len++] = '[';
    bool first = true;

    for (size_t i = 0; i < st->series_count; i++) {
        const storage_series_t *s = st->series[i];
        if (!first) {
            if (len + 2 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = ',';
        }
        first = false;

        char item[512];
        int n = snprintf(item, sizeof(item),
            "{"
            "\"0020000D\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"0020000E\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"00080060\":{\"vr\":\"CS\",\"Value\":[\"%s\"]},"
            "\"00200011\":{\"vr\":\"IS\",\"Value\":[%d]},"
            "\"0008103E\":{\"vr\":\"LO\",\"Value\":[\"%s\"]}"
            "}",
            st->study_instance_uid, s->series_instance_uid, s->modality, s->series_number, s->series_description);

        if (n > 0) {
            while (len + (size_t)n + 4 >= cap) {
                cap *= 2;
                buf = (char *)realloc(buf, cap);
            }
            memcpy(buf + len, item, n);
            len += (size_t)n;
        }
    }

    if (len + 2 >= cap) { cap += 8; buf = (char *)realloc(buf, cap); }
    buf[len++] = ']';
    buf[len] = '\0';
    return buf;
}

char *dicomweb_qido_query_instances(const storage_hierarchy_t *h, const char *study_uid, const char *series_uid, const char *query_params) {
    storage_series_t *s = storage_find_series(h, study_uid, series_uid);
    if (!s) return strdup("[]");

    size_t cap = 2048;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return NULL;

    buf[len++] = '[';
    bool first = true;

    for (size_t i = 0; i < s->instance_count; i++) {
        const storage_instance_t *inst = s->instances[i];
        if (!first) {
            if (len + 2 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = ',';
        }
        first = false;

        char item[512];
        int n = snprintf(item, sizeof(item),
            "{"
            "\"0020000D\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"0020000E\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"00080018\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"00080016\":{\"vr\":\"UI\",\"Value\":[\"%s\"]},"
            "\"00200013\":{\"vr\":\"IS\",\"Value\":[%d]},"
            "\"00280010\":{\"vr\":\"US\",\"Value\":[%u]},"
            "\"00280011\":{\"vr\":\"US\",\"Value\":[%u]}"
            "}",
            study_uid, series_uid, inst->sop_instance_uid, inst->sop_class_uid,
            inst->instance_number, inst->rows, inst->cols);

        if (n > 0) {
            while (len + (size_t)n + 4 >= cap) {
                cap *= 2;
                buf = (char *)realloc(buf, cap);
            }
            memcpy(buf + len, item, n);
            len += (size_t)n;
        }
    }

    if (len + 2 >= cap) { cap += 8; buf = (char *)realloc(buf, cap); }
    buf[len++] = ']';
    buf[len] = '\0';
    return buf;
}
