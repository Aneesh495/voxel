// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::json
// Implementation of the DICOM JSON Model per NEMA PS3.18 Section F.1.

use voxel::types;
use voxel::dicom::dataset;
use voxel::dicom::element;
use voxel::dicom::tag;
use voxel::dicom::vr;

// Formats a 16-bit integer as a 4-character uppercase hexadecimal string.
fn hex16(val: u16, out: []u8) void = {
	const hex_digits = "0123456789ABCDEF";
	out[0] = hex_digits[(val >> 12u16) & 0x0Fu16]: u8;
	out[1] = hex_digits[(val >> 8u16) & 0x0Fu16]: u8;
	out[2] = hex_digits[(val >> 4u16) & 0x0Fu16]: u8;
	out[3] = hex_digits[val & 0x0Fu16]: u8;
};

// Formats a DICOM tag as an 8-character uppercase hex string (e.g. "00100020").
export fn tag_to_hex8(t: tag::dicom_tag, out: []u8) void = {
	hex16(t.group, out[..4]);
	hex16(t.element, out[4..8]);
};

// Serializes a dataset into the standard DICOM JSON model format per PS3.18.
export fn dataset_to_json(
	ds: *dataset::dataset,
	out_buf: []u8,
) (size | types::error_code) = {
	let buf = types::buffer_init(out_buf);

	// Start root JSON object
	types::buffer_write_u8(&buf, '{')?;

	let first = true;
	for (let i = 0z; i < dataset::dataset_count(ds); i += 1z) {
		match (dataset::dataset_get_at(ds, i)) {
		case let elem: element::data_element =>
			// Do not serialize pixel data payload directly into JSON
			if (elem.tag.group == 0x7FE0u16) {
				continue;
			};

			if (!first) {
				types::buffer_write_u8(&buf, ',')?;
			};
			first = false;

			// Write "GGGGEEEE": {
			types::buffer_write_u8(&buf, '"')?;
			let hex_tag: [8]u8 = [0...];
			tag_to_hex8(elem.tag, hex_tag[..]);
			types::buffer_write_bytes(&buf, hex_tag[..])?;
			types::buffer_write_bytes(&buf, [':', '{', '"', 'v', 'r', '"', ':', '"'])?;

			const vr_s = vr::vr_to_str(elem.vr);
			types::buffer_write_u8(&buf, vr_s[0]: u8)?;
			types::buffer_write_u8(&buf, vr_s[1]: u8)?;
			types::buffer_write_u8(&buf, '"')?;

			// If element has value data, write "Value": [...]
			const str_val = element::element_get_string_bytes(&elem);
			if (len(str_val) > 0z) {
				types::buffer_write_bytes(&buf, [',', '"', 'V', 'a', 'l', 'u', 'e', '"', ':', '['])?;

				if (elem.vr == vr::dicom_vr::PN) {
					// Person Name object: {"Alphabetic": "..."}
					types::buffer_write_bytes(&buf, ['{', '"', 'A', 'l', 'p', 'h', 'a', 'b', 'e', 't', 'i', 'c', '"', ':', '"'])?;
					types::buffer_write_bytes(&buf, str_val)?;
					types::buffer_write_bytes(&buf, ['"', '}'])?;
				} else if (vr::vr_is_string(elem.vr)) {
					// String value
					types::buffer_write_u8(&buf, '"')?;
					// Escape characters if necessary
					for (let b = 0z; b < len(str_val); b += 1z) {
						const c = str_val[b];
						if (c == '"' || c == '\\') {
							types::buffer_write_u8(&buf, '\\')?;
						};
						types::buffer_write_u8(&buf, c)?;
					};
					types::buffer_write_u8(&buf, '"')?;
				} else if (elem.vr == vr::dicom_vr::US || elem.vr == vr::dicom_vr::UL) {
					// Numerical unsigned integer
					match (element::element_get_u32(&elem, types::endianness::LITTLE)) {
					case let num: u32 =>
						// Convert integer to string
						let temp: [12]u8 = [0...];
						let t_idx = 0z;
						let n = num;
						if (n == 0u32) {
							types::buffer_write_u8(&buf, '0')?;
						} else {
							while (n > 0u32) {
								temp[t_idx] = ('0': u8) + ((n % 10u32): u8);
								t_idx += 1z;
								n /= 10u32;
							};
							for (let r = 0z; r < t_idx; r += 1z) {
								types::buffer_write_u8(&buf, temp[t_idx - 1z - r])?;
							};
						};
					case =>
						types::buffer_write_u8(&buf, '0')?;
					};
				} else {
					// Fallback string representation
					types::buffer_write_u8(&buf, '"')?;
					types::buffer_write_bytes(&buf, str_val)?;
					types::buffer_write_u8(&buf, '"')?;
				};

				types::buffer_write_u8(&buf, ']')?;
			};

			types::buffer_write_u8(&buf, '}')?;
		case void => break;
		};
	};

	types::buffer_write_u8(&buf, '}')?;
	return buf.cursor;
};
