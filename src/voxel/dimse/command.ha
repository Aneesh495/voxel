// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dimse::command
// DIMSE-C command set structures, message fields, and status codes per NEMA PS3.7.

use voxel::types;
use voxel::dicom::dataset;
use voxel::dicom::element;
use voxel::dicom::tag;
use voxel::dicom::vr;

export type command_type = enum u16 {
	C_STORE_RQ = 0x0001,
	C_STORE_RSP = 0x8001,
	C_GET_RQ = 0x0010,
	C_GET_RSP = 0x8010,
	C_FIND_RQ = 0x0020,
	C_FIND_RSP = 0x8020,
	C_MOVE_RQ = 0x0021,
	C_MOVE_RSP = 0x8021,
	C_ECHO_RQ = 0x0030,
	C_ECHO_RSP = 0x8030,
	C_CANCEL_RQ = 0x0FFF,
};

export type dimse_status = enum u16 {
	SUCCESS = 0x0000,
	PENDING = 0xFF00,
	PENDING_OPTIONAL_WARNING = 0xFF01,
	CANCEL = 0xFE00,
	REFUSED_OUT_OF_RESOURCES = 0xA700,
	REFUSED_UNABLE_TO_CALCULATE = 0xA701,
	REFUSED_UNABLE_TO_PERFORM = 0xA702,
	ERROR_DATASET_DOES_NOT_MATCH = 0xA900,
	ERROR_CANNOT_UNDERSTAND = 0xC000,
};

export def TAG_COMMAND_GROUP_LENGTH: tag::dicom_tag       = tag::dicom_tag { group = 0x0000, element = 0x0000 };
export def TAG_AFFECTED_SOP_CLASS_UID: tag::dicom_tag     = tag::dicom_tag { group = 0x0000, element = 0x0002 };
export def TAG_REQUESTED_SOP_CLASS_UID: tag::dicom_tag    = tag::dicom_tag { group = 0x0000, element = 0x0003 };
export def TAG_COMMAND_FIELD: tag::dicom_tag              = tag::dicom_tag { group = 0x0000, element = 0x0100 };
export def TAG_MESSAGE_ID: tag::dicom_tag                 = tag::dicom_tag { group = 0x0000, element = 0x0110 };
export def TAG_MESSAGE_ID_BEING_RESPONDED: tag::dicom_tag = tag::dicom_tag { group = 0x0000, element = 0x0120 };
export def TAG_MOVE_DESTINATION: tag::dicom_tag           = tag::dicom_tag { group = 0x0000, element = 0x0600 };
export def TAG_PRIORITY: tag::dicom_tag                   = tag::dicom_tag { group = 0x0000, element = 0x0700 };
export def TAG_COMMAND_DATA_SET_TYPE: tag::dicom_tag      = tag::dicom_tag { group = 0x0000, element = 0x0800 };
export def TAG_STATUS: tag::dicom_tag                     = tag::dicom_tag { group = 0x0000, element = 0x0900 };
export def TAG_AFFECTED_SOP_INSTANCE_UID: tag::dicom_tag  = tag::dicom_tag { group = 0x0000, element = 0x1000 };

export def NO_DATASET_PRESENT: u16 = 0x0101u16;
export def DATASET_PRESENT: u16    = 0x0001u16;

// Writes a single DICOM element in Implicit VR Little Endian format (used for Command sets).
// Per PS3.7 Section 6.3: Command sets are ALWAYS encoded in Implicit VR Little Endian.
fn write_cmd_elem(
	buf: *types::byte_buffer,
	t: tag::dicom_tag,
	val: []u8,
) (void | types::error_code) = {
	types::buffer_write_u16(buf, t.group, types::endianness::LITTLE)?;
	types::buffer_write_u16(buf, t.element, types::endianness::LITTLE)?;
	types::buffer_write_u32(buf, len(val): u32, types::endianness::LITTLE)?;
	types::buffer_write_bytes(buf, val)?;
};

