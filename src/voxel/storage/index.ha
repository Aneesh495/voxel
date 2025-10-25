// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::storage::index
// In-memory indexing over clinical DICOM keys, wildcard search, and range filtering.

use voxel::types;
use voxel::storage::hierarchy;

export type query_filter = struct {
	patient_id: []u8,
	patient_name: []u8,
	study_date_start: []u8,
	study_date_end: []u8,
	modality: []u8,
	accession_number: []u8,
	study_instance_uid: []u8,
};

// Returns true if text matches pattern with optional '*' wildcard.
export fn wildcard_match(text: []u8, pattern: []u8) bool = {
	if (len(pattern) == 0z) {
		return true; // Empty pattern matches all
	};

	let t_idx = 0z;
	let p_idx = 0z;
	let star_idx: size = 0xFFFFz;
	let match_idx = 0z;

	while (t_idx < len(text)) {
		if (p_idx < len(pattern) && (pattern[p_idx] == '?' || pattern[p_idx] == text[t_idx])) {
			t_idx += 1z;
			p_idx += 1z;
		} else if (p_idx < len(pattern) && pattern[p_idx] == '*') {
			star_idx = p_idx;
			match_idx = t_idx;
			p_idx += 1z;
		} else if (star_idx != 0xFFFFz) {
			p_idx = star_idx + 1z;
			match_idx += 1z;
			t_idx = match_idx;
		} else {
			return false;
		};
	};

	while (p_idx < len(pattern) && pattern[p_idx] == '*') {
		p_idx += 1z;
	};

	return p_idx == len(pattern);
};

// Evaluates whether a study record matches the active query filter.
export fn matches_study_query(
	patient: *hierarchy::patient_node,
	study: *hierarchy::study_node,
	filter: *query_filter,
) bool = {
	// 1. PatientID filter
	if (len(filter.patient_id) > 0z) {
		if (!wildcard_match(patient.patient_id[..patient.patient_id_len], filter.patient_id)) {
			return false;
		};
	};

	// 2. PatientName filter
	if (len(filter.patient_name) > 0z) {
		if (!wildcard_match(patient.patient_name[..patient.patient_name_len], filter.patient_name)) {
			return false;
		};
	};

	// 3. AccessionNumber filter
	if (len(filter.accession_number) > 0z) {
		if (!wildcard_match(study.accession_number[..study.accession_number_len], filter.accession_number)) {
			return false;
		};
	};

	// 4. StudyInstanceUID filter
	if (len(filter.study_instance_uid) > 0z) {
		if (!hierarchy::bytes_equal(study.study_instance_uid[..study.study_instance_uid_len], filter.study_instance_uid)) {
			return false;
		};
	};

	// 5. StudyDate range filter (YYYYMMDD)
	if (len(filter.study_date_start) >= 8z && study.study_date_len >= 8z) {
		for (let i = 0z; i < 8z; i += 1z) {
			if (study.study_date[i] < filter.study_date_start[i]) return false;
			if (study.study_date[i] > filter.study_date_start[i]) break;
		};
	};

	if (len(filter.study_date_end) >= 8z && study.study_date_len >= 8z) {
		for (let i = 0z; i < 8z; i += 1z) {
			if (study.study_date[i] > filter.study_date_end[i]) return false;
			if (study.study_date[i] < filter.study_date_end[i]) break;
		};
	};

	return true;
};
