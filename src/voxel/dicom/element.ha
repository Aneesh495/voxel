// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::element
// In-memory representation of a DICOM Data Element, encapsulation of raw values,
// typed extractors, and sequence hierarchy support per DICOM PS3.5.

use voxel::types;
use voxel::dicom::tag;
use voxel::dicom::vr;

export type data_element = struct {
	tag: tag::dicom_tag,
	vr: vr::dicom_vr,
	length: u32,
	data: []u8,
	is_undefined_length: bool,
};

// Constructs a data element from raw byte slice with explicit length.
export fn element_new(t: tag::dicom_tag, v: vr::dicom_vr, raw: []u8) data_element = {
	return data_element {
		tag = t,
		vr = v,
		length = len(raw): u32,
		data = raw,
		is_undefined_length = false,
	};
};

// Constructs an empty data element with undefined length (0xFFFFFFFF).
export fn element_new_undefined(t: tag::dicom_tag, v: vr::dicom_vr) data_element = {
	return data_element {
		tag = t,
		vr = v,
		length = 0xFFFFFFFFu32,
		data = [],
		is_undefined_length = true,
	};
};

// Reads a 16-bit unsigned integer from the element payload.
export fn element_get_u16(elem: *data_element, endian: types::endianness) (u16 | types::error_code) = {
	if (len(elem.data) < 2z) {
		return types::error_code::CORRUPT_DATA;
	};
	let buf = types::buffer_init(elem.data);
	return types::buffer_read_u16(&buf, endian);
};

// Reads a 32-bit unsigned integer from the element payload.
export fn element_get_u32(elem: *data_element, endian: types::endianness) (u32 | types::error_code) = {
	if (len(elem.data) < 4z) {
		return types::error_code::CORRUPT_DATA;
	};
	let buf = types::buffer_init(elem.data);
	return types::buffer_read_u32(&buf, endian);
};

// Reads a 16-bit signed integer from the element payload.
export fn element_get_i16(elem: *data_element, endian: types::endianness) (i16 | types::error_code) = {
	const unsigned_val = element_get_u16(elem, endian)?;
	const ptr = &unsigned_val: *i16;
	return *ptr;
};

// Reads a 32-bit signed integer from the element payload.
export fn element_get_i32(elem: *data_element, endian: types::endianness) (i32 | types::error_code) = {
	const unsigned_val = element_get_u32(elem, endian)?;
	const ptr = &unsigned_val: *i32;
	return *ptr;
};

// Reads a 32-bit floating point number from the element payload.
export fn element_get_f32(elem: *data_element, endian: types::endianness) (f32 | types::error_code) = {
	if (len(elem.data) < 4z) {
		return types::error_code::CORRUPT_DATA;
	};
	let buf = types::buffer_init(elem.data);
	return types::buffer_read_f32(&buf, endian);
};

// Reads a 64-bit floating point number from the element payload.
export fn element_get_f64(elem: *data_element, endian: types::endianness) (f64 | types::error_code) = {
	if (len(elem.data) < 8z) {
		return types::error_code::CORRUPT_DATA;
	};
	let buf = types::buffer_init(elem.data);
	return types::buffer_read_f64(&buf, endian);
};

// Returns a trimmed string view of the element payload, removing trailing padding spaces and null bytes.
export fn element_get_string_bytes(elem: *data_element) []u8 = {
	let bytes = elem.data;
	let end = len(bytes);
	while (end > 0z && (bytes[end - 1z] == ' ' || bytes[end - 1z] == 0x00u8)) {
		end -= 1z;
	};
	return bytes[..end];
};

// Parses a Decimal String (DS) element into an f64 value.
// Handles scientific notation, leading/trailing whitespace, and multiple values separated by backslash.
export fn element_parse_ds_first(elem: *data_element) (f64 | types::error_code) = {
	const raw = element_get_string_bytes(elem);
	if (len(raw) == 0z) {
		return types::error_code::CORRUPT_DATA;
	};

	let start = 0z;
	while (start < len(raw) && raw[start] == ' ') {
		start += 1z;
	};

	let end = start;
	while (end < len(raw) && raw[end] != '\\' && raw[end] != ' ') {
		end += 1z;
	};

	if (start >= end) {
		return types::error_code::CORRUPT_DATA;
	};

	// Parse sign
	let is_negative = false;
	let curr = start;
	if (raw[curr] == '-') {
		is_negative = true;
		curr += 1z;
	} else if (raw[curr] == '+') {
		curr += 1z;
	};

	let int_part: f64 = 0.0;
	let frac_part: f64 = 0.0;
	let divisor: f64 = 1.0;
	let has_decimal = false;

	while (curr < end) {
		const c = raw[curr];
		if (c >= '0' && c <= '9') {
			const digit = (c - '0'): f64;
			if (!has_decimal) {
				int_part = int_part * 10.0 + digit;
			} else {
				divisor *= 10.0;
				frac_part += digit / divisor;
			};
		} else if (c == '.') {
			if (has_decimal) {
				return types::error_code::CORRUPT_DATA;
			};
			has_decimal = true;
		} else if (c == 'e' || c == 'E') {
			// Scientific notation exponent parsing
			curr += 1z;
			let exp_neg = false;
			if (curr < end && raw[curr] == '-') {
				exp_neg = true;
				curr += 1z;
			} else if (curr < end && raw[curr] == '+') {
				curr += 1z;
			};
			let exp_val = 0;
			while (curr < end && raw[curr] >= '0' && raw[curr] <= '9') {
				exp_val = exp_val * 10 + (raw[curr] - '0'): int;
				curr += 1z;
			};
			let total = int_part + frac_part;
			if (is_negative) total = -total;
			let scale: f64 = 1.0;
			for (let e = 0; e < exp_val; e += 1) {
				scale *= 10.0;
			};
			if (exp_neg) {
				return total / scale;
			} else {
				return total * scale;
			};
		} else {
			break;
		};
		curr += 1z;
	};

	let result = int_part + frac_part;
	if (is_negative) {
		result = -result;
	};
	return result;
};

// Parses an Integer String (IS) element into an i32 value.
export fn element_parse_is_first(elem: *data_element) (i32 | types::error_code) = {
	const raw = element_get_string_bytes(elem);
	if (len(raw) == 0z) {
		return types::error_code::CORRUPT_DATA;
	};

	let start = 0z;
	while (start < len(raw) && raw[start] == ' ') {
		start += 1z;
	};

	let is_neg = false;
	if (start < len(raw) && raw[start] == '-') {
		is_neg = true;
		start += 1z;
	} else if (start < len(raw) && raw[start] == '+') {
		start += 1z;
	};

	let acc: i32 = 0;
	let count = 0;
	for (let i = start; i < len(raw); i += 1z) {
		const c = raw[i];
		if (c >= '0' && c <= '9') {
			acc = acc * 10 + (c - '0'): i32;
			count += 1;
		} else {
			break;
		};
	};

	if (count == 0) {
		return types::error_code::CORRUPT_DATA;
	};

	if (is_neg) {
		acc = -acc;
	};
	return acc;
};
