#include "dimse.h"
#include <stdlib.h>
#include <string.h>

dimse_association_t *dimse_association_create(const char *called_ae) {
    dimse_association_t *assoc = (dimse_association_t *)calloc(1, sizeof(dimse_association_t));
    if (!assoc) return NULL;
    if (called_ae) {
        strncpy(assoc->called_ae, called_ae, 16);
    } else {
        strncpy(assoc->called_ae, "VOXEL_PACS", 16);
    }
    assoc->max_pdu_length = 65536;
    return assoc;
}

void dimse_association_destroy(dimse_association_t *assoc) {
    if (assoc) {
        free(assoc);
    }
}

bool dimse_process_associate_rq(
    dimse_association_t *assoc,
    const uint8_t *pdu_bytes,
    size_t pdu_len,
    uint8_t *out_ac,
    size_t *out_ac_len
) {
    if (!assoc || !pdu_bytes || pdu_len < 68 || !out_ac || !out_ac_len) return false;
    if (pdu_bytes[0] != PDU_A_ASSOCIATE_RQ) return false;

    // Read Called and Calling AE Titles
    char called[17] = {0};
    char calling[17] = {0};
    memcpy(called, pdu_bytes + 12, 16);
    memcpy(calling, pdu_bytes + 28, 16);

    // Strip trailing whitespace
    for (int i = 15; i >= 0 && called[i] == ' '; i--) called[i] = '\0';
    for (int i = 15; i >= 0 && calling[i] == ' '; i--) calling[i] = '\0';

    strncpy(assoc->called_ae, called, 16);
    strncpy(assoc->calling_ae, calling, 16);

    // Parse presentation context items starting at offset 74
    size_t cursor = 74;
    assoc->context_count = 0;

    while (cursor + 4 <= pdu_len && assoc->context_count < 32) {
        uint8_t item_type = pdu_bytes[cursor];
        uint16_t item_len = ((uint16_t)pdu_bytes[cursor + 2] << 8) | (uint16_t)pdu_bytes[cursor + 3];

        if (item_type == 0x20) { // Presentation Context Item (RQ)
            uint8_t ctx_id = pdu_bytes[cursor + 4];
            size_t sub_cursor = cursor + 8;
            size_t end_item = cursor + 4 + item_len;

            char abstract_syntax[64] = {0};

            while (sub_cursor + 4 <= end_item) {
                uint8_t sub_type = pdu_bytes[sub_cursor];
                uint16_t sub_len = ((uint16_t)pdu_bytes[sub_cursor + 2] << 8) | (uint16_t)pdu_bytes[sub_cursor + 3];

                if (sub_type == 0x30) { // Abstract Syntax
                    size_t c_len = (sub_len < 63) ? sub_len : 63;
                    memcpy(abstract_syntax, pdu_bytes + sub_cursor + 4, c_len);
                    abstract_syntax[c_len] = '\0';
                }
                sub_cursor += 4 + sub_len;
            }

            dimse_pres_context_t *ctx = &assoc->contexts[assoc->context_count++];
            ctx->id = ctx_id;
            strncpy(ctx->abstract_syntax, abstract_syntax, 63);
            strncpy(ctx->transfer_syntax, UID_EXPLICIT_VR_LITTLE_ENDIAN, 63);
            ctx->is_accepted = true; // Accept standard medical SOP classes
        }
        cursor += 4 + item_len;
    }

    // Build A-ASSOCIATE-AC response PDU
    size_t out_cur = 6; // Reserve 6 bytes for header

    // Protocol Version: 0x0001
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = 0x01;
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = 0x00;

    // Called AE Title (16 bytes)
    for (int i = 0; i < 16; i++) {
        out_ac[out_cur + i] = (i < (int)strlen(assoc->called_ae)) ? assoc->called_ae[i] : ' ';
    }
    out_cur += 16;

    // Calling AE Title (16 bytes)
    for (int i = 0; i < 16; i++) {
        out_ac[out_cur + i] = (i < (int)strlen(assoc->calling_ae)) ? assoc->calling_ae[i] : ' ';
    }
    out_cur += 16;

    // Reserved (32 bytes zero)
    memset(out_ac + out_cur, 0, 32);
    out_cur += 32;

    // Application Context (0x10)
    const char *app_ctx = "1.2.840.10008.3.1.1.1";
    uint16_t app_len = (uint16_t)strlen(app_ctx);
    out_ac[out_cur++] = 0x10;
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = (uint8_t)((app_len >> 8) & 0xFF);
    out_ac[out_cur++] = (uint8_t)(app_len & 0xFF);
    memcpy(out_ac + out_cur, app_ctx, app_len);
    out_cur += app_len;

    // Presentation Context (AC) items (0x21)
    for (size_t i = 0; i < assoc->context_count; i++) {
        const dimse_pres_context_t *ctx = &assoc->contexts[i];
        out_ac[out_cur++] = 0x21; // Type
        out_ac[out_cur++] = 0x00; // Reserved
        size_t len_pos = out_cur;
        out_cur += 2; // Reserve length

        out_ac[out_cur++] = ctx->id;
        out_ac[out_cur++] = 0x00;
        out_ac[out_cur++] = ctx->is_accepted ? 0x00 : 0x03; // Result
        out_ac[out_cur++] = 0x00;

        if (ctx->is_accepted) {
            uint16_t ts_len = (uint16_t)strlen(ctx->transfer_syntax);
            out_ac[out_cur++] = 0x40; // Transfer Syntax sub-item
            out_ac[out_cur++] = 0x00;
            out_ac[out_cur++] = (uint8_t)((ts_len >> 8) & 0xFF);
            out_ac[out_cur++] = (uint8_t)(ts_len & 0xFF);
            memcpy(out_ac + out_cur, ctx->transfer_syntax, ts_len);
            out_cur += ts_len;
        }

        uint16_t item_len = (uint16_t)(out_cur - len_pos - 2);
        out_ac[len_pos] = (uint8_t)((item_len >> 8) & 0xFF);
        out_ac[len_pos + 1] = (uint8_t)(item_len & 0xFF);
    }

    // User Information Item (0x50)
    out_ac[out_cur++] = 0x50;
    out_ac[out_cur++] = 0x00;
    size_t uinfo_len_pos = out_cur;
    out_cur += 2;

    // Maximum Length sub-item (0x51)
    out_ac[out_cur++] = 0x51;
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = 0x04;
    uint32_t max_pdu = assoc->max_pdu_length;
    out_ac[out_cur++] = (uint8_t)((max_pdu >> 24) & 0xFF);
    out_ac[out_cur++] = (uint8_t)((max_pdu >> 16) & 0xFF);
    out_ac[out_cur++] = (uint8_t)((max_pdu >> 8) & 0xFF);
    out_ac[out_cur++] = (uint8_t)(max_pdu & 0xFF);

    // Implementation Class UID sub-item (0x52)
    const char *impl_uid = VOXEL_IMPLEMENTATION_CLASS_UID;
    uint16_t impl_len = (uint16_t)strlen(impl_uid);
    out_ac[out_cur++] = 0x52;
    out_ac[out_cur++] = 0x00;
    out_ac[out_cur++] = (uint8_t)((impl_len >> 8) & 0xFF);
    out_ac[out_cur++] = (uint8_t)(impl_len & 0xFF);
    memcpy(out_ac + out_cur, impl_uid, impl_len);
    out_cur += impl_len;

    uint16_t uinfo_len = (uint16_t)(out_cur - uinfo_len_pos - 2);
    out_ac[uinfo_len_pos] = (uint8_t)((uinfo_len >> 8) & 0xFF);
    out_ac[uinfo_len_pos + 1] = (uint8_t)(uinfo_len & 0xFF);

    // Write PDU Header
    uint32_t total_pdu_len = (uint32_t)(out_cur - 6);
    out_ac[0] = PDU_A_ASSOCIATE_AC;
    out_ac[1] = 0x00;
    out_ac[2] = (uint8_t)((total_pdu_len >> 24) & 0xFF);
    out_ac[3] = (uint8_t)((total_pdu_len >> 16) & 0xFF);
    out_ac[4] = (uint8_t)((total_pdu_len >> 8) & 0xFF);
    out_ac[5] = (uint8_t)(total_pdu_len & 0xFF);

    *out_ac_len = out_cur;
    assoc->is_established = true;
    return true;
}
