// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dimse::association
// DICOM Association Negotiation state machine, presentation context matching,
// Application Entity validation, and A-ASSOCIATE-AC encoder per PS3.8.

use voxel::types;
use voxel::dicom::uid;
use voxel::dimse::pdu;

export type association_state = enum {
	STA1_IDLE = 1,
	STA2_TRANSPORT_OPEN = 2,
	STA6_ASSOCIATION_ESTABLISHED = 6,
	STA7_AWAITING_RELEASE_RP = 7,
	STA8_AWAITING_LOCAL_RELEASE_RP = 8,
	STA13_TRANSPORT_CLOSED = 13,
};

export type negotiated_context = struct {
	context_id: u8,
	abstract_syntax: [64]u8,
	abstract_syntax_len: size,
	transfer_syntax: [64]u8,
	transfer_syntax_len: size,
	is_accepted: bool,
};

export type association = struct {
	state: association_state,
	calling_ae: [16]u8,
	called_ae: [16]u8,
	max_pdu_received: u32,
	max_pdu_send: u32,
	contexts: [32]negotiated_context,
	context_count: size,
};

// Initializes a blank association structure.
export fn association_init(called_title: []u8) association = {
	let assoc = association {
		state = association_state::STA1_IDLE,
		calling_ae = [' '...],
		called_ae = [' '...],
		max_pdu_received = 16384u32,
		max_pdu_send = 65536u32,
		contexts = [negotiated_context {
			context_id = 0u8,
			abstract_syntax = [0...],
			abstract_syntax_len = 0z,
			transfer_syntax = [0...],
			transfer_syntax_len = 0z,
			is_accepted = false,
		}...],
		context_count = 0z,
	};

	const c_len = if (len(called_title) > 16z) 16z else len(called_title);
	for (let i = 0z; i < c_len; i += 1z) {
		assoc.called_ae[i] = called_title[i];
	};

	return assoc;
};

// Evaluates whether an abstract syntax is supported by this PACS server.
export fn is_abstract_syntax_supported(syntax: []u8) bool = {
	const supported: [_][]u8 = [
		// Verification SOP Class (C-ECHO)
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '1'],
		// CT Image Storage
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '1', '.', '2'],
		// Enhanced CT Image Storage
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '1', '.', '2', '.', '1'],
		// MR Image Storage
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '1', '.', '4'],
		// Secondary Capture
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '1', '.', '7'],
		// Patient Root Q/R Find
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '2', '.', '1', '.', '1'],
		// Study Root Q/R Find
		['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '5', '.', '1', '.', '4', '.', '1', '.', '2', '.', '2', '.', '1'],
	];

	for (let i = 0z; i < len(supported); i += 1z) {
		const s = supported[i];
		if (len(s) == len(syntax)) {
			let eq = true;
			for (let b = 0z; b < len(s); b += 1z) {
				if (s[b] != syntax[b]) {
					eq = false;
					break;
				};
			};
			if (eq) return true;
		};
	};

	return false;
};

