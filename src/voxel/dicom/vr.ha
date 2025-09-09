// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::vr
// Complete Value Representation (VR) definitions, binary encoding rules,
// padding requirements, and validation functions per DICOM PS3.5.

export type dicom_vr = enum {
	AE = 0,  // Application Entity (16 chars max)
	AS = 1,  // Age String (4 chars: e.g. "024Y")
	AT = 2,  // Attribute Tag (4 bytes, 2x 16-bit integers)
	CS = 3,  // Code String (16 chars max, uppercase, digits, spaces, _)
	DA = 4,  // Date (8 chars: YYYYMMDD)
	DS = 5,  // Decimal String (16 chars max, floating-point text)
	DT = 6,  // Date Time (26 chars max: YYYYMMDDHHMMSS.FFFFFF&ZZXX)
	FL = 7,  // Floating Point Single (4 bytes IEEE 754)
	FD = 8,  // Floating Point Double (8 bytes IEEE 754)
	IS = 9,  // Integer String (12 chars max: signed integer text)
	LO = 10, // Long String (64 chars max)
	LT = 11, // Long Text (10240 chars max)
	OB = 12, // Other Byte (byte stream)
	OD = 13, // Other Double (8-byte float stream)
	OF = 14, // Other Float (4-byte float stream)
	OL = 15, // Other Long (4-byte integer stream)
	OW = 16, // Other Word (16-bit word stream)
	PN = 17, // Person Name (Family^Given^Middle^Prefix^Suffix)
	SH = 18, // Short String (16 chars max)
	SL = 19, // Signed Long (4 bytes signed integer)
	SQ = 20, // Sequence of Items
	SS = 21, // Signed Short (2 bytes signed integer)
	ST = 22, // Short Text (1024 chars max)
	SV = 23, // Signed 64-bit Very Long (8 bytes)
	TM = 24, // Time (16 chars max: HHMMSS.FFFFFF)
	UC = 25, // Unlimited Characters
	UI = 26, // Unique Identifier / OID (64 chars max: [0-9.])
	UL = 27, // Unsigned Long (4 bytes unsigned integer)
	UN = 28, // Unknown (arbitrary byte stream)
	UR = 29, // Universal Resource Identifier
	US = 30, // Unsigned Short (2 bytes unsigned integer)
	UT = 31, // Unlimited Text
	UV = 32, // Unsigned 64-bit Very Long (8 bytes)
};

// Returns the 2-character ASCII mnemonic for a given Value Representation.
export fn vr_to_str(vr: dicom_vr) str = {
	switch (vr) {
	case dicom_vr::AE => return "AE";
	case dicom_vr::AS => return "AS";
	case dicom_vr::AT => return "AT";
	case dicom_vr::CS => return "CS";
	case dicom_vr::DA => return "DA";
	case dicom_vr::DS => return "DS";
	case dicom_vr::DT => return "DT";
	case dicom_vr::FL => return "FL";
	case dicom_vr::FD => return "FD";
	case dicom_vr::IS => return "IS";
	case dicom_vr::LO => return "LO";
	case dicom_vr::LT => return "LT";
	case dicom_vr::OB => return "OB";
	case dicom_vr::OD => return "OD";
	case dicom_vr::OF => return "OF";
	case dicom_vr::OL => return "OL";
	case dicom_vr::OW => return "OW";
	case dicom_vr::PN => return "PN";
	case dicom_vr::SH => return "SH";
	case dicom_vr::SL => return "SL";
	case dicom_vr::SQ => return "SQ";
	case dicom_vr::SS => return "SS";
	case dicom_vr::ST => return "ST";
	case dicom_vr::SV => return "SV";
	case dicom_vr::TM => return "TM";
	case dicom_vr::UC => return "UC";
	case dicom_vr::UI => return "UI";
	case dicom_vr::UL => return "UL";
	case dicom_vr::UN => return "UN";
	case dicom_vr::UR => return "UR";
	case dicom_vr::US => return "US";
	case dicom_vr::UT => return "UT";
	case dicom_vr::UV => return "UV";
	};
};

// Parses a 2-character ASCII mnemonic into a dicom_vr enum value.
export fn vr_from_chars(c0: u8, c1: u8) (dicom_vr | void) = {
	if (c0 == 'A' && c1 == 'E') return dicom_vr::AE;
	if (c0 == 'A' && c1 == 'S') return dicom_vr::AS;
	if (c0 == 'A' && c1 == 'T') return dicom_vr::AT;
	if (c0 == 'C' && c1 == 'S') return dicom_vr::CS;
	if (c0 == 'D' && c1 == 'A') return dicom_vr::DA;
	if (c0 == 'D' && c1 == 'S') return dicom_vr::DS;
	if (c0 == 'D' && c1 == 'T') return dicom_vr::DT;
	if (c0 == 'F' && c1 == 'L') return dicom_vr::FL;
	if (c0 == 'F' && c1 == 'D') return dicom_vr::FD;
	if (c0 == 'I' && c1 == 'S') return dicom_vr::IS;
	if (c0 == 'L' && c1 == 'O') return dicom_vr::LO;
	if (c0 == 'L' && c1 == 'T') return dicom_vr::LT;
	if (c0 == 'O' && c1 == 'B') return dicom_vr::OB;
	if (c0 == 'O' && c1 == 'D') return dicom_vr::OD;
	if (c0 == 'O' && c1 == 'F') return dicom_vr::OF;
	if (c0 == 'O' && c1 == 'L') return dicom_vr::OL;
	if (c0 == 'O' && c1 == 'W') return dicom_vr::OW;
	if (c0 == 'P' && c1 == 'N') return dicom_vr::PN;
	if (c0 == 'S' && c1 == 'H') return dicom_vr::SH;
	if (c0 == 'S' && c1 == 'L') return dicom_vr::SL;
	if (c0 == 'S' && c1 == 'Q') return dicom_vr::SQ;
	if (c0 == 'S' && c1 == 'S') return dicom_vr::SS;
	if (c0 == 'S' && c1 == 'T') return dicom_vr::ST;
	if (c0 == 'S' && c1 == 'V') return dicom_vr::SV;
	if (c0 == 'T' && c1 == 'M') return dicom_vr::TM;
	if (c0 == 'U' && c1 == 'C') return dicom_vr::UC;
	if (c0 == 'U' && c1 == 'I') return dicom_vr::UI;
	if (c0 == 'U' && c1 == 'L') return dicom_vr::UL;
	if (c0 == 'U' && c1 == 'N') return dicom_vr::UN;
	if (c0 == 'U' && c1 == 'R') return dicom_vr::UR;
	if (c0 == 'U' && c1 == 'S') return dicom_vr::US;
	if (c0 == 'U' && c1 == 'T') return dicom_vr::UT;
	if (c0 == 'U' && c1 == 'V') return dicom_vr::UV;
	return;
};

