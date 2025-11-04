// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::storage::wal
// Append-only Write-Ahead Log (WAL) for atomic metadata persistence,
// CRC32 frame checksumming, and crash recovery replay.

use voxel::types;

export def WAL_MAGIC: u32 = 0x56585741u32; // 'VXWA'

export type wal_tx_type = enum u16 {
	INSERT_INSTANCE = 0x0001,
	DELETE_INSTANCE = 0x0002,
	UPDATE_STUDY    = 0x0003,
};

export type wal_frame_header = struct {
	magic: u32,
	seq_num: u64,
	tx_type: wal_tx_type,
	payload_length: u32,
	crc32: u32,
};

// Encodes a WAL frame with CRC32 integrity checksum into buffer.
export fn encode_wal_frame(
	out: []u8,
	seq_num: u64,
	tx_type: wal_tx_type,
	payload: []u8,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	const payload_len = len(payload): u32;

	// Frame header: 22 bytes before CRC
	types::buffer_write_u32(&buf, WAL_MAGIC, types::endianness::LITTLE)?;
	types::buffer_write_u64(&buf, seq_num, types::endianness::LITTLE)?;
	types::buffer_write_u16(&buf, tx_type: u16, types::endianness::LITTLE)?;
	types::buffer_write_u32(&buf, payload_len, types::endianness::LITTLE)?;

	// Write payload
	types::buffer_write_bytes(&buf, payload)?;

	// Compute CRC32 over magic, seq_num, tx_type, payload_length, and payload bytes
	const crc = types::crc32_calculate(out[..buf.cursor]);

	// Append 4-byte CRC32
	types::buffer_write_u32(&buf, crc, types::endianness::LITTLE)?;

	return buf.cursor;
};

// Validates and decodes a WAL frame from a raw log slice.
export fn validate_wal_frame(
	frame_bytes: []u8,
) (wal_frame_header | types::error_code) = {
	if (len(frame_bytes) < 22z) {
		return types::error_code::CORRUPT_DATA;
	};

	let buf = types::buffer_init(frame_bytes);
	const magic = types::buffer_read_u32(&buf, types::endianness::LITTLE)?;
	if (magic != WAL_MAGIC) {
		return types::error_code::CORRUPT_DATA;
	};

	const seq_num = types::buffer_read_u64(&buf, types::endianness::LITTLE)?;
	const raw_tx = types::buffer_read_u16(&buf, types::endianness::LITTLE)?;
	const payload_len = types::buffer_read_u32(&buf, types::endianness::LITTLE)?;

	const total_expected_len = 18z + (payload_len: size) + 4z;
	if (len(frame_bytes) < total_expected_len) {
		return types::error_code::BUFFER_OVERFLOW;
	};

	// Check CRC32
	const recorded_crc_pos = 18z + (payload_len: size);
	let crc_buf = types::buffer_init(frame_bytes[recorded_crc_pos..]);
	const recorded_crc = types::buffer_read_u32(&crc_buf, types::endianness::LITTLE)?;

	const computed_crc = types::crc32_calculate(frame_bytes[..recorded_crc_pos]);
	if (recorded_crc != computed_crc) {
		return types::error_code::CORRUPT_DATA;
	};

	const tx_type = match (raw_tx) {
	case 0x0001u16 => yield wal_tx_type::INSERT_INSTANCE;
	case 0x0002u16 => yield wal_tx_type::DELETE_INSTANCE;
	case 0x0003u16 => yield wal_tx_type::UPDATE_STUDY;
	case => return types::error_code::CORRUPT_DATA;
	};

	return wal_frame_header {
		magic = magic,
		seq_num = seq_num,
		tx_type = tx_type,
		payload_length = payload_len,
		crc32 = recorded_crc,
	};
};
