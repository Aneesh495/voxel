// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::dataset
// In-memory DICOM Dataset container, maintaining canonical tag ordering,
// rapid tag indexing, insertion, lookup, and deletion per DICOM PS3.5.

use voxel::types;
use voxel::dicom::element;
use voxel::dicom::tag;
use voxel::dicom::vr;

export type dataset = struct {
	elements: []element::data_element,
	count: size,
};

// Initializes an empty DICOM dataset with capacity for elements.
export fn dataset_init(storage: []element::data_element) dataset = {
	return dataset {
		elements = storage,
		count = 0z,
	};
};

// Returns the number of elements contained in the dataset.
export fn dataset_count(ds: *dataset) size = {
	return ds.count;
};

// Returns the element at a specific ordinal index.
export fn dataset_get_at(ds: *dataset, idx: size) (element::data_element | void) = {
	if (idx >= ds.count) {
		return;
	};
	return ds.elements[idx];
};

// Performs a binary search over the canonically sorted dataset for a specific tag.
export fn dataset_find_index(ds: *dataset, t: tag::dicom_tag) (size | void) = {
	if (ds.count == 0z) {
		return;
	};

	let low = 0z;
	let high = ds.count - 1z;

	while (low <= high) {
		const mid = low + (high - low) / 2z;
		const mid_tag = ds.elements[mid].tag;

		if (tag::tag_equal(mid_tag, t)) {
			return mid;
		};

		if (tag::tag_less(mid_tag, t)) {
			low = mid + 1z;
		} else {
			if (mid == 0z) {
				break;
			};
			high = mid - 1z;
		};
	};

	return;
};

// Looks up an element in the dataset by tag.
export fn dataset_get(ds: *dataset, t: tag::dicom_tag) (element::data_element | void) = {
	match (dataset_find_index(ds, t)) {
	case let idx: size =>
		return ds.elements[idx];
	case void =>
		return;
	};
};

// Inserts an element into the dataset maintaining canonical ascending tag ordering.
// If the tag already exists, its value is updated in place.
export fn dataset_insert(ds: *dataset, elem: element::data_element) (void | types::error_code) = {
	match (dataset_find_index(ds, elem.tag)) {
	case let idx: size =>
		// In-place replacement of existing element
		ds.elements[idx] = elem;
		return;
	case void =>
		if (ds.count >= len(ds.elements)) {
			return types::error_code::BUFFER_OVERFLOW;
		};

		// Find insertion point to maintain sorted tag ordering
		let insert_pos = ds.count;
		for (let i = 0z; i < ds.count; i += 1z) {
			if (tag::tag_less(elem.tag, ds.elements[i].tag)) {
				insert_pos = i;
				break;
			};
		};

		// Shift subsequent elements forward
		for (let i = ds.count; i > insert_pos; i -= 1z) {
			ds.elements[i] = ds.elements[i - 1z];
		};

		ds.elements[insert_pos] = elem;
		ds.count += 1z;
		return;
	};
};

// Removes an element with the specified tag from the dataset.
export fn dataset_remove(ds: *dataset, t: tag::dicom_tag) bool = {
	match (dataset_find_index(ds, t)) {
	case let idx: size =>
		for (let i = idx; i < ds.count - 1z; i += 1z) {
			ds.elements[i] = ds.elements[i + 1z];
		};
		ds.count -= 1z;
		return true;
	case void =>
		return false;
	};
};

// Helper: extracts string value bytes for a given tag.
export fn dataset_get_string_bytes(ds: *dataset, t: tag::dicom_tag) ([]u8 | void) = {
	match (dataset_get(ds, t)) {
	case let elem: element::data_element =>
		return element::element_get_string_bytes(&elem);
	case void =>
		return;
	};
};

// Helper: extracts a u16 value for a given tag.
export fn dataset_get_u16(ds: *dataset, t: tag::dicom_tag, endian: types::endianness) (u16 | void) = {
	match (dataset_get(ds, t)) {
	case let elem: element::data_element =>
		match (element::element_get_u16(&elem, endian)) {
		case let val: u16 => return val;
		case => return;
		};
	case void =>
		return;
	};
};

// Helper: extracts a u32 value for a given tag.
export fn dataset_get_u32(ds: *dataset, t: tag::dicom_tag, endian: types::endianness) (u32 | void) = {
	match (dataset_get(ds, t)) {
	case let elem: element::data_element =>
		match (element::element_get_u32(&elem, endian)) {
		case let val: u32 => return val;
		case => return;
		};
	case void =>
		return;
	};
};

// Helper: extracts a Decimal String (DS) as an f64 value.
export fn dataset_get_ds_f64(ds: *dataset, t: tag::dicom_tag) (f64 | void) = {
	match (dataset_get(ds, t)) {
	case let elem: element::data_element =>
		match (element::element_parse_ds_first(&elem)) {
		case let val: f64 => return val;
		case => return;
		};
	case void =>
		return;
	};
};

// Helper: extracts an Integer String (IS) as an i32 value.
export fn dataset_get_is_i32(ds: *dataset, t: tag::dicom_tag) (i32 | void) = {
	match (dataset_get(ds, t)) {
	case let elem: element::data_element =>
		match (element::element_parse_is_first(&elem)) {
		case let val: i32 => return val;
		case => return;
		};
	case void =>
		return;
	};
};
