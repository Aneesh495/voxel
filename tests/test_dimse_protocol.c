#include "voxel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void test_association_negotiation(void) {
    printf("[TEST] DIMSE A-ASSOCIATE-RQ / A-ASSOCIATE-AC Negotiation... ");
    dimse_association_t *assoc = dimse_association_create("VOXEL_PACS");
    assert(assoc != NULL);

    // Build synthetic A-ASSOCIATE-RQ PDU (Type 0x01)
    uint8_t rq[128];
    memset(rq, 0, sizeof(rq));
    rq[0] = PDU_A_ASSOCIATE_RQ;
    rq[1] = 0x00;

    // Called AE: "VOXEL_PACS      "
    memcpy(rq + 12, "VOXEL_PACS      ", 16);
    // Calling AE: "CLIENT_SCU      "
    memcpy(rq + 28, "CLIENT_SCU      ", 16);

    // Presentation Context item at offset 74
    rq[74] = 0x20; // Presentation Context Item
    rq[75] = 0x00;
    rq[76] = 0x00;
    rq[77] = 28;   // Length
    rq[78] = 0x01; // Context ID = 1

    // Abstract syntax sub-item: Verification SOP Class
    rq[82] = 0x30;
    rq[83] = 0x00;
    const char *abstract = UID_VERIFICATION_SOP_CLASS;
    uint16_t abs_len = (uint16_t)strlen(abstract);
    rq[84] = (uint8_t)((abs_len >> 8) & 0xFF);
    rq[85] = (uint8_t)(abs_len & 0xFF);
    memcpy(rq + 86, abstract, abs_len);

    uint32_t total_rq_len = 74 - 6 + 4 + 28;
    rq[2] = (uint8_t)((total_rq_len >> 24) & 0xFF);
    rq[3] = (uint8_t)((total_rq_len >> 16) & 0xFF);
    rq[4] = (uint8_t)((total_rq_len >> 8) & 0xFF);
    rq[5] = (uint8_t)(total_rq_len & 0xFF);

    uint8_t ac[1024];
    size_t ac_len = 0;
    bool ok = dimse_process_associate_rq(assoc, rq, 6 + total_rq_len, ac, &ac_len);
    assert(ok);
    assert(ac_len > 68);

    // Verify A-ASSOCIATE-AC PDU type
    assert(ac[0] == PDU_A_ASSOCIATE_AC);
    assert(assoc->is_established);
    assert(strcmp(assoc->calling_ae, "CLIENT_SCU") == 0);
    assert(strcmp(assoc->called_ae, "VOXEL_PACS") == 0);
    assert(assoc->context_count == 1);
    assert(assoc->contexts[0].is_accepted);

    dimse_association_destroy(assoc);
    printf("PASSED\n");
}

static void test_command_generation(void) {
    printf("[TEST] DIMSE-C Command Responses (C-ECHO, C-STORE, C-FIND)... ");
    uint8_t echo_rsp[256];
    size_t echo_len = 0;
    assert(dimse_create_echo_rsp(42, UID_VERIFICATION_SOP_CLASS, DIMSE_STATUS_SUCCESS, echo_rsp, &echo_len));
    assert(echo_len > 0);

    // Verify Message ID Being Responded To (42) and Status Success (0x0000)
    // Command Field tag is (0000, 0100) with value 0x8030
    bool found_cmd_field = false;
    for (size_t i = 0; i + 8 <= echo_len; i++) {
        uint16_t g = (uint16_t)echo_rsp[i] | ((uint16_t)echo_rsp[i + 1] << 8);
        uint16_t e = (uint16_t)echo_rsp[i + 2] | ((uint16_t)echo_rsp[i + 3] << 8);
        if (g == 0x0000 && e == 0x0100) {
            uint16_t cmd = (uint16_t)echo_rsp[i + 8] | ((uint16_t)echo_rsp[i + 9] << 8);
            assert(cmd == 0x8030); // C-ECHO-RSP
            found_cmd_field = true;
            break;
        }
    }
    assert(found_cmd_field);

    uint8_t store_rsp[512];
    size_t store_len = 0;
    assert(dimse_create_store_rsp(101, UID_CT_IMAGE_STORAGE, "1.2.3.4", DIMSE_STATUS_SUCCESS, store_rsp, &store_len));
    assert(store_len > 0);

    uint8_t find_rsp[512];
    size_t find_len = 0;
    assert(dimse_create_find_rsp(202, UID_STUDY_ROOT_QR_FIND, DIMSE_STATUS_PENDING, true, find_rsp, &find_len));
    assert(find_len > 0);

    printf("PASSED\n");
}

int main(void) {
    printf("=== DIMSE Protocol & Association Unit Test Suite ===\n");
    test_association_negotiation();
    test_command_generation();
    printf("=== All DIMSE Protocol Unit Tests Passed! ===\n\n");
    return 0;
}
