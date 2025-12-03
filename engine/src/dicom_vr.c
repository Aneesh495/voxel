#include "dicom.h"
#include <string.h>

const char *dicom_vr_to_str(dicom_vr_t vr) {
    switch (vr) {
        case VR_AE: return "AE";
        case VR_AS: return "AS";
        case VR_AT: return "AT";
        case VR_CS: return "CS";
        case VR_DA: return "DA";
        case VR_DS: return "DS";
        case VR_DT: return "DT";
        case VR_FL: return "FL";
        case VR_FD: return "FD";
        case VR_IS: return "IS";
        case VR_LO: return "LO";
        case VR_LT: return "LT";
        case VR_OB: return "OB";
        case VR_OD: return "OD";
        case VR_OF: return "OF";
        case VR_OL: return "OL";
        case VR_OW: return "OW";
        case VR_PN: return "PN";
        case VR_SH: return "SH";
        case VR_SL: return "SL";
        case VR_SQ: return "SQ";
        case VR_SS: return "SS";
        case VR_ST: return "ST";
        case VR_SV: return "SV";
        case VR_TM: return "TM";
        case VR_UC: return "UC";
        case VR_UI: return "UI";
        case VR_UL: return "UL";
        case VR_UN: return "UN";
        case VR_UR: return "UR";
        case VR_US: return "US";
        case VR_UT: return "UT";
        case VR_UV: return "UV";
        default:    return "UN";
    }
}

dicom_vr_t dicom_vr_from_str(const char *str) {
    if (!str || strlen(str) < 2) return VR_UN;
    char c0 = str[0];
    char c1 = str[1];

    if (c0 == 'A' && c1 == 'E') return VR_AE;
    if (c0 == 'A' && c1 == 'S') return VR_AS;
    if (c0 == 'A' && c1 == 'T') return VR_AT;
    if (c0 == 'C' && c1 == 'S') return VR_CS;
    if (c0 == 'D' && c1 == 'A') return VR_DA;
    if (c0 == 'D' && c1 == 'S') return VR_DS;
    if (c0 == 'D' && c1 == 'T') return VR_DT;
    if (c0 == 'F' && c1 == 'L') return VR_FL;
    if (c0 == 'F' && c1 == 'D') return VR_FD;
    if (c0 == 'I' && c1 == 'S') return VR_IS;
    if (c0 == 'L' && c1 == 'O') return VR_LO;
    if (c0 == 'L' && c1 == 'T') return VR_LT;
    if (c0 == 'O' && c1 == 'B') return VR_OB;
    if (c0 == 'O' && c1 == 'D') return VR_OD;
    if (c0 == 'O' && c1 == 'F') return VR_OF;
    if (c0 == 'O' && c1 == 'L') return VR_OL;
    if (c0 == 'O' && c1 == 'W') return VR_OW;
    if (c0 == 'P' && c1 == 'N') return VR_PN;
    if (c0 == 'S' && c1 == 'H') return VR_SH;
    if (c0 == 'S' && c1 == 'L') return VR_SL;
    if (c0 == 'S' && c1 == 'Q') return VR_SQ;
    if (c0 == 'S' && c1 == 'S') return VR_SS;
    if (c0 == 'S' && c1 == 'T') return VR_ST;
    if (c0 == 'S' && c1 == 'V') return VR_SV;
    if (c0 == 'T' && c1 == 'M') return VR_TM;
    if (c0 == 'U' && c1 == 'C') return VR_UC;
    if (c0 == 'U' && c1 == 'I') return VR_UI;
    if (c0 == 'U' && c1 == 'L') return VR_UL;
    if (c0 == 'U' && c1 == 'N') return VR_UN;
    if (c0 == 'U' && c1 == 'R') return VR_UR;
    if (c0 == 'U' && c1 == 'S') return VR_US;
    if (c0 == 'U' && c1 == 'T') return VR_UT;
    if (c0 == 'U' && c1 == 'V') return VR_UV;

    return VR_UN;
}

bool dicom_vr_is_string(dicom_vr_t vr) {
    switch (vr) {
        case VR_AE: case VR_AS: case VR_CS: case VR_DA:
        case VR_DS: case VR_DT: case VR_IS: case VR_LO:
        case VR_LT: case VR_PN: case VR_SH: case VR_ST:
        case VR_TM: case VR_UC: case VR_UI: case VR_UR:
        case VR_UT:
            return true;
        default:
            return false;
    }
}

bool dicom_vr_has_explicit_32bit_len(dicom_vr_t vr) {
    switch (vr) {
        case VR_OB: case VR_OD: case VR_OF: case VR_OL:
        case VR_OW: case VR_SQ: case VR_UC: case VR_UN:
        case VR_UR: case VR_UT:
            return true;
        default:
            return false;
    }
}
