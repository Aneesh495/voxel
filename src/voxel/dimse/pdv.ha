// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dimse::pdv
// Presentation Data Value (PDV) framing, fragmentation, and stream reassembly
// for P-DATA-TF PDUs per NEMA PS3.8 Section 9.3.5.1.

use voxel::types;
use voxel::dimse::pdu;

export type pdv_item = struct {
	context_id: u8,
	is_command: bool,
	is_last: bool,
	payload: []u8,
};

// Reads and unpacks all PDV fragments from a P-DATA-TF PDU payload.
export fn parse_p_data_tf(
	buf: *types::byte_buffer,
	pdu_length: u32,
	dest_pdvs: []pdv_item,
) (size | types::error_code) = {
	let bytes_read = 0u32;
	let pdv_count = 0z;

	while (bytes_read < pdu_length && pdv_count < len(dest_pdvs)) {
		if (types::buffer_remaining(buf) < 6z) {
			return types::error_code::CORRUPT_DATA;
		};

		// 4-byte PDV length (includes the 2-byte header: context_id + mch)
		const pdv_len = types::buffer_read_u32(buf, types::endianness::BIG)?;
		if (pdv_len < 2u32) {
			return types::error_code::CORRUPT_DATA;
		};

		const context_id = types::buffer_read_u8(buf)?;
		const mch = types::buffer_read_u8(buf)?;

		const is_command = (mch & 0x01u8) != 0u8;
		const is_last = (mch & 0x02u8) != 0u8;
		const payload_len = (pdv_len - 2u32): size;

		if (types::buffer_remaining(buf) < payload_len) {
			return types::error_code::CORRUPT_DATA;
		};

		const start_pos = buf.cursor;
		buf.cursor += payload_len;
		const payload_slice = buf.data[start_pos..start_pos + payload_len];

		dest_pdvs[pdv_count] = pdv_item {
			context_id = context_id,
			is_command = is_command,
			is_last = is_last,
			payload = payload_slice,
		};
		pdv_count += 1z;

		bytes_read += 4u32 + pdv_len;
	};

	return pdv_count;
};

// Encodes a single complete or fragmented message into a P-DATA-TF PDU.
export fn encode_p_data_tf_single(
	out: []u8,
	context_id: u8,
	is_command: bool,
	is_last: bool,
	payload: []u8,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	const pdv_len = (2z + len(payload)): u32;
	const total_pdu_len = 4u32 + pdv_len;

	// PDU Header
	pdu::pdu_write_header(&buf, pdu::pdu_type::P_DATA_TF, total_pdu_len)?;

	// PDV Length (4 bytes Big Endian)
	types::buffer_write_u32(&buf, pdv_len, types::endianness::BIG)?;

	// Presentation Context ID (1 byte)
	types::buffer_write_u8(&buf, context_id)?;

	// Message Control Header (1 byte)
	let mch: u8 = 0u8;
	if (is_command) {
		mch |= 0x01u8;
	};
	if (is_last) {
		mch |= 0x02u8;
	};
	types::buffer_write_u8(&buf, mch)?;

	// Payload data
	types::buffer_write_bytes(&buf, payload)?;

	return buf.cursor;
};