// Returns true if the VR is text-based (string representation).
export fn vr_is_string(vr: dicom_vr) bool = {
	switch (vr) {
	case dicom_vr::AE, dicom_vr::AS, dicom_vr::CS, dicom_vr::DA,
	     dicom_vr::DS, dicom_vr::DT, dicom_vr::IS, dicom_vr::LO,
	     dicom_vr::LT, dicom_vr::PN, dicom_vr::SH, dicom_vr::ST,
	     dicom_vr::TM, dicom_vr::UC, dicom_vr::UI, dicom_vr::UR,
	     dicom_vr::UT =>
		return true;
	case =>
		return false;
	};
};

// Returns true if the VR is a sequence of nested datasets.
export fn vr_is_sequence(vr: dicom_vr) bool = {
	return vr == dicom_vr::SQ;
};

// Returns true if the VR represents binary data elements.
export fn vr_is_binary(vr: dicom_vr) bool = {
	switch (vr) {
	case dicom_vr::AT, dicom_vr::FL, dicom_vr::FD, dicom_vr::OB,
	     dicom_vr::OD, dicom_vr::OF, dicom_vr::OL, dicom_vr::OW,
	     dicom_vr::SL, dicom_vr::SS, dicom_vr::SV, dicom_vr::UL,
	     dicom_vr::UN, dicom_vr::US, dicom_vr::UV =>
		return true;
	case =>
		return false;
	};
};

// Returns the required padding byte for odd-length elements per DICOM PS3.5.
// UI elements must be padded with a trailing null (0x00).
// Text VRs are padded with ASCII Space (0x20).
// Binary VRs are padded with null (0x00).
export fn vr_pad_byte(vr: dicom_vr) u8 = {
	if (vr == dicom_vr::UI) {
		return 0x00u8;
	};
	if (vr_is_string(vr)) {
		return ' ';
	};
	return 0x00u8;
};

// Returns true if this VR uses a 32-bit length field in Explicit VR encoding.
// Per DICOM PS3.5 Section 7.1.2:
// OB, OD, OF, OL, OW, SQ, UC, UN, UR, and UT use 2 bytes of reserved 0x0000
// followed by a 4-byte (32-bit) unsigned length field.
// All other VRs use a direct 2-byte (16-bit) unsigned length field.
export fn vr_has_explicit_32bit_length(vr: dicom_vr) bool = {
	switch (vr) {
	case dicom_vr::OB, dicom_vr::OD, dicom_vr::OF, dicom_vr::OL,
	     dicom_vr::OW, dicom_vr::SQ, dicom_vr::UC, dicom_vr::UN,
	     dicom_vr::UR, dicom_vr::UT =>
		return true;
	case =>
		return false;
	};
};

// Validates whether a UID string adheres to DICOM rules:
// - Maximum 64 characters
// - Characters only in '0'-'9' and '.'
// - No leading zero in components unless component is solely "0"
export fn vr_validate_ui(bytes: []u8) bool = {
	if (len(bytes) == 0z || len(bytes) > 64z) {
		return false;
	};
	let at_component_start = true;
	for (let i = 0z; i < len(bytes); i += 1z) {
		const c = bytes[i];
		if (c == '.') {
			if (at_component_start) {
				return false; // Empty component like ".." or leading "."
			};
			at_component_start = true;
		} else if (c >= '0' && c <= '9') {
			if (at_component_start && c == '0' && i + 1z < len(bytes) && bytes[i + 1z] != '.') {
				return false; // Leading zero in multi-digit number
			};
			at_component_start = false;
		} else if (c == 0x00u8 && i == len(bytes) - 1z) {
			// Permissible trailing null padding byte
			break;
		} else {
			return false; // Invalid character
		};
	};
	return !at_component_start;
};

// Validates whether a date string conforms to DA format (YYYYMMDD).
export fn vr_validate_da(bytes: []u8) bool = {
	if (len(bytes) < 8z) {
		return false;
	};
	for (let i = 0z; i < 8z; i += 1z) {
		if (bytes[i] < '0' || bytes[i] > '9') {
			return false;
		};
	};
	return true;
};
