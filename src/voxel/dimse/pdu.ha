// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dimse::pdu
// DICOM Upper Layer Protocol for TCP/IP Protocol Data Unit (PDU) structures,
// serialization, and deserialization per NEMA PS3.8 Section 9.

use voxel::types;

export type pdu_type = enum u8 {
	A_ASSOCIATE_RQ = 0x01,
	A_ASSOCIATE_AC = 0x02,
	A_ASSOCIATE_RJ = 0x03,
	P_DATA_TF      = 0x04,
	A_RELEASE_RQ   = 0x05,
	A_RELEASE_RP   = 0x06,
	A_ABORT        = 0x07,
};

export type item_type = enum u8 {
	APPLICATION_CONTEXT          = 0x10,
	PRESENTATION_CONTEXT_RQ      = 0x20,
	PRESENTATION_CONTEXT_AC      = 0x21,
	ABSTRACT_SYNTAX              = 0x30,
	TRANSFER_SYNTAX              = 0x40,
	USER_INFORMATION             = 0x50,
	MAXIMUM_LENGTH               = 0x51,
	IMPLEMENTATION_CLASS_UID     = 0x52,
	ASYNCHRONOUS_OPERATIONS      = 0x53,
	SCP_SCU_ROLE_SELECTION       = 0x54,
	IMPLEMENTATION_VERSION_NAME  = 0x55,
};

export type presentation_context_rq = struct {
	id: u8,
	abstract_syntax: []u8,
	transfer_syntaxes: [][]u8,
};

export type presentation_context_ac = struct {
	id: u8,
	result: u8, // 0 = Acceptance, 1 = User Rejection, 2 = No Reason, 3 = Abstract Syntax Not Supported, 4 = Transfer Syntaxes Not Supported
	transfer_syntax: []u8,
};

export type associate_rq = struct {
	protocol_version: u16,
	called_ae: [16]u8,
	calling_ae: [16]u8,
	application_context: []u8,
	max_pdu_length: u32,
	implementation_class_uid: []u8,
	implementation_version_name: []u8,
};

// Writes a 6-byte PDU header (Type, Reserved 0x00, 32-bit Big-Endian Length).
export fn pdu_write_header(
	buf: *types::byte_buffer,
	ptype: pdu_type,
	length: u32,
) (void | types::error_code) = {
	types::buffer_write_u8(buf, ptype: u8)?;
	types::buffer_write_u8(buf, 0x00u8)?;
	types::buffer_write_u32(buf, length, types::endianness::BIG)?;
};

// Reads a 6-byte PDU header from the stream.
export fn pdu_read_header(
	buf: *types::byte_buffer,
) ((pdu_type, u32) | types::error_code) = {
	const raw_type = types::buffer_read_u8(buf)?;
	const _res = types::buffer_read_u8(buf)?;
	const length = types::buffer_read_u32(buf, types::endianness::BIG)?;

	const ptype = match (raw_type) {
	case 0x01u8 => yield pdu_type::A_ASSOCIATE_RQ;
	case 0x02u8 => yield pdu_type::A_ASSOCIATE_AC;
	case 0x03u8 => yield pdu_type::A_ASSOCIATE_RJ;
	case 0x04u8 => yield pdu_type::P_DATA_TF;
	case 0x05u8 => yield pdu_type::A_RELEASE_RQ;
	case 0x06u8 => yield pdu_type::A_RELEASE_RP;
	case 0x07u8 => yield pdu_type::A_ABORT;
	case => return types::error_code::PARSE_ERROR;
	};

	return (ptype, length);
};

// Encodes an A-RELEASE-RQ PDU into the buffer (fixed 10 bytes).
export fn encode_release_rq(out: []u8) (size | types::error_code) = {
	let buf = types::buffer_init(out);
	pdu_write_header(&buf, pdu_type::A_RELEASE_RQ, 4u32)?;
	types::buffer_write_u32(&buf, 0u32, types::endianness::BIG)?;
	return buf.cursor;
};

// Encodes an A-RELEASE-RP PDU into the buffer (fixed 10 bytes).
export fn encode_release_rp(out: []u8) (size | types::error_code) = {
	let buf = types::buffer_init(out);
	pdu_write_header(&buf, pdu_type::A_RELEASE_RP, 4u32)?;
	types::buffer_write_u32(&buf, 0u32, types::endianness::BIG)?;
	return buf.cursor;
};

// Encodes an A-ABORT PDU into the buffer (fixed 10 bytes: Source, Reason).
export fn encode_abort(out: []u8, source: u8, reason: u8) (size | types::error_code) = {
	let buf = types::buffer_init(out);
	pdu_write_header(&buf, pdu_type::A_ABORT, 4u32)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u8(&buf, source)?;
	types::buffer_write_u8(&buf, reason)?;
	return buf.cursor;
};

// Encodes an A-ASSOCIATE-RJ PDU into the buffer (fixed 10 bytes).
export fn encode_associate_rj(
	out: []u8,
	result: u8, // 1 = rejected-permanent, 2 = rejected-transient
	source: u8, // 1 = DICOM UL service-user, 2 = DICOM UL service-provider (ACSE related), 3 = presentation related
	reason: u8,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);
	pdu_write_header(&buf, pdu_type::A_ASSOCIATE_RJ, 4u32)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u8(&buf, result)?;
	types::buffer_write_u8(&buf, source)?;
	types::buffer_write_u8(&buf, reason)?;
	return buf.cursor;
};
