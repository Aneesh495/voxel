#ifndef VOXEL_DIMSE_H
#define VOXEL_DIMSE_H

#include "dicom.h"

#ifdef __cplusplus
extern "C" {
#endif

// PDU types per NEMA PS3.8
typedef enum {
    PDU_A_ASSOCIATE_RQ = 0x01,
    PDU_A_ASSOCIATE_AC = 0x02,
    PDU_A_ASSOCIATE_RJ = 0x03,
    PDU_P_DATA_TF      = 0x04,
    PDU_A_RELEASE_RQ   = 0x05,
    PDU_A_RELEASE_RP   = 0x06,
    PDU_A_ABORT        = 0x07
} dimse_pdu_type_t;

// Presentation context definition
typedef struct {
    uint8_t id;
    char abstract_syntax[64];
    char transfer_syntax[64];
    bool is_accepted;
} dimse_pres_context_t;

// Association structure
typedef struct {
    char calling_ae[17];
    char called_ae[17];
    uint32_t max_pdu_length;
    dimse_pres_context_t contexts[32];
    size_t context_count;
    bool is_established;
} dimse_association_t;

// DIMSE Command types
typedef enum {
    DIMSE_CMD_C_STORE_RQ = 0x0001,
    DIMSE_CMD_C_STORE_RSP = 0x8001,
    DIMSE_CMD_C_GET_RQ   = 0x0010,
    DIMSE_CMD_C_GET_RSP  = 0x8010,
    DIMSE_CMD_C_FIND_RQ  = 0x0020,
    DIMSE_CMD_C_FIND_RSP = 0x8020,
    DIMSE_CMD_C_MOVE_RQ  = 0x0021,
    DIMSE_CMD_C_MOVE_RSP = 0x8021,
    DIMSE_CMD_C_ECHO_RQ  = 0x0030,
    DIMSE_CMD_C_ECHO_RSP = 0x8030,
    DIMSE_CMD_C_CANCEL_RQ = 0x0FFF
} dimse_cmd_type_t;

// DIMSE Status codes
typedef enum {
    DIMSE_STATUS_SUCCESS = 0x0000,
    DIMSE_STATUS_PENDING = 0xFF00,
    DIMSE_STATUS_PENDING_WARNING = 0xFF01,
    DIMSE_STATUS_CANCEL  = 0xFE00,
    DIMSE_STATUS_REFUSED_RESOURCES = 0xA700,
    DIMSE_STATUS_ERROR_NO_MATCH = 0xA900,
    DIMSE_STATUS_ERROR_CANNOT_UNDERSTAND = 0xC000
} dimse_status_t;

// DIMSE API Functions
dimse_association_t *dimse_association_create(const char *called_ae);
void dimse_association_destroy(dimse_association_t *assoc);

bool dimse_process_associate_rq(dimse_association_t *assoc, const uint8_t *pdu_bytes, size_t pdu_len, uint8_t *out_ac, size_t *out_ac_len);
bool dimse_create_echo_rsp(uint16_t msg_id, const char *sop_class_uid, uint16_t status, uint8_t *out_bytes, size_t *out_len);
bool dimse_create_store_rsp(uint16_t msg_id, const char *sop_class_uid, const char *sop_instance_uid, uint16_t status, uint8_t *out_bytes, size_t *out_len);
bool dimse_create_find_rsp(uint16_t msg_id, const char *sop_class_uid, uint16_t status, bool has_dataset, uint8_t *out_bytes, size_t *out_len);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_DIMSE_H