// Encodes a C-ECHO-RSP command dataset (Success verification response).
export fn encode_c_echo_rsp(
	out: []u8,
	msg_id: u16,
	sop_class_uid: []u8,
	status_code: u16,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	// Affected SOP Class UID (0000, 0002)
	write_cmd_elem(&buf, TAG_AFFECTED_SOP_CLASS_UID, sop_class_uid)?;

	// Command Field (0000, 0100) -> 0x8030 (C-ECHO-RSP)
	const cmd_val: [2]u8 = [0x30u8, 0x80u8];
	write_cmd_elem(&buf, TAG_COMMAND_FIELD, cmd_val[..])?;

	// Message ID Being Responded To (0000, 0120)
	const msg_bytes: [2]u8 = [(msg_id & 0xFFu16): u8, ((msg_id >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_MESSAGE_ID_BEING_RESPONDED, msg_bytes[..])?;

	// Command Data Set Type (0000, 0800) -> 0x0101 (Null/No dataset)
	const no_data: [2]u8 = [0x01u8, 0x01u8];
	write_cmd_elem(&buf, TAG_COMMAND_DATA_SET_TYPE, no_data[..])?;

	// Status (0000, 0900)
	const status_bytes: [2]u8 = [(status_code & 0xFFu16): u8, ((status_code >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_STATUS, status_bytes[..])?;

	return buf.cursor;
};

// Encodes a C-STORE-RSP command dataset (Storage confirmation response).
export fn encode_c_store_rsp(
	out: []u8,
	msg_id: u16,
	sop_class_uid: []u8,
	sop_instance_uid: []u8,
	status_code: u16,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	// Affected SOP Class UID (0000, 0002)
	write_cmd_elem(&buf, TAG_AFFECTED_SOP_CLASS_UID, sop_class_uid)?;

	// Command Field (0000, 0100) -> 0x8001 (C-STORE-RSP)
	const cmd_val: [2]u8 = [0x01u8, 0x80u8];
	write_cmd_elem(&buf, TAG_COMMAND_FIELD, cmd_val[..])?;

	// Message ID Being Responded To (0000, 0120)
	const msg_bytes: [2]u8 = [(msg_id & 0xFFu16): u8, ((msg_id >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_MESSAGE_ID_BEING_RESPONDED, msg_bytes[..])?;

	// Command Data Set Type (0000, 0800) -> 0x0101 (No dataset)
	const no_data: [2]u8 = [0x01u8, 0x01u8];
	write_cmd_elem(&buf, TAG_COMMAND_DATA_SET_TYPE, no_data[..])?;

	// Status (0000, 0900)
	const status_bytes: [2]u8 = [(status_code & 0xFFu16): u8, ((status_code >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_STATUS, status_bytes[..])?;

	// Affected SOP Instance UID (0000, 1000)
	if (len(sop_instance_uid) > 0z) {
		write_cmd_elem(&buf, TAG_AFFECTED_SOP_INSTANCE_UID, sop_instance_uid)?;
	};

	return buf.cursor;
};

// Encodes a C-FIND-RSP command dataset (Query match response).
export fn encode_c_find_rsp(
	out: []u8,
	msg_id: u16,
	sop_class_uid: []u8,
	status_code: u16,
	has_dataset: bool,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	// Affected SOP Class UID (0000, 0002)
	write_cmd_elem(&buf, TAG_AFFECTED_SOP_CLASS_UID, sop_class_uid)?;

	// Command Field (0000, 0100) -> 0x8020 (C-FIND-RSP)
	const cmd_val: [2]u8 = [0x20u8, 0x80u8];
	write_cmd_elem(&buf, TAG_COMMAND_FIELD, cmd_val[..])?;

	// Message ID Being Responded To (0000, 0120)
	const msg_bytes: [2]u8 = [(msg_id & 0xFFu16): u8, ((msg_id >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_MESSAGE_ID_BEING_RESPONDED, msg_bytes[..])?;

	// Command Data Set Type (0000, 0800)
	const data_type_val: [2]u8 = if (has_dataset) [0x01u8, 0x00u8] else [0x01u8, 0x01u8];
	write_cmd_elem(&buf, TAG_COMMAND_DATA_SET_TYPE, data_type_val[..])?;

	// Status (0000, 0900)
	const status_bytes: [2]u8 = [(status_code & 0xFFu16): u8, ((status_code >> 8u16) & 0xFFu16): u8];
	write_cmd_elem(&buf, TAG_STATUS, status_bytes[..])?;

	return buf.cursor;
};
