// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicomweb::stow
// STOW-RS (Store Over the Web) multipart/related DICOM ingestion per PS3.18 Section 6.6.

use voxel::types;
use voxel::dicom::dataset;
use voxel::dicom::part10;

export type stow_result = struct {
	stored_instances: size,
	failed_instances: size,
	study_instance_uid: [64]u8,
	study_instance_uid_len: size,
};

// Extracts boundary parameter from Content-Type: multipart/related; boundary=...
export fn extract_multipart_boundary(content_type_header: []u8, out_boundary: []u8) size = {
	const key = ['b', 'o', 'u', 'n', 'd', 'a', 'r', 'y', '='];
	let idx = 0z;

	while (idx + len(key) <= len(content_type_header)) {
		let match_found = true;
		for (let k = 0z; k < len(key); k += 1z) {
			if (content_type_header[idx + k] != key[k]) {
				match_found = false;
				break;
			};
		};

		if (match_found) {
			let start = idx + len(key);
			// Trim leading quotes if present
			if (start < len(content_type_header) && content_type_header[start] == '"') {
				start += 1z;
			};
			let end = start;
			while (end < len(content_type_header) &&
				content_type_header[end] != '"' &&
				content_type_header[end] != ';' &&
				content_type_header[end] != '\r' &&
				content_type_header[end] != ' ') {
				end += 1z;
			};
			const b_len = end - start;
			const copy_len = if (b_len > len(out_boundary)) len(out_boundary) else b_len;
			for (let i = 0z; i < copy_len; i += 1z) {
				out_boundary[i] = content_type_header[start + i];
			};
			return copy_len;
		};

		idx += 1z;
	};

	return 0z;
};

// Formats STOW-RS 200 OK XML response indicating stored instances.
export fn format_stow_success_response(out: []u8, stored_count: size) size = {
	let buf = types::buffer_init(out);

	types::buffer_write_bytes(&buf, [
		'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '2', '0', '0', ' ', 'O', 'K', '\r', '\n',
		'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e', ':', ' ', 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/', 'd', 'i', 'c', 'o', 'm', '+', 'x', 'm', 'l', '\r', '\n',
		'A', 'c', 'c', 'e', 's', 's', '-', 'C', 'o', 'n', 't', 'r', 'o', 'l', '-', 'A', 'l', 'l', 'o', 'w', '-', 'O', 'r', 'i', 'g', 'i', 'n', ':', ' ', '*', '\r', '\n',
		'\r', '\n',
		'<', '?', 'x', 'm', 'l', ' ', 'v', 'e', 'r', 's', 'i', 'o', 'n', '=', '"', '1', '.', '0', '"', '?', '>',
		'<', 'N', 'a', 't', 'i', 'v', 'e', 'D', 'i', 'c', 'o', 'm', 'M', 'o', 'd', 'e', 'l', '>',
		'<', '/', 'N', 'a', 't', 'i', 'v', 'e', 'D', 'i', 'c', 'o', 'm', 'M', 'o', 'd', 'e', 'l', '>'
	])!;

	return buf.cursor;
};
