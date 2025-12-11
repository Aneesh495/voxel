#include "dimse.h"
#include <string.h>

static void write_elem_implicit(
    uint8_t *buf,
    size_t *cursor,
    uint16_t group,
    uint16_t element,
    const uint8_t *val,
    uint32_t val_len
) {
    buf[(*cursor)++] = (uint8_t)(group & 0xFF);
    buf[(*cursor)++] = (uint8_t)((group >> 8) & 0xFF);
    buf[(*cursor)++] = (uint8_t)(element & 0xFF);
    buf[(*cursor)++] = (uint8_t)((element >> 8) & 0xFF);

    buf[(*cursor)++] = (uint8_t)(val_len & 0xFF);
    buf[(*cursor)++] = (uint8_t)((val_len >> 8) & 0xFF);
    buf[(*cursor)++] = (uint8_t)((val_len >> 16) & 0xFF);
    buf[(*cursor)++] = (uint8_t)((val_len >> 24) & 0xFF);

    if (val && val_len > 0) {
        memcpy(buf + *cursor, val, val_len);
        *cursor += val_len;
    }
}

bool dimse_create_echo_rsp(
    uint16_t msg_id,
    const char *sop_class_uid,
    uint16_t status,
    uint8_t *out_bytes,
    size_t *out_len
) {
    if (!out_bytes || !out_len) return false;
    size_t cur = 0;

    // Affected SOP Class UID (0000, 0002)
    size_t sc_len = sop_class_uid ? strlen(sop_class_uid) : 0;
    if (sc_len % 2 != 0) sc_len++;
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0002, (const uint8_t *)(sop_class_uid ? sop_class_uid : UID_VERIFICATION_SOP_CLASS), (uint32_t)sc_len);

    // Command Field (0000, 0100) -> 0x8030
    uint8_t cmd_bytes[2] = { 0x30, 0x80 };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0100, cmd_bytes, 2);

    // Message ID Being Responded To (0000, 0120)
    uint8_t msg_bytes[2] = { (uint8_t)(msg_id & 0xFF), (uint8_t)((msg_id >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0120, msg_bytes, 2);

    // Command Data Set Type (0000, 0800) -> 0x0101 (Null)
    uint8_t no_data[2] = { 0x01, 0x01 };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0800, no_data, 2);

    // Status (0000, 0900)
    uint8_t status_bytes[2] = { (uint8_t)(status & 0xFF), (uint8_t)((status >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0900, status_bytes, 2);

    *out_len = cur;
    return true;
}

bool dimse_create_store_rsp(
    uint16_t msg_id,
    const char *sop_class_uid,
    const char *sop_instance_uid,
    uint16_t status,
    uint8_t *out_bytes,
    size_t *out_len
) {
    if (!out_bytes || !out_len) return false;
    size_t cur = 0;

    // Affected SOP Class UID (0000, 0002)
    size_t sc_len = sop_class_uid ? strlen(sop_class_uid) : 0;
    if (sc_len % 2 != 0) sc_len++;
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0002, (const uint8_t *)sop_class_uid, (uint32_t)sc_len);

    // Command Field (0000, 0100) -> 0x8001
    uint8_t cmd_bytes[2] = { 0x01, 0x80 };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0100, cmd_bytes, 2);

    // Message ID Being Responded To (0000, 0120)
    uint8_t msg_bytes[2] = { (uint8_t)(msg_id & 0xFF), (uint8_t)((msg_id >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0120, msg_bytes, 2);

    // Command Data Set Type (0000, 0800) -> 0x0101
    uint8_t no_data[2] = { 0x01, 0x01 };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0800, no_data, 2);

    // Status (0000, 0900)
    uint8_t status_bytes[2] = { (uint8_t)(status & 0xFF), (uint8_t)((status >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0900, status_bytes, 2);

    // Affected SOP Instance UID (0000, 1000)
    if (sop_instance_uid) {
        size_t si_len = strlen(sop_instance_uid);
        if (si_len % 2 != 0) si_len++;
        write_elem_implicit(out_bytes, &cur, 0x0000, 0x1000, (const uint8_t *)sop_instance_uid, (uint32_t)si_len);
    }

    *out_len = cur;
    return true;
}

bool dimse_create_find_rsp(
    uint16_t msg_id,
    const char *sop_class_uid,
    uint16_t status,
    bool has_dataset,
    uint8_t *out_bytes,
    size_t *out_len
) {
    if (!out_bytes || !out_len) return false;
    size_t cur = 0;

    // Affected SOP Class UID (0000, 0002)
    size_t sc_len = sop_class_uid ? strlen(sop_class_uid) : 0;
    if (sc_len % 2 != 0) sc_len++;
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0002, (const uint8_t *)sop_class_uid, (uint32_t)sc_len);

    // Command Field (0000, 0100) -> 0x8020
    uint8_t cmd_bytes[2] = { 0x20, 0x80 };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0100, cmd_bytes, 2);

    // Message ID Being Responded To (0000, 0120)
    uint8_t msg_bytes[2] = { (uint8_t)(msg_id & 0xFF), (uint8_t)((msg_id >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0120, msg_bytes, 2);

    // Command Data Set Type (0000, 0800)
    uint8_t data_type_bytes[2] = { (uint8_t)(has_dataset ? 0x01 : 0x01), (uint8_t)(has_dataset ? 0x00 : 0x01) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0800, data_type_bytes, 2);

    // Status (0000, 0900)
    uint8_t status_bytes[2] = { (uint8_t)(status & 0xFF), (uint8_t)((status >> 8) & 0xFF) };
    write_elem_implicit(out_bytes, &cur, 0x0000, 0x0900, status_bytes, 2);

    *out_len = cur;
    return true;
}