// Selects the preferred Transfer Syntax among a list of proposed transfer syntaxes.
// Preference order: Explicit VR Little Endian > Implicit VR Little Endian.
export fn select_transfer_syntax(proposed: [][]u8) ([]u8 | void) = {
	const explicit_le = ['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '2', '.', '1'];
	const implicit_le = ['1', '.', '2', '.', '8', '4', '0', '.', '1', '0', '0', '0', '8', '.', '1', '.', '2'];

	// 1. Try to find Explicit VR Little Endian
	for (let i = 0z; i < len(proposed); i += 1z) {
		const p = proposed[i];
		if (len(p) == len(explicit_le)) {
			let match_found = true;
			for (let b = 0z; b < len(p); b += 1z) {
				if (p[b] != explicit_le[b]) {
					match_found = false;
					break;
				};
			};
			if (match_found) return explicit_le;
		};
	};

	// 2. Fall back to Implicit VR Little Endian
	for (let i = 0z; i < len(proposed); i += 1z) {
		const p = proposed[i];
		if (len(p) == len(implicit_le)) {
			let match_found = true;
			for (let b = 0z; b < len(p); b += 1z) {
				if (p[b] != implicit_le[b]) {
					match_found = false;
					break;
				};
			};
			if (match_found) return implicit_le;
		};
	};

	return;
};

// Encodes an A-ASSOCIATE-AC PDU in response to an association request.
export fn encode_associate_ac(
	assoc: *association,
	out: []u8,
) (size | types::error_code) = {
	let buf = types::buffer_init(out);

	// Reserve 6 bytes for PDU header, will write actual length at end
	buf.cursor = 6z;

	// Protocol version: 0x0001
	types::buffer_write_u16(&buf, 0x0001u16, types::endianness::BIG)?;

	// Reserved 2 bytes: 0x0000
	types::buffer_write_u16(&buf, 0x0000u16, types::endianness::BIG)?;

	// Called AE Title (16 bytes)
	types::buffer_write_bytes(&buf, assoc.called_ae[..])?;

	// Calling AE Title (16 bytes)
	types::buffer_write_bytes(&buf, assoc.calling_ae[..])?;

	// Reserved 32 bytes (0x00)
	for (let i = 0z; i < 32z; i += 1z) {
		types::buffer_write_u8(&buf, 0x00u8)?;
	};

	// 1. Application Context Item (0x10)
	const app_context = "1.2.840.10008.3.1.1.1";
	types::buffer_write_u8(&buf, 0x10u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u16(&buf, len(app_context): u16, types::endianness::BIG)?;
	for (let i = 0z; i < len(app_context); i += 1z) {
		types::buffer_write_u8(&buf, app_context[i]: u8)?;
	};

	// 2. Presentation Context Acceptance Items (0x21)
	for (let i = 0z; i < assoc.context_count; i += 1z) {
		const ctx = assoc.contexts[i];
		types::buffer_write_u8(&buf, 0x21u8)?; // Item type 0x21
		types::buffer_write_u8(&buf, 0x00u8)?; // Reserved

		const item_len_pos = buf.cursor;
		types::buffer_write_u16(&buf, 0u16, types::endianness::BIG)?; // Placeholder length

		types::buffer_write_u8(&buf, ctx.context_id)?; // Context ID
		types::buffer_write_u8(&buf, 0x00u8)?;         // Reserved
		const result: u8 = if (ctx.is_accepted) 0u8 else 3u8; // 0 = Acceptance, 3 = Abstract Syntax Not Supported
		types::buffer_write_u8(&buf, result)?;
		types::buffer_write_u8(&buf, 0x00u8)?;         // Reserved

		// Sub-item 0x40: Transfer Syntax
		if (ctx.is_accepted && ctx.transfer_syntax_len > 0z) {
			types::buffer_write_u8(&buf, 0x40u8)?;
			types::buffer_write_u8(&buf, 0x00u8)?;
			types::buffer_write_u16(&buf, ctx.transfer_syntax_len: u16, types::endianness::BIG)?;
			types::buffer_write_bytes(&buf, ctx.transfer_syntax[..ctx.transfer_syntax_len])?;
		};

		// Backfill item length
		const item_len = (buf.cursor - item_len_pos - 2z): u16;
		buf.data[item_len_pos] = ((item_len >> 8u16) & 0xFFu16): u8;
		buf.data[item_len_pos + 1z] = (item_len & 0xFFu16): u8;
	};

	// 3. User Information Item (0x50)
	types::buffer_write_u8(&buf, 0x50u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	const user_info_len_pos = buf.cursor;
	types::buffer_write_u16(&buf, 0u16, types::endianness::BIG)?;

	// Sub-item 0x51: Maximum Length Item (4 bytes max PDU size)
	types::buffer_write_u8(&buf, 0x51u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u16(&buf, 4u16, types::endianness::BIG)?;
	types::buffer_write_u32(&buf, assoc.max_pdu_send, types::endianness::BIG)?;

	// Sub-item 0x52: Implementation Class UID
	const impl_uid = uid::VOXEL_IMPLEMENTATION_CLASS_UID;
	types::buffer_write_u8(&buf, 0x52u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u16(&buf, len(impl_uid): u16, types::endianness::BIG)?;
	for (let i = 0z; i < len(impl_uid); i += 1z) {
		types::buffer_write_u8(&buf, impl_uid[i]: u8)?;
	};

	// Sub-item 0x55: Implementation Version Name
	const impl_ver = uid::VOXEL_IMPLEMENTATION_VERSION_NAME;
	types::buffer_write_u8(&buf, 0x55u8)?;
	types::buffer_write_u8(&buf, 0x00u8)?;
	types::buffer_write_u16(&buf, len(impl_ver): u16, types::endianness::BIG)?;
	for (let i = 0z; i < len(impl_ver); i += 1z) {
		types::buffer_write_u8(&buf, impl_ver[i]: u8)?;
	};

	// Backfill User Information Item Length
	const user_info_len = (buf.cursor - user_info_len_pos - 2z): u16;
	buf.data[user_info_len_pos] = ((user_info_len >> 8u16) & 0xFFu16): u8;
	buf.data[user_info_len_pos + 1z] = (user_info_len & 0xFFu16): u8;

	// Backfill PDU Header
	const total_pdu_len = (buf.cursor - 6z): u32;
	buf.data[0] = pdu::pdu_type::A_ASSOCIATE_AC: u8;
	buf.data[1] = 0x00u8;
	buf.data[2] = ((total_pdu_len >> 24u32) & 0xFFu32): u8;
	buf.data[3] = ((total_pdu_len >> 16u32) & 0xFFu32): u8;
	buf.data[4] = ((total_pdu_len >> 8u32) & 0xFFu32): u8;
	buf.data[5] = (total_pdu_len & 0xFFu32): u8;

	assoc.state = association_state::STA6_ASSOCIATION_ESTABLISHED;
	return buf.cursor;
};
