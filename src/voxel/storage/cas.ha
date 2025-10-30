// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::storage::cas
// Content-Addressable Storage (CAS) with 2-level sharding, SHA-256 deduplication,
// and atomic file creation.

use voxel::types;

export type cas_digest = [32]u8;

// Formats a 32-byte binary digest into a 64-character lowercase hexadecimal string.
export fn digest_to_hex(digest: *cas_digest, out_hex: []u8) void = {
	const hex_chars = "0123456789abcdef";
	for (let i = 0z; i < 32z; i += 1z) {
		const b = digest[i];
		out_hex[i * 2z] = hex_chars[(b >> 4u8) & 0x0Fu8]: u8;
		out_hex[i * 2z + 1z] = hex_chars[b & 0x0Fu8]: u8;
	};
};

// Generates a 2-level sharded storage path: "prefix/ab/cd/abcdef0123..."
export fn build_sharded_path(
	prefix: []u8,
	digest_hex: []u8,
	out_path: []u8,
) size = {
	let cursor = 0z;

	// Copy prefix
	for (let i = 0z; i < len(prefix); i += 1z) {
		out_path[cursor] = prefix[i];
		cursor += 1z;
	};

	if (cursor > 0z && out_path[cursor - 1z] != '/') {
		out_path[cursor] = '/';
		cursor += 1z;
	};

	// Level 1 directory: first 2 hex chars
	out_path[cursor] = digest_hex[0];
	out_path[cursor + 1z] = digest_hex[1];
	out_path[cursor + 2z] = '/';
	cursor += 3z;

	// Level 2 directory: next 2 hex chars
	out_path[cursor] = digest_hex[2];
	out_path[cursor + 1z] = digest_hex[3];
	out_path[cursor + 2z] = '/';
	cursor += 3z;

	// Full 64-char filename
	for (let i = 0z; i < 64z; i += 1z) {
		out_path[cursor] = digest_hex[i];
		cursor += 1z;
	};

	return cursor;
};
