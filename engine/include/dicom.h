#ifndef VOXEL_DICOM_H
#define VOXEL_DICOM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// DICOM Value Representation enumeration per NEMA PS3.5
typedef enum {
    VR_AE = 0,  // Application Entity
    VR_AS = 1,  // Age String
    VR_AT = 2,  // Attribute Tag
    VR_CS = 3,  // Code String
    VR_DA = 4,  // Date
    VR_DS = 5,  // Decimal String
    VR_DT = 6,  // Date Time
    VR_FL = 7,  // Floating Point Single
    VR_FD = 8,  // Floating Point Double
    VR_IS = 9,  // Integer String
    VR_LO = 10, // Long String
    VR_LT = 11, // Long Text
    VR_OB = 12, // Other Byte
    VR_OD = 13, // Other Double
    VR_OF = 14, // Other Float
    VR_OL = 15, // Other Long
    VR_OW = 16, // Other Word
    VR_PN = 17, // Person Name
    VR_SH = 18, // Short String
    VR_SL = 19, // Signed Long
    VR_SQ = 20, // Sequence of Items
    VR_SS = 21, // Signed Short
    VR_ST = 22, // Short Text
    VR_SV = 23, // Signed 64-bit Very Long
    VR_TM = 24, // Time
    VR_UC = 25, // Unlimited Characters
    VR_UI = 26, // Unique Identifier
    VR_UL = 27, // Unsigned Long
    VR_UN = 28, // Unknown
    VR_UR = 29, // Universal Resource Identifier
    VR_US = 30, // Unsigned Short
    VR_UT = 31, // Unlimited Text
    VR_UV = 32  // Unsigned 64-bit Very Long
} dicom_vr_t;

// DICOM Tag structure (16-bit group, 16-bit element)
typedef struct {
    uint16_t group;
    uint16_t element;
} dicom_tag_t;

#define MAKE_TAG(g, e) ((dicom_tag_t){ (uint16_t)(g), (uint16_t)(e) })
#define TAG_U32(t) (((uint32_t)(t).group << 16) | (uint32_t)(t).element)
#define TAG_EQUAL(a, b) ((a).group == (b).group && (a).element == (b).element)
#define TAG_LESS(a, b) (((a).group != (b).group) ? ((a).group < (b).group) : ((a).element < (b).element))

// Standard DICOM Tag Constants
#define TAG_FILE_META_INFORMATION_GROUP_LENGTH MAKE_TAG(0x0002, 0x0000)
#define TAG_FILE_META_INFORMATION_VERSION      MAKE_TAG(0x0002, 0x0001)
#define TAG_MEDIA_STORAGE_SOP_CLASS_UID        MAKE_TAG(0x0002, 0x0002)
#define TAG_MEDIA_STORAGE_SOP_INSTANCE_UID     MAKE_TAG(0x0002, 0x0003)
#define TAG_TRANSFER_SYNTAX_UID                MAKE_TAG(0x0002, 0x0010)
#define TAG_IMPLEMENTATION_CLASS_UID           MAKE_TAG(0x0002, 0x0012)
#define TAG_IMPLEMENTATION_VERSION_NAME        MAKE_TAG(0x0002, 0x0013)

#define TAG_SPECIFIC_CHARACTER_SET             MAKE_TAG(0x0008, 0x0005)
#define TAG_IMAGE_TYPE                         MAKE_TAG(0x0008, 0x0008)
#define TAG_SOP_CLASS_UID                      MAKE_TAG(0x0008, 0x0016)
#define TAG_SOP_INSTANCE_UID                   MAKE_TAG(0x0008, 0x0018)
#define TAG_STUDY_DATE                         MAKE_TAG(0x0008, 0x0020)
#define TAG_SERIES_DATE                        MAKE_TAG(0x0008, 0x0021)
#define TAG_ACQUISITION_DATE                   MAKE_TAG(0x0008, 0x0022)
#define TAG_CONTENT_DATE                       MAKE_TAG(0x0008, 0x0023)
#define TAG_STUDY_TIME                         MAKE_TAG(0x0008, 0x0030)
#define TAG_SERIES_TIME                        MAKE_TAG(0x0008, 0x0031)
#define TAG_ACCESSION_NUMBER                   MAKE_TAG(0x0008, 0x0050)
#define TAG_QUERY_RETRIEVE_LEVEL               MAKE_TAG(0x0008, 0x0052)
#define TAG_MODALITY                           MAKE_TAG(0x0008, 0x0060)
#define TAG_MANUFACTURER                       MAKE_TAG(0x0008, 0x0070)
#define TAG_INSTITUTION_NAME                   MAKE_TAG(0x0008, 0x0080)
#define TAG_REFERRING_PHYSICIAN_NAME           MAKE_TAG(0x0008, 0x0090)
#define TAG_STUDY_DESCRIPTION                  MAKE_TAG(0x0008, 0x1030)
#define TAG_SERIES_DESCRIPTION                 MAKE_TAG(0x0008, 0x103E)
#define TAG_MANUFACTURER_MODEL_NAME            MAKE_TAG(0x0008, 0x1090)

