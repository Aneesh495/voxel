// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dimse::service
// Service Class Provider (SCP) dispatcher, processing incoming DIMSE-C commands
// and orchestrating DICOM storage, query, and verification workflows.

use voxel::types;
use voxel::dicom::dataset;
use voxel::dicom::element;
use voxel::dicom::tag;
use voxel::dicom::vr;
use voxel::dimse::association;
use voxel::dimse::command;
use voxel::dimse::pdu;
use voxel::dimse::pdv;

export type service_handler = struct {
	assoc: *association::association,
	active_context_id: u8,
	current_command: command::command_type,
	current_message_id: u16,
	has_pending_dataset: bool,
};

// Initializes a service handler bound to an active association.
export fn service_handler_init(assoc: *association::association) service_handler = {
	return service_handler {
		assoc = assoc,
		active_context_id = 0u8,
		current_command = command::command_type::C_ECHO_RQ,
		current_message_id = 0u16,
		has_pending_dataset = false,
	};
};

// Processes a decoded command dataset and determines required action.
export fn dispatch_command(
	handler: *service_handler,
	cmd_dataset: *dataset::dataset,
	out_pdu_buf: []u8,
) (size | types::error_code) = {
	// Extract Command Field (0000, 0100)
	const cmd_field = match (dataset::dataset_get_u16(cmd_dataset, command::TAG_COMMAND_FIELD, types::endianness::LITTLE)) {
	case let c: u16 => c;
	case void => return types::error_code::PARSE_ERROR;
	};

	// Extract Message ID (0000, 0110)
	const msg_id = match (dataset::dataset_get_u16(cmd_dataset, command::TAG_MESSAGE_ID, types::endianness::LITTLE)) {
	case let m: u16 => m;
	case void => 0u16;
	};
	handler.current_message_id = msg_id;

	// Extract Affected SOP Class UID (0000, 0002)
	const sop_class_raw = match (dataset::dataset_get_string_bytes(cmd_dataset, command::TAG_AFFECTED_SOP_CLASS_UID)) {
	case let s: []u8 => s;
	case void => [];
	};

	// Branch by command type
	switch (cmd_field) {
	case command::command_type::C_ECHO_RQ:
		// Handle C-ECHO Verification: respond with C-ECHO-RSP Success (0x0000)
		let rsp_payload: [256]u8 = [0...];
		const rsp_len = command::encode_c_echo_rsp(
			rsp_payload[..],
			msg_id,
			sop_class_raw,
			command::dimse_status::SUCCESS: u16,
		)?;

		// Package command dataset into P-DATA-TF PDU
		return pdv::encode_p_data_tf_single(
			out_pdu_buf,
			handler.active_context_id,
			true,  // is_command = true
			true,  // is_last = true
			rsp_payload[..rsp_len],
		);

	case command::command_type::C_STORE_RQ:
		// Note: A C-STORE-RQ command has an accompanying dataset containing the image pixels.
		// Flag that incoming data PDVs belong to this store operation.
		handler.current_command = command::command_type::C_STORE_RQ;
		handler.has_pending_dataset = true;
		return 0z; // Awaiting data PDV

	case command::command_type::C_FIND_RQ:
		// A C-FIND-RQ command has an accompanying identifier dataset with search keys.
		handler.current_command = command::command_type::C_FIND_RQ;
		handler.has_pending_dataset = true;
		return 0z; // Awaiting identifier dataset

	case =>
		// Unrecognized command
		return types::error_code::INVALID_ARGUMENT;
	};
};

// Generates a C-STORE-RSP confirmation after full dataset ingestion.
export fn complete_c_store(
	handler: *service_handler,
	sop_class_uid: []u8,
	sop_instance_uid: []u8,
	status_code: u16,
	out_pdu_buf: []u8,
) (size | types::error_code) = {
	let rsp_payload: [512]u8 = [0...];
	const rsp_len = command::encode_c_store_rsp(
		rsp_payload[..],
		handler.current_message_id,
		sop_class_uid,
		sop_instance_uid,
		status_code,
	)?;

	handler.has_pending_dataset = false;

	return pdv::encode_p_data_tf_single(
		out_pdu_buf,
		handler.active_context_id,
		true, // is_command
		true, // is_last
		rsp_payload[..rsp_len],
	);
};

// Generates final C-FIND-RSP completing a query transaction.
export fn complete_c_find(
	handler: *service_handler,
	sop_class_uid: []u8,
	status_code: u16,
	out_pdu_buf: []u8,
) (size | types::error_code) = {
	let rsp_payload: [512]u8 = [0...];
	const rsp_len = command::encode_c_find_rsp(
		rsp_payload[..],
		handler.current_message_id,
		sop_class_uid,
		status_code,
		false, // has_dataset = false for final completion
	)?;

	handler.has_pending_dataset = false;

	return pdv::encode_p_data_tf_single(
		out_pdu_buf,
		handler.active_context_id,
		true, // is_command
		true, // is_last
		rsp_payload[..rsp_len],
	);
};
