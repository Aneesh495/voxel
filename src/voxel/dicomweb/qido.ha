// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicomweb::qido
// QIDO-RS (Query based on ID for DICOM Objects) RESTful API per NEMA PS3.18 Section 6.7.

use voxel::types;
use voxel::dicom::json;
use voxel::storage::hierarchy;
use voxel::storage::index;

export type qido_level = enum {
	STUDY_LEVEL,
	SERIES_LEVEL,
	INSTANCE_LEVEL,
};

export type qido_request = struct {
	level: qido_level,
	study_uid: []u8,
	series_uid: []u8,
	filter: index::query_filter,
	limit: size,
	offset: size,
};

// Formats the response header for QIDO-RS JSON queries.
export fn write_qido_response_header(out: []u8, count: size) size = {
	const header_prefix = "HTTP/1.1 200 OK\r\nContent-Type: application/dicom+json\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
	for (let i = 0z; i < len(header_prefix); i += 1z) {
		out[i] = header_prefix[i]: u8;
	};
	return len(header_prefix);
};

// Parses URL query parameters into an index::query_filter structure.
export fn parse_qido_query_params(
	query_str: []u8,
	req: *qido_request,
) void = {
	let idx = 0z;
	while (idx < len(query_str)) {
		let key_start = idx;
		while (idx < len(query_str) && query_str[idx] != '=' && query_str[idx] != '&') {
			idx += 1z;
		};
		const key = query_str[key_start..idx];

		let val: []u8 = [];
		if (idx < len(query_str) && query_str[idx] == '=') {
			idx += 1z;
			let val_start = idx;
			while (idx < len(query_str) && query_str[idx] != '&') {
				idx += 1z;
			};
			val = query_str[val_start..idx];
		};

		if (idx < len(query_str) && query_str[idx] == '&') {
			idx += 1z;
		};

		// Match keys
		if (hierarchy::bytes_equal(key, ['P', 'a', 't', 'i', 'e', 'n', 't', 'I', 'D'])) {
			req.filter.patient_id = val;
		} else if (hierarchy::bytes_equal(key, ['P', 'a', 't', 'i', 'e', 'n', 't', 'N', 'a', 'm', 'e'])) {
			req.filter.patient_name = val;
		} else if (hierarchy::bytes_equal(key, ['M', 'o', 'd', 'a', 'l', 'i', 't', 'y'])) {
			req.filter.modality = val;
		} else if (hierarchy::bytes_equal(key, ['A', 'c', 'c', 'e', 's', 's', 'i', 'o', 'n', 'N', 'u', 'm', 'b', 'e', 'r'])) {
			req.filter.accession_number = val;
		} else if (hierarchy::bytes_equal(key, ['S', 't', 'u', 'd', 'y', 'D', 'a', 't', 'e'])) {
			// Check if range: YYYYMMDD-YYYYMMDD
			let dash_pos: size = 0xFFFFz;
			for (let d = 0z; d < len(val); d += 1z) {
				if (val[d] == '-') {
					dash_pos = d;
					break;
				};
			};
			if (dash_pos != 0xFFFFz) {
				req.filter.study_date_start = val[..dash_pos];
				req.filter.study_date_end = val[dash_pos + 1z..];
			} else {
				req.filter.study_date_start = val;
				req.filter.study_date_end = val;
			};
		};
	};
};