#define TAG_PATIENT_NAME                       MAKE_TAG(0x0010, 0x0010)
#define TAG_PATIENT_ID                         MAKE_TAG(0x0010, 0x0020)
#define TAG_PATIENT_BIRTH_DATE                 MAKE_TAG(0x0010, 0x0030)
#define TAG_PATIENT_SEX                        MAKE_TAG(0x0010, 0x0040)
#define TAG_PATIENT_AGE                        MAKE_TAG(0x0010, 0x1010)

#define TAG_BODY_PART_EXAMINED                 MAKE_TAG(0x0018, 0x0015)
#define TAG_SLICE_THICKNESS                    MAKE_TAG(0x0018, 0x0050)
#define TAG_KVP                                MAKE_TAG(0x0018, 0x0060)
#define TAG_SPACING_BETWEEN_SLICES             MAKE_TAG(0x0018, 0x0088)

#define TAG_STUDY_INSTANCE_UID                 MAKE_TAG(0x0020, 0x000D)
#define TAG_SERIES_INSTANCE_UID                MAKE_TAG(0x0020, 0x000E)
#define TAG_STUDY_ID                           MAKE_TAG(0x0020, 0x0010)
#define TAG_SERIES_NUMBER                      MAKE_TAG(0x0020, 0x0011)
#define TAG_INSTANCE_NUMBER                    MAKE_TAG(0x0020, 0x0013)
#define TAG_IMAGE_POSITION_PATIENT             MAKE_TAG(0x0020, 0x0032)
#define TAG_IMAGE_ORIENTATION_PATIENT          MAKE_TAG(0x0020, 0x0037)
#define TAG_FRAME_OF_REFERENCE_UID             MAKE_TAG(0x0020, 0x0052)
#define TAG_SLICE_LOCATION                     MAKE_TAG(0x0020, 0x1041)

#define TAG_SAMPLES_PER_PIXEL                  MAKE_TAG(0x0028, 0x0002)
#define TAG_PHOTOMETRIC_INTERPRETATION         MAKE_TAG(0x0028, 0x0004)
#define TAG_ROWS                               MAKE_TAG(0x0028, 0x0010)
#define TAG_COLUMNS                            MAKE_TAG(0x0028, 0x0011)
#define TAG_PIXEL_SPACING                      MAKE_TAG(0x0028, 0x0030)
#define TAG_BITS_ALLOCATED                     MAKE_TAG(0x0028, 0x0100)
#define TAG_BITS_STORED                        MAKE_TAG(0x0028, 0x0101)
#define TAG_HIGH_BIT                           MAKE_TAG(0x0028, 0x0102)
#define TAG_PIXEL_REPRESENTATION               MAKE_TAG(0x0028, 0x0103)
#define TAG_WINDOW_CENTER                      MAKE_TAG(0x0028, 0x1050)
#define TAG_WINDOW_WIDTH                       MAKE_TAG(0x0028, 0x1051)
#define TAG_RESCALE_INTERCEPT                  MAKE_TAG(0x0028, 0x1052)
#define TAG_RESCALE_SLOPE                      MAKE_TAG(0x0028, 0x1053)

#define TAG_PIXEL_DATA                         MAKE_TAG(0x7FE0, 0x0010)
#define TAG_ITEM                               MAKE_TAG(0xFFFE, 0xE000)
#define TAG_ITEM_DELIMITATION_ITEM             MAKE_TAG(0xFFFE, 0xE00D)
#define TAG_SEQUENCE_DELIMITATION_ITEM         MAKE_TAG(0xFFFE, 0xE0DD)

