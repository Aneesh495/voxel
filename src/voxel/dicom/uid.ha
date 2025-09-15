// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::uid
// Standard DICOM Unique Identifiers (UIDs) for Storage SOP Classes,
// Query/Retrieve Services, Transfer Syntaxes, and ISO/IEC 9834-8 UID generator.

export def UID_IMPLICIT_VR_LITTLE_ENDIAN: str = "1.2.840.10008.1.2";
export def UID_EXPLICIT_VR_LITTLE_ENDIAN: str = "1.2.840.10008.1.2.1";
export def UID_DEFLATED_EXPLICIT_VR_LITTLE_ENDIAN: str = "1.2.840.10008.1.2.1.99";
export def UID_EXPLICIT_VR_BIG_ENDIAN: str = "1.2.840.10008.1.2.2";
export def UID_RLE_LOSSLESS: str = "1.2.840.10008.1.2.5";
export def UID_JPEG_BASELINE_PROCESS_1: str = "1.2.840.10008.1.2.4.50";
export def UID_JPEG_LOSSLESS_HIERARCHICAL: str = "1.2.840.10008.1.2.4.70";
export def UID_JPEG_LS_LOSSLESS: str = "1.2.840.10008.1.2.4.80";
export def UID_JPEG2000_LOSSLESS: str = "1.2.840.10008.1.2.4.90";

export def UID_VERIFICATION_SOP_CLASS: str = "1.2.840.10008.1.1";
export def UID_CT_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.2";
export def UID_ENHANCED_CT_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.2.1";
export def UID_MR_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.4";
export def UID_ENHANCED_MR_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.4.1";
export def UID_SECONDARY_CAPTURE_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.7";
export def UID_CR_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.1";
export def UID_DIGITAL_XRAY_PRESENTATION: str = "1.2.840.10008.5.1.4.1.1.1.1";
export def UID_ULTRASOUND_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.6.1";
export def UID_PET_IMAGE_STORAGE: str = "1.2.840.10008.5.1.4.1.1.128";
export def UID_RT_STRUCTURE_SET_STORAGE: str = "1.2.840.10008.5.1.4.1.1.481.3";

export def UID_PATIENT_ROOT_QR_FIND: str = "1.2.840.10008.5.1.4.1.2.1.1";
export def UID_PATIENT_ROOT_QR_MOVE: str = "1.2.840.10008.5.1.4.1.2.1.2";
export def UID_STUDY_ROOT_QR_FIND: str = "1.2.840.10008.5.1.4.1.2.2.1";
export def UID_STUDY_ROOT_QR_MOVE: str = "1.2.840.10008.5.1.4.1.2.2.2";

export def VOXEL_IMPLEMENTATION_CLASS_UID: str = "1.2.826.0.1.3680043.10.1142.1";
export def VOXEL_IMPLEMENTATION_VERSION_NAME: str = "VOXEL_PACS_1_0";

// Checks whether a transfer syntax uses Explicit Value Representations.
export fn syntax_is_explicit(uid: []u8) bool = {
	const implicit_bytes = [
		'1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '2'
	];
	if (len(uid) == len(implicit_bytes)) {
		let match_found = true;
		for (let i = 0z; i < len(uid); i += 1z) {
			if (uid[i] != implicit_bytes[i]) {
				match_found = false;
				break;
			};
		};
		if (match_found) return false;
	};
	return true;
};

// Checks whether a transfer syntax uses Big Endian byte ordering.
export fn syntax_is_big_endian(uid: []u8) bool = {
	const big_bytes = [
		'1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '2', '.', '2'
	];
	if (len(uid) == len(big_bytes)) {
		for (let i = 0z; i < len(uid); i += 1z) {
			if (uid[i] != big_bytes[i]) {
				return false;
			};
		};
		return true;
	};
	return false;
};

// Checks whether a transfer syntax encapsulates compressed pixel fragments.
export fn syntax_is_encapsulated(uid: []u8) bool = {
	// Encapsulated syntaxes include JPEG, JPEG-LS, J2K, RLE (prefix 1.2.840.10008.1.2.4 or .5)
	const prefix = [
		'1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '2', '.'
	];
	if (len(uid) < len(prefix) + 2z) {
		return false;
	};
	for (let i = 0z; i < len(prefix); i += 1z) {
		if (uid[i] != prefix[i]) {
			return false;
		};
	};
	const sub = uid[len(prefix)];
	return sub == '4' || sub == '5';
};

// Formats a 128-bit integer into a standard decimal OID string under prefix 2.25.x
// per ITU-T X.667 / ISO/IEC 9834-8 (UUID as decimal).
export fn format_uuid_oid(out_buf: []u8, uuid_high: u64, uuid_low: u64) size = {
	const prefix = "2.25.";
	const p_len = len(prefix);
	for (let i = 0z; i < p_len; i += 1z) {
		out_buf[i] = prefix[i]: u8;
	};

	// Convert 128-bit value (uuid_high, uuid_low) to decimal string digits
	// Simple multi-precision division by 10
	let temp: [40]u8 = [0...];
	let t_len = 0z;
	let h = uuid_high;
	let l = uuid_low;

	if (h == 0u64 && l == 0u64) {
		out_buf[p_len] = '0';
		return p_len + 1z;
	};

	while (h > 0u64 || l > 0u64) {
		// Division of (h:l) by 10:
		const rem_h = h % 10u64;
		h = h / 10u64;

		// Carry rem_h * 2^64 into low 64 bits:
		// 2^64 mod 10 = 6, so rem = (rem_h * 6 + (l mod 10)) mod 10
		// Precise 128-bit quotient:
		// l_full = rem_h * 2^64 + l
		const rem_h_times_2_64 = rem_h * 1844674407370955161u64 + (rem_h * 6u64) / 10u64;
		const l_div = l / 10u64;
		const l_rem = l % 10u64;
		const total_rem = (rem_h * 6u64 + l_rem) % 10u64;
		l = rem_h_times_2_64 + l_div + (rem_h * 6u64 + l_rem) / 10u64;

		temp[t_len] = ('0': u8) + (total_rem: u8);
		t_len += 1z;
	};

	for (let i = 0z; i < t_len; i += 1z) {
		out_buf[p_len + i] = temp[t_len - 1z - i];
	};

	return p_len + t_len;
};
