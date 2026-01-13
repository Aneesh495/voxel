#ifndef VOXEL_STORAGE_H
#define VOXEL_STORAGE_H

#include "dicom.h"

#ifdef __cplusplus
extern "C" {
#endif

// 4-tier relational model: Patient -> Study -> Series -> Instance
typedef struct {
    char sop_instance_uid[65];
    char sop_class_uid[65];
    int32_t instance_number;
    uint16_t rows;
    uint16_t cols;
    char file_path[256];
    size_t file_size;
} storage_instance_t;

typedef struct {
    char series_instance_uid[65];
    char modality[17];
    int32_t series_number;
    char series_description[65];
    char body_part[33];
    storage_instance_t **instances;
    size_t instance_count;
    size_t instance_capacity;
} storage_series_t;

typedef struct {
    char study_instance_uid[65];
    char study_date[17];
    char study_time[17];
    char accession_number[65];
    char study_description[65];
    char referring_physician[65];
    storage_series_t **series;
    size_t series_count;
    size_t series_capacity;
} storage_study_t;

typedef struct {
    char patient_id[65];
    char patient_name[65];
    char birth_date[17];
    char sex[9];
    storage_study_t **studies;
    size_t study_count;
    size_t study_capacity;
} storage_patient_t;

typedef struct {
    storage_patient_t **patients;
    size_t patient_count;
    size_t patient_capacity;
    char base_directory[256];
} storage_hierarchy_t;

// Storage API
storage_hierarchy_t *storage_hierarchy_create(const char *base_dir);
void storage_hierarchy_destroy(storage_hierarchy_t *h);
bool storage_hierarchy_scan_directory(storage_hierarchy_t *h, const char *dir_path);

bool storage_ingest_dataset(storage_hierarchy_t *h, const dicom_dataset_t *ds, const char *raw_file_path);
storage_study_t *storage_find_study(const storage_hierarchy_t *h, const char *study_uid);
storage_series_t *storage_find_series(const storage_hierarchy_t *h, const char *study_uid, const char *series_uid);
storage_instance_t *storage_find_instance(const storage_hierarchy_t *h, const char *sop_uid);

// Content-Addressable Storage (CAS)
bool storage_cas_store(const char *base_dir, const uint8_t *data, size_t len, char *out_path, size_t out_path_max);

// Write-Ahead Log (WAL)
bool storage_wal_append(const char *wal_path, uint64_t seq, uint16_t tx_type, const uint8_t *payload, size_t payload_len);
bool storage_wal_replay(const char *wal_path, storage_hierarchy_t *h);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_STORAGE_H