// Well-known Transfer Syntax UIDs
#define UID_IMPLICIT_VR_LITTLE_ENDIAN "1.2.840.10008.1.2"
#define UID_EXPLICIT_VR_LITTLE_ENDIAN "1.2.840.10008.1.2.1"
#define UID_EXPLICIT_VR_BIG_ENDIAN    "1.2.840.10008.1.2.2"

// Well-known SOP Class UIDs
#define UID_VERIFICATION_SOP_CLASS    "1.2.840.10008.1.1"
#define UID_CT_IMAGE_STORAGE          "1.2.840.10008.5.1.4.1.1.2"
#define UID_MR_IMAGE_STORAGE          "1.2.840.10008.5.1.4.1.1.4"
#define UID_SECONDARY_CAPTURE_STORAGE "1.2.840.10008.5.1.4.1.1.7"
#define UID_PATIENT_ROOT_QR_FIND      "1.2.840.10008.5.1.4.1.2.1.1"
#define UID_STUDY_ROOT_QR_FIND        "1.2.840.10008.5.1.4.1.2.2.1"

#define VOXEL_IMPLEMENTATION_CLASS_UID "1.2.826.0.1.3680043.10.1142.1"
#define VOXEL_IMPLEMENTATION_VERSION_NAME "VOXEL_PACS_1_0"

// Forward declaration of dataset struct
struct dicom_dataset;

// DICOM Data Element
typedef struct dicom_element {
    dicom_tag_t tag;
    dicom_vr_t vr;
    uint32_t length;
    uint8_t *data;
    bool is_allocated;
    bool is_undefined_length;
    struct dicom_dataset **sequence_items;
    size_t sequence_count;
} dicom_element_t;

// DICOM Dataset
typedef struct dicom_dataset {
    dicom_element_t *elements;
    size_t count;
    size_t capacity;
} dicom_dataset_t;

// Dictionary entry
typedef struct {
    dicom_tag_t tag;
    dicom_vr_t vr;
    const char *keyword;
    const char *name;
} dicom_dict_entry_t;

// API functions
const char *dicom_vr_to_str(dicom_vr_t vr);
dicom_vr_t dicom_vr_from_str(const char *str);
bool dicom_vr_is_string(dicom_vr_t vr);
bool dicom_vr_has_explicit_32bit_len(dicom_vr_t vr);

const dicom_dict_entry_t *dicom_dict_lookup(dicom_tag_t tag);
dicom_vr_t dicom_dict_get_vr(dicom_tag_t tag);
const char *dicom_dict_get_keyword(dicom_tag_t tag);

dicom_dataset_t *dicom_dataset_create(void);
void dicom_dataset_destroy(dicom_dataset_t *ds);
bool dicom_dataset_insert(dicom_dataset_t *ds, dicom_element_t elem);
bool dicom_dataset_insert_string(dicom_dataset_t *ds, dicom_tag_t tag, dicom_vr_t vr, const char *str);
bool dicom_dataset_insert_u16(dicom_dataset_t *ds, dicom_tag_t tag, uint16_t val);
bool dicom_dataset_insert_u32(dicom_dataset_t *ds, dicom_tag_t tag, uint32_t val);
bool dicom_dataset_insert_f64(dicom_dataset_t *ds, dicom_tag_t tag, dicom_vr_t vr, double val);

dicom_element_t *dicom_dataset_get(const dicom_dataset_t *ds, dicom_tag_t tag);
const char *dicom_dataset_get_string(const dicom_dataset_t *ds, dicom_tag_t tag);
bool dicom_dataset_get_u16(const dicom_dataset_t *ds, dicom_tag_t tag, uint16_t *out_val);
bool dicom_dataset_get_u32(const dicom_dataset_t *ds, dicom_tag_t tag, uint32_t *out_val);
bool dicom_dataset_get_i32(const dicom_dataset_t *ds, dicom_tag_t tag, int32_t *out_val);
bool dicom_dataset_get_f64(const dicom_dataset_t *ds, dicom_tag_t tag, double *out_val);

// Part 10 Stream reader and writer
dicom_dataset_t *dicom_part10_read_file(const char *filepath);
dicom_dataset_t *dicom_part10_read_memory(const uint8_t *buffer, size_t length);
bool dicom_part10_write_file(const char *filepath, const dicom_dataset_t *ds);
uint8_t *dicom_part10_write_memory(const dicom_dataset_t *ds, size_t *out_len);

// JSON Export
char *dicom_dataset_to_json(const dicom_dataset_t *ds);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_DICOM_H
