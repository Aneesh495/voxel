// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::net::http
// Lightweight, zero-dependency HTTP/1.1 request parser, route dispatcher,
// and CORS preflight handler per RFC 7230 / RFC 7231.

use voxel::types;

export type http_method = enum {
	GET,
	POST,
	OPTIONS,
	HEAD,
	UNKNOWN,
};

export type http_request = struct {
	method: http_method,
	path: []u8,
	query: []u8,
	content_length: size,
	content_type: []u8,
	body: []u8,
	keep_alive: bool,
};

// Parses the HTTP method token from the start of the request line.
export fn parse_http_method(raw: []u8) (http_method, size) = {
	if (len(raw) >= 4z && raw[0] == 'G' && raw[1] == 'E' && raw[2] == 'T' && raw[3] == ' ') {
		return (http_method::GET, 4z);
	};
	if (len(raw) >= 5z && raw[0] == 'P' && raw[1] == 'O' && raw[2] == 'S' && raw[3] == 'T' && raw[4] == ' ') {
		return (http_method::POST, 5z);
	};
	if (len(raw) >= 8z && raw[0] == 'O' && raw[1] == 'P' && raw[2] == 'T' && raw[3] == 'I' && raw[4] == 'O' && raw[5] == 'N' && raw[6] == 'S' && raw[7] == ' ') {
		return (http_method::OPTIONS, 8z);
	};
	if (len(raw) >= 5z && raw[0] == 'H' && raw[1] == 'E' && raw[2] == 'A' && raw[3] == 'D' && raw[4] == ' ') {
		return (http_method::HEAD, 5z);
	};
	return (http_method::UNKNOWN, 0z);
};

// Parses an incoming HTTP/1.1 request buffer into an http_request structure.
export fn parse_http_request(raw: []u8) (http_request | types::error_code) = {
	const (method, m_len) = parse_http_method(raw);
	if (method == http_method::UNKNOWN) {
		return types::error_code::PARSE_ERROR;
	};

	let cursor = m_len;

	// Extract path and query
	let path_start = cursor;
	let query_start: size = 0xFFFFz;

	while (cursor < len(raw) && raw[cursor] != ' ' && raw[cursor] != '\r') {
		if (raw[cursor] == '?' && query_start == 0xFFFFz) {
			query_start = cursor;
		};
		cursor += 1z;
	};

	let path: []u8 = [];
	let query: []u8 = [];

	if (query_start != 0xFFFFz) {
		path = raw[path_start..query_start];
		query = raw[query_start + 1z..cursor];
	} else {
		path = raw[path_start..cursor];
	};

	// Advance past HTTP version to headers line
	while (cursor < len(raw) && raw[cursor] != '\n') {
		cursor += 1z;
	};
	if (cursor < len(raw) && raw[cursor] == '\n') {
		cursor += 1z;
	};

	let content_len = 0z;
	let content_type: []u8 = [];
	let keep_alive = true;

	// Parse header lines
	while (cursor < len(raw)) {
		if (cursor + 1z < len(raw) && raw[cursor] == '\r' && raw[cursor + 1z] == '\n') {
			cursor += 2z;
			break; // Empty line signals start of body
		};
		if (raw[cursor] == '\n') {
			cursor += 1z;
			break;
		};

		let line_start = cursor;
		while (cursor < len(raw) && raw[cursor] != '\r' && raw[cursor] != '\n') {
			cursor += 1z;
		};
		const line = raw[line_start..cursor];
		if (cursor < len(raw) && raw[cursor] == '\r') cursor += 1z;
		if (cursor < len(raw) && raw[cursor] == '\n') cursor += 1z;

		// Match header: Content-Length:
		const cl_prefix = ['C', 'o', 'n', 't', 'e', 'n', 't', '-', 'L', 'e', 'n', 'g', 't', 'h', ':'];
		if (len(line) > len(cl_prefix)) {
			let is_cl = true;
			for (let i = 0z; i < len(cl_prefix); i += 1z) {
				const c1 = line[i];
				const c2 = cl_prefix[i];
				if (c1 != c2 && c1 != (c2 + 32u8)) {
					is_cl = false;
					break;
				};
			};
			if (is_cl) {
				let v_start = len(cl_prefix);
				while (v_start < len(line) && line[v_start] == ' ') v_start += 1z;
				let val = 0z;
				for (let i = v_start; i < len(line); i += 1z) {
					if (line[i] >= '0' && line[i] <= '9') {
						val = val * 10z + (line[i] - '0'): size;
					} else {
						break;
					};
				};
				content_len = val;
			};
		};

		// Match header: Content-Type:
		const ct_prefix = ['C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e', ':'];
		if (len(line) > len(ct_prefix)) {
			let is_ct = true;
			for (let i = 0z; i < len(ct_prefix); i += 1z) {
				const c1 = line[i];
				const c2 = ct_prefix[i];
				if (c1 != c2 && c1 != (c2 + 32u8)) {
					is_ct = false;
					break;
				};
			};
			if (is_ct) {
				let v_start = len(ct_prefix);
				while (v_start < len(line) && line[v_start] == ' ') v_start += 1z;
				content_type = line[v_start..];
			};
		};
	};

	const body = if (cursor < len(raw)) raw[cursor..] else [];

	return http_request {
		method = method,
		path = path,
		query = query,
		content_length = content_len,
		content_type = content_type,
		body = body,
		keep_alive = keep_alive,
	};
};

// Formats a standard 200 OK CORS preflight response for OPTIONS requests.
export fn write_cors_preflight_response(out: []u8) size = {
	const resp = "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Authorization, Accept\r\nAccess-Control-Max-Age: 86400\r\n\r\n";
	for (let i = 0z; i < len(resp); i += 1z) {
		out[i] = resp[i]: u8;
	};
	return len(resp);
};
