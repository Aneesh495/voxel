#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

static void test_relational_hierarchy(void) {
    printf("[TEST] Relational Hierarchy (Patient -> Study -> Series -> Instance)... ");
    storage_hierarchy_t *h = storage_hierarchy_create("data/test_storage");
    assert(h != NULL);

    // Create synthetic dataset 1 (CT slice 1)
    dicom_dataset_t *ds1 = dicom_dataset_create();
    dicom_dataset_insert_string(ds1, TAG_PATIENT_ID, VR_LO, "PAT-001");
    dicom_dataset_insert_string(ds1, TAG_PATIENT_NAME, VR_PN, "DOE^JANE");
    dicom_dataset_insert_string(ds1, TAG_STUDY_INSTANCE_UID, VR_UI, "1.2.3.100");
    dicom_dataset_insert_string(ds1, TAG_SERIES_INSTANCE_UID, VR_UI, "1.2.3.100.1");
    dicom_dataset_insert_string(ds1, TAG_SOP_INSTANCE_UID, VR_UI, "1.2.3.100.1.1");
    dicom_dataset_insert_string(ds1, TAG_MODALITY, VR_CS, "CT");
    dicom_dataset_insert_u16(ds1, TAG_INSTANCE_NUMBER, 1);

    assert(storage_ingest_dataset(h, ds1, "dummy_path_1.dcm"));

    // Create synthetic dataset 2 (CT slice 2 in same series)
    dicom_dataset_t *ds2 = dicom_dataset_create();
    dicom_dataset_insert_string(ds2, TAG_PATIENT_ID, VR_LO, "PAT-001");
    dicom_dataset_insert_string(ds2, TAG_PATIENT_NAME, VR_PN, "DOE^JANE");
    dicom_dataset_insert_string(ds2, TAG_STUDY_INSTANCE_UID, VR_UI, "1.2.3.100");
    dicom_dataset_insert_string(ds2, TAG_SERIES_INSTANCE_UID, VR_UI, "1.2.3.100.1");
    dicom_dataset_insert_string(ds2, TAG_SOP_INSTANCE_UID, VR_UI, "1.2.3.100.1.2");
    dicom_dataset_insert_string(ds2, TAG_MODALITY, VR_CS, "CT");
    dicom_dataset_insert_u16(ds2, TAG_INSTANCE_NUMBER, 2);

    assert(storage_ingest_dataset(h, ds2, "dummy_path_2.dcm"));

    // Verify hierarchy structure
    assert(h->patient_count == 1);
    assert(h->patients[0]->study_count == 1);
    assert(h->patients[0]->studies[0]->series_count == 1);
    assert(h->patients[0]->studies[0]->series[0]->instance_count == 2);

    // Test find functions
    storage_study_t *st = storage_find_study(h, "1.2.3.100");
    assert(st != NULL);
    assert(strcmp(st->study_instance_uid, "1.2.3.100") == 0);

    storage_series_t *s = storage_find_series(h, "1.2.3.100", "1.2.3.100.1");
    assert(s != NULL);
    assert(strcmp(s->modality, "CT") == 0);

    storage_instance_t *inst1 = storage_find_instance(h, "1.2.3.100.1.1");
    assert(inst1 != NULL);
    assert(inst1->instance_number == 1);

    dicom_dataset_destroy(ds1);
    dicom_dataset_destroy(ds2);
    storage_hierarchy_destroy(h);
    printf("PASSED\n");
}

static void test_cas_and_wal(void) {
    printf("[TEST] Content-Addressable Storage & WAL Checksum Replay... ");
    const char *test_dir = "data/test_cas";
    const uint8_t test_data[] = "DICOM_TEST_BYTES";

    char path1[256];
    assert(storage_cas_store(test_dir, test_data, sizeof(test_data), path1, sizeof(path1)));
    assert(access(path1, F_OK) == 0);

    // Storing identical data produces identical CAS path (deduplication)
    char path2[256];
    assert(storage_cas_store(test_dir, test_data, sizeof(test_data), path2, sizeof(path2)));
    assert(strcmp(path1, path2) == 0);

    // WAL append and replay
    const char *wal_path = "data/test.wal";
    unlink(wal_path);

    assert(storage_wal_append(wal_path, 1, 0x0001, (const uint8_t *)"record_1", 8));
    assert(storage_wal_append(wal_path, 2, 0x0001, (const uint8_t *)"record_2", 8));

    storage_hierarchy_t *h = storage_hierarchy_create(test_dir);
    assert(storage_wal_replay(wal_path, h));

    storage_hierarchy_destroy(h);
    unlink(wal_path);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Storage Hierarchy, CAS, & WAL Unit Test Suite ===\n");
    test_relational_hierarchy();
    test_cas_and_wal();
    printf("=== All Storage Hierarchy Unit Tests Passed! ===\n\n");
    return 0;
}
