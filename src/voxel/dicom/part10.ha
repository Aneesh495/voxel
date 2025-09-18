// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::part10
// High-throughput DICOM Part 10 binary stream deserializer and serializer per PS3.10.

use voxel::types;
use voxel::dicom::dataset;
use voxel::dicom::dict;
use voxel::dicom::element;
use voxel::dicom::tag;
use voxel::dicom::uid;
use voxel::dicom::vr;

export type part10_header = struct {
	has_preamble: bool,
	transfer_syntax_uid: []u8,
	sop_class_uid: []u8,
	sop_instance_uid: []u8,
};

// Validates whether the stream begins with a standard 128-byte preamble and DICM prefix.
export fn has_dicm_prefix(stream: []u8) bool = {
	if (len(stream) < 132z) {
		return false;
	};
	return stream[128] == 'D' &&
		stream[129] == 'I' &&
		stream[130] == 'C' &&
		stream[131] == 'M';
};

// Parses a DICOM Part 10 binary stream into File Meta Information and main dataset.
export fn parse_part10(
	raw_stream: []u8,
	meta_dataset: *dataset::dataset,
	main_dataset: *dataset::dataset,
) (part10_header | types::error_code) = {
	let buf = types::buffer_init(raw_stream);
	let header = part10_header {
		has_preamble = false,
		transfer_syntax_uid = [],
		sop_class_uid = [],
		sop_instance_uid = [],
	};

	// Check for standard 128-byte preamble + "DICM" identifier
	if (has_dicm_prefix(raw_stream)) {
		buf.cursor = 132z;
		header.has_preamble = true;
	} else {
		// Non-preamble direct stream (e.g. raw network dataset)
		buf.cursor = 0z;
	};

	// File Meta Information (Group 0002) is strictly Explicit VR Little Endian
	let is_explicit = true;
	let endian = types::endianness::LITTLE;

	while (types::buffer_remaining(&buf) >= 6z) {
		const saved_cursor = buf.cursor;
		const group = types::buffer_read_u16(&buf, endian)?;
		const elem_num = types::buffer_read_u16(&buf, endian)?;
		const elem_tag = tag::tag_new(group, elem_num);

		// If we transition past Group 0002, switch to the negotiated transfer syntax
		if (group != 0x0002u16) {
			buf.cursor = saved_cursor; // Rewind back to element header
			break;
		};

		// Group 0002 is always Explicit VR Little Endian
		const vr_c0 = types::buffer_read_u8(&buf)?;
		const vr_c1 = types::buffer_read_u8(&buf)?;
		let elem_vr = vr::dicom_vr::UN;
		match (vr::vr_from_chars(vr_c0, vr_c1)) {
		case let v: vr::dicom_vr =>
			elem_vr = v;
		case void =>
			return types::error_code::PARSE_ERROR;
		};

		let elem_len: u32 = 0u32;
		if (vr::vr_has_explicit_32bit_length(elem_vr)) {
			// 2 reserved bytes (0x0000) followed by 4-byte length
			const _reserved = types::buffer_read_u16(&buf, endian)?;
			elem_len = types::buffer_read_u32(&buf, endian)?;
		} else {
			// Direct 2-byte length
			elem_len = (types::buffer_read_u16(&buf, endian)?): u32;
		};

		if (types::buffer_remaining(&buf) < (elem_len: size)) {
			return types::error_code::CORRUPT_DATA;
		};

		const val_start = buf.cursor;
		buf.cursor += (elem_len: size);
		const val_slice = raw_stream[val_start..val_start + (elem_len: size)];

		const data_elem = element::element_new(elem_tag, elem_vr, val_slice);
		dataset::dataset_insert(meta_dataset, data_elem)?;

		// Cache critical meta header fields
		if (tag::tag_equal(elem_tag, tag::TAG_TRANSFER_SYNTAX_UID)) {
			header.transfer_syntax_uid = element::element_get_string_bytes(&data_elem);
		} else if (tag::tag_equal(elem_tag, tag::TAG_MEDIA_STORAGE_SOP_CLASS_UID)) {
			header.sop_class_uid = element::element_get_string_bytes(&data_elem);
		} else if (tag::tag_equal(elem_tag, tag::TAG_MEDIA_STORAGE_SOP_INSTANCE_UID)) {
			header.sop_instance_uid = element::element_get_string_bytes(&data_elem);
		};
	};

	// Determine syntax parameters for main dataset
	if (len(header.transfer_syntax_uid) > 0z) {
		is_explicit = uid::syntax_is_explicit(header.transfer_syntax_uid);
		if (uid::syntax_is_big_endian(header.transfer_syntax_uid)) {
			endian = types::endianness::BIG;
		} else {
			endian = types::endianness::LITTLE;
		};
	};

	// Parse main dataset body
	while (types::buffer_remaining(&buf) >= 4z) {
		const group = types::buffer_read_u16(&buf, endian)?;
		const elem_num = types::buffer_read_u16(&buf, endian)?;
		const elem_tag = tag::tag_new(group, elem_num);

		let elem_vr = vr::dicom_vr::UN;
		let elem_len: u32 = 0u32;
		let is_undefined = false;

		if (is_explicit) {
			if (types::buffer_remaining(&buf) < 2z) break;
			const vr_c0 = types::buffer_read_u8(&buf)?;
			const vr_c1 = types::buffer_read_u8(&buf)?;
			match (vr::vr_from_chars(vr_c0, vr_c1)) {
			case let v: vr::dicom_vr =>
				elem_vr = v;
			case void =>
				// Fallback to dictionary
				elem_vr = dict::dict_get_vr(elem_tag);
			};

			if (vr::vr_has_explicit_32bit_length(elem_vr)) {
				if (types::buffer_remaining(&buf) < 6z) break;
				const _res = types::buffer_read_u16(&buf, endian)?;
				elem_len = types::buffer_read_u32(&buf, endian)?;
			} else {
				if (types::buffer_remaining(&buf) < 2z) break;
				elem_len = (types::buffer_read_u16(&buf, endian)?): u32;
			};
		} else {
			// Implicit VR Little Endian: VR is omitted, 4-byte length follows tag
			if (types::buffer_remaining(&buf) < 4z) break;
			elem_len = types::buffer_read_u32(&buf, endian)?;
			elem_vr = dict::dict_get_vr(elem_tag);
		};

		if (elem_len == 0xFFFFFFFFu32) {
			// Undefined length element (e.g. Sequence or Encapsulated Pixel Data)
			is_undefined = true;
			// Scan forward to delimiter tag (FFFE, E0DD)
			const content_start = buf.cursor;
			let found_delimiter = false;
			while (types::buffer_remaining(&buf) >= 8z) {
				const check_grp = types::buffer_read_u16(&buf, endian)?;
				const check_elm = types::buffer_read_u16(&buf, endian)?;
				if (check_grp == 0xFFFEu16 && (check_elm == 0xE0DDu16 || check_elm == 0xE00Du16)) {
					const _delim_len = types::buffer_read_u32(&buf, endian)?;
					found_delimiter = true;
					break;
				};
			};
			const val_slice = if (found_delimiter) {
				yield raw_stream[content_start..buf.cursor - 8z];
			} else {
				yield raw_stream[content_start..];
			};
			let d_elem = element::element_new(elem_tag, elem_vr, val_slice);
			d_elem.is_undefined_length = true;
			dataset::dataset_insert(main_dataset, d_elem)?;
		} else {
			if (types::buffer_remaining(&buf) < (elem_len: size)) {
				// Truncated trailing element
				const rem = types::buffer_remaining(&buf);
				const val_start = buf.cursor;
				buf.cursor += rem;
				const val_slice = raw_stream[val_start..val_start + rem];
				const d_elem = element::element_new(elem_tag, elem_vr, val_slice);
				dataset::dataset_insert(main_dataset, d_elem)?;
				break;
			};

			const val_start = buf.cursor;
			buf.cursor += (elem_len: size);
			const val_slice = raw_stream[val_start..val_start + (elem_len: size)];
			const d_elem = element::element_new(elem_tag, elem_vr, val_slice);
			dataset::dataset_insert(main_dataset, d_elem)?;
		};
	};

	return header;
};

