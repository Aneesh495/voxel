// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::storage::hierarchy
// 4-tier relational DICOM information model (Patient -> Study -> Series -> Instance)
// with relational links, tree traversal, and metadata aggregation per PS3.3.

use voxel::types;

export type instance_node = struct {
	sop_instance_uid: [64]u8,
	sop_instance_uid_len: size,
	sop_class_uid: [64]u8,
	sop_class_uid_len: size,
	instance_number: i32,
	rows: u16,
	cols: u16,
	sha256_digest: [32]u8,
	file_size: size,
};

export type series_node = struct {
	series_instance_uid: [64]u8,
	series_instance_uid_len: size,
	modality: [16]u8,
	modality_len: size,
	series_number: i32,
	series_description: [64]u8,
	series_description_len: size,
	body_part: [32]u8,
	body_part_len: size,
	instance_count: size,
};

export type study_node = struct {
	study_instance_uid: [64]u8,
	study_instance_uid_len: size,
	study_date: [16]u8,
	study_date_len: size,
	study_time: [16]u8,
	study_time_len: size,
	accession_number: [64]u8,
	accession_number_len: size,
	study_description: [64]u8,
	study_description_len: size,
	referring_physician: [64]u8,
	referring_physician_len: size,
	series_count: size,
};

export type patient_node = struct {
	patient_id: [64]u8,
	patient_id_len: size,
	patient_name: [64]u8,
	patient_name_len: size,
	birth_date: [16]u8,
	birth_date_len: size,
	sex: [8]u8,
	sex_len: size,
	study_count: size,
};

// Helper: compares two byte strings for exact equality.
export fn bytes_equal(a: []u8, b: []u8) bool = {
	if (len(a) != len(b)) return false;
	for (let i = 0z; i < len(a); i += 1z) {
		if (a[i] != b[i]) return false;
	};
	return true;
};

// Helper: copies a string slice into a fixed-capacity destination buffer.
export fn copy_string_bounded(dest: []u8, src: []u8) size = {
	const count = if (len(src) > len(dest)) len(dest) else len(src);
	for (let i = 0z; i < count; i += 1z) {
		dest[i] = src[i];
	};
	return count;
};
