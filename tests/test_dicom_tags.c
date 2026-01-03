#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_tag_primitives(void) {
    printf("[TEST] DICOM Tag Primitives & Ordering... ");
    dicom_tag_t t1 = MAKE_TAG(0x0010, 0x0010);
    dicom_tag_t t2 = MAKE_TAG(0x0010, 0x0020);
    dicom_tag_t t3 = MAKE_TAG(0x0020, 0x000D);

    assert(TAG_EQUAL(t1, MAKE_TAG(0x0010, 0x0010)));
    assert(!TAG_EQUAL(t1, t2));
    assert(TAG_LESS(t1, t2));
    assert(TAG_LESS(t2, t3));
    assert(!TAG_LESS(t3, t1));

    uint32_t packed = TAG_U32(t1);
    assert(packed == 0x00100010);
    printf("PASSED\n");
}

static void test_vr_properties(void) {
    printf("[TEST] Value Representation Properties... ");
    assert(strcmp(dicom_vr_to_str(VR_PN), "PN") == 0);
    assert(strcmp(dicom_vr_to_str(VR_UI), "UI") == 0);
    assert(strcmp(dicom_vr_to_str(VR_OB), "OB") == 0);

    assert(dicom_vr_from_str("PN") == VR_PN);
    assert(dicom_vr_from_str("UI") == VR_UI);
    assert(dicom_vr_from_str("OB") == VR_OB);
    assert(dicom_vr_from_str("XX") == VR_UN);

    assert(dicom_vr_is_string(VR_PN));
    assert(dicom_vr_is_string(VR_LO));
    assert(!dicom_vr_is_string(VR_OB));
    assert(!dicom_vr_is_string(VR_US));

    assert(dicom_vr_has_explicit_32bit_len(VR_OB));
    assert(dicom_vr_has_explicit_32bit_len(VR_OW));
    assert(dicom_vr_has_explicit_32bit_len(VR_SQ));
    assert(!dicom_vr_has_explicit_32bit_len(VR_UI));
    assert(!dicom_vr_has_explicit_32bit_len(VR_US));
    printf("PASSED\n");
}

static void test_dictionary_lookups(void) {
    printf("[TEST] Dictionary Keyword & VR Lookups... ");
    const dicom_dict_entry_t *e_name = dicom_dict_lookup(TAG_PATIENT_NAME);
    assert(e_name != NULL);
    assert(e_name->vr == VR_PN);
    assert(strcmp(e_name->keyword, "PatientName") == 0);

    const dicom_dict_entry_t *e_mod = dicom_dict_lookup(TAG_MODALITY);
    assert(e_mod != NULL);
    assert(e_mod->vr == VR_CS);
    assert(strcmp(e_mod->keyword, "Modality") == 0);

    assert(dicom_dict_get_vr(TAG_ROWS) == VR_US);
    assert(strcmp(dicom_dict_get_keyword(TAG_STUDY_INSTANCE_UID), "StudyInstanceUID") == 0);
    printf("PASSED\n");
}

static void test_dataset_operations(void) {
    printf("[TEST] Dataset Insert, Sort Invariant, & Query... ");
    dicom_dataset_t *ds = dicom_dataset_create();
    assert(ds != NULL);

    // Insert out of tag order to test auto-sorting
    dicom_dataset_insert_string(ds, TAG_STUDY_INSTANCE_UID, VR_UI, "1.2.3.4.5");
    dicom_dataset_insert_string(ds, TAG_PATIENT_NAME, VR_PN, "DOE^JOHN");
    dicom_dataset_insert_string(ds, TAG_PATIENT_ID, VR_LO, "P12345");
    dicom_dataset_insert_u16(ds, TAG_ROWS, 512);

    assert(ds->count == 4);

    // Verify strictly ascending tag ordering
    for (size_t i = 1; i < ds->count; i++) {
        assert(TAG_LESS(ds->elements[i - 1].tag, ds->elements[i].tag));
    }

    // Verify value extraction
    const char *name = dicom_dataset_get_string(ds, TAG_PATIENT_NAME);
    assert(name != NULL);
    assert(strcmp(name, "DOE^JOHN") == 0);

    const char *pid = dicom_dataset_get_string(ds, TAG_PATIENT_ID);
    assert(pid != NULL);
    assert(strcmp(pid, "P12345") == 0);

    uint16_t rows = 0;
    assert(dicom_dataset_get_u16(ds, TAG_ROWS, &rows));
    assert(rows == 512);

    dicom_dataset_destroy(ds);
    printf("PASSED\n");
}

static void test_part10_roundtrip(void) {
    printf("[TEST] Part 10 Binary Serialization Round-Trip... ");
    dicom_dataset_t *src = dicom_dataset_create();

    dicom_dataset_insert_string(src, TAG_MEDIA_STORAGE_SOP_CLASS_UID, VR_UI, UID_CT_IMAGE_STORAGE);
    dicom_dataset_insert_string(src, TAG_MEDIA_STORAGE_SOP_INSTANCE_UID, VR_UI, "1.2.826.0.1.3680043.10.1");
    dicom_dataset_insert_string(src, TAG_TRANSFER_SYNTAX_UID, VR_UI, UID_EXPLICIT_VR_LITTLE_ENDIAN);
    dicom_dataset_insert_string(src, TAG_PATIENT_NAME, VR_PN, "ALICE^SMITH");
    dicom_dataset_insert_string(src, TAG_PATIENT_ID, VR_LO, "ID-999");
    dicom_dataset_insert_u16(src, TAG_ROWS, 256);
    dicom_dataset_insert_u16(src, TAG_COLUMNS, 256);

    size_t out_len = 0;
    uint8_t *raw_bytes = dicom_part10_write_memory(src, &out_len);
    assert(raw_bytes != NULL);
    assert(out_len > 132);

    // Verify preamble and magic
    assert(raw_bytes[128] == 'D' && raw_bytes[129] == 'I' && raw_bytes[130] == 'C' && raw_bytes[131] == 'M');

    // Deserialize back into a new dataset
    dicom_dataset_t *dst = dicom_part10_read_memory(raw_bytes, out_len);
    assert(dst != NULL);

    const char *name = dicom_dataset_get_string(dst, TAG_PATIENT_NAME);
    assert(name != NULL);
    assert(strcmp(name, "ALICE^SMITH") == 0);

    const char *pid = dicom_dataset_get_string(dst, TAG_PATIENT_ID);
    assert(pid != NULL);
    assert(strcmp(pid, "ID-999") == 0);

    uint16_t rows = 0, cols = 0;
    assert(dicom_dataset_get_u16(dst, TAG_ROWS, &rows));
    assert(assert(dicom_dataset_get_u16(dst, TAG_COLUMNS, &cols)), 1);
    assert(rows == 256 && cols == 256);

    free(raw_bytes);
    dicom_dataset_destroy(src);
    dicom_dataset_destroy(dst);
    printf("PASSED\n");
}

int main(void) {
    printf("=== DICOM Tags, VRs, & Part 10 Unit Test Suite ===\n");
    test_tag_primitives();
    test_vr_properties();
    test_dictionary_lookups();
    test_dataset_operations();
    test_part10_roundtrip();
    printf("=== All DICOM Tag & Part 10 Unit Tests Passed! ===\n\n");
    return 0;
}