// Serializes a dataset into Part 10 binary stream format with 128-byte preamble and DICM header.
export fn serialize_part10(
	out_stream: []u8,
	meta_dataset: *dataset::dataset,
	main_dataset: *dataset::dataset,
) (size | types::error_code) = {
	let buf = types::buffer_init(out_stream);

	// 1. Write 128 zero bytes preamble
	for (let i = 0z; i < 128z; i += 1z) {
		types::buffer_write_u8(&buf, 0x00u8)?;
	};

	// 2. Write "DICM" magic identifier
	types::buffer_write_u8(&buf, 'D')?;
	types::buffer_write_u8(&buf, 'I')?;
	types::buffer_write_u8(&buf, 'C')?;
	types::buffer_write_u8(&buf, 'M')?;

	// 3. Write File Meta Information (Explicit VR Little Endian)
	for (let i = 0z; i < dataset::dataset_count(meta_dataset); i += 1z) {
		match (dataset::dataset_get_at(meta_dataset, i)) {
		case let elem: element::data_element =>
			types::buffer_write_u16(&buf, elem.tag.group, types::endianness::LITTLE)?;
			types::buffer_write_u16(&buf, elem.tag.element, types::endianness::LITTLE)?;

			const vr_str = vr::vr_to_str(elem.vr);
			types::buffer_write_u8(&buf, vr_str[0]: u8)?;
			types::buffer_write_u8(&buf, vr_str[1]: u8)?;

			if (vr::vr_has_explicit_32bit_length(elem.vr)) {
				types::buffer_write_u16(&buf, 0x0000u16, types::endianness::LITTLE)?;
				types::buffer_write_u32(&buf, elem.length, types::endianness::LITTLE)?;
			} else {
				types::buffer_write_u16(&buf, elem.length: u16, types::endianness::LITTLE)?;
			};

			types::buffer_write_bytes(&buf, elem.data)?;
		case void => break;
		};
	};

	// 4. Write Main Dataset (Explicit VR Little Endian)
	for (let i = 0z; i < dataset::dataset_count(main_dataset); i += 1z) {
		match (dataset::dataset_get_at(main_dataset, i)) {
		case let elem: element::data_element =>
			types::buffer_write_u16(&buf, elem.tag.group, types::endianness::LITTLE)?;
			types::buffer_write_u16(&buf, elem.tag.element, types::endianness::LITTLE)?;

			const vr_str = vr::vr_to_str(elem.vr);
			types::buffer_write_u8(&buf, vr_str[0]: u8)?;
			types::buffer_write_u8(&buf, vr_str[1]: u8)?;

			if (vr::vr_has_explicit_32bit_length(elem.vr)) {
				types::buffer_write_u16(&buf, 0x0000u16, types::endianness::LITTLE)?;
				types::buffer_write_u32(&buf, elem.length, types::endianness::LITTLE)?;
			} else {
				types::buffer_write_u16(&buf, elem.length: u16, types::endianness::LITTLE)?;
			};

			types::buffer_write_bytes(&buf, elem.data)?;
		case void => break;
		};
	};

	return buf.cursor;
};
