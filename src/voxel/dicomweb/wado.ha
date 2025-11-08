// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicomweb::wado
// WADO-RS (Web Access to DICOM Objects) RESTful API per NEMA PS3.18 Section 6.5.

use voxel::types;
use voxel::volume::lut;
use voxel::volume::mpr;

export type wado_endpoint_type = enum {
	STUDY_METADATA,
	SERIES_METADATA,
	INSTANCE_DICOM,
	INSTANCE_RENDERED,
	FRAME_RENDERED,
	SERIES_MPR_RENDERED,
};

export type wado_request = struct {
	endpoint: wado_endpoint_type,
	study_uid: []u8,
	series_uid: []u8,
	instance_uid: []u8,
	frame_number: size,
	window_level: lut::window_level,
	mpr_plane: mpr::mpr_plane,
	mpr_slice: size,
};

// Formats a multipart/related HTTP response header for WADO-RS DICOM instance streaming.
export fn write_wado_multipart_header(
	out: []u8,
	boundary: []u8,
	content_len: size,
) size = {
	let buf = types::buffer_init(out);

	types::buffer_write_bytes(&buf, [
		'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '2', '0', '0', ' ', 'O', 'K', '\r', '\n'
	])!;

	types::buffer_write_bytes(&buf, [
		'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e', ':', ' ',
		'm', 'u', 'l', 't', 'i', 'p', 'a', 'r', 't', '/', 'r', 'e', 'l', 'a', 't', 'e', 'd', ';', ' ',
		't', 'y', 'p', 'e', '=', '"', 'a', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n', '/', 'd', 'i', 'c', 'o', 'm', '"', ';', ' ',
		'b', 'o', 'u', 'n', 'd', 'a', 'r', 'y', '='
	])!;
	types::buffer_write_bytes(&buf, boundary)!;
	types::buffer_write_bytes(&buf, ['\r', '\n'])!;

	types::buffer_write_bytes(&buf, [
		'A', 'c', 'c', 'e', 's', 's', '-', 'C', 'o', 'n', 't', 'r', 'o', 'l', '-', 'A', 'l', 'l', 'o', 'w', '-', 'O', 'r', 'i', 'g', 'i', 'n', ':', ' ', '*', '\r', '\n',
		'\r', '\n'
	])!;

	return buf.cursor;
};

// Formats the single-part PNG response header for /rendered requests.
export fn write_wado_rendered_header(
	out: []u8,
	image_bytes_len: size,
) size = {
	let buf = types::buffer_init(out);

	types::buffer_write_bytes(&buf, [
		'H', 'T', 'T', 'P', '/', '1', '.', '1', ' ', '2', '0', '0', ' ', 'O', 'K', '\r', '\n',
		'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e', ':', ' ', 'i', 'm', 'a', 'g', 'e', '/', 'p', 'n', 'g', '\r', '\n',
		'A', 'c', 'c', 'e', 's', 's', '-', 'C', 'o', 'n', 't', 'r', 'o', 'l', '-', 'A', 'l', 'l', 'o', 'w', '-', 'O', 'r', 'i', 'g', 'i', 'n', ':', ' ', '*', '\r', '\n',
		'C', 'a', 'c', 'h', 'e', '-', 'C', 'o', 'n', 't', 'r', 'o', 'l', ':', ' ', 'p', 'u', 'b', 'l', 'i', 'c', ',', ' ', 'm', 'a', 'x', '-', 'a', 'g', 'e', '=', '3', '6', '0', '0', '\r', '\n',
		'\r', '\n'
	])!;

	return buf.cursor;
};
