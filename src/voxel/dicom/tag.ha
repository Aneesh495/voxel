// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::tag
// Complete DICOM tag definition, packing, comparison, and standard dictionary tags.

export type dicom_tag = struct {
	group: u16,
	element: u16,
};

// Constructs a new DICOM tag from 16-bit group and element numbers.
export fn tag_new(group: u16, element: u16) dicom_tag = {
	return dicom_tag {
		group = group,
		element = element,
	};
};

// Packs a DICOM tag into a single 32-bit unsigned integer (group in high 16 bits).
export fn tag_to_u32(t: dicom_tag) u32 = {
	return ((t.group: u32) << 16u32) | (t.element: u32);
};

// Unpacks a 32-bit integer into a DICOM tag.
export fn tag_from_u32(val: u32) dicom_tag = {
	return dicom_tag {
		group = ((val >> 16u32) & 0xFFFFu32): u16,
		element = (val & 0xFFFFu32): u16,
	};
};

// Checks equality between two DICOM tags.
export fn tag_equal(a: dicom_tag, b: dicom_tag) bool = {
	return a.group == b.group && a.element == b.element;
};

// Returns true if tag 'a' precedes tag 'b' in canonical DICOM ordering.
// Per DICOM PS3.5, data elements must be strictly ordered by increasing tag numbers.
export fn tag_less(a: dicom_tag, b: dicom_tag) bool = {
	if (a.group != b.group) {
		return a.group < b.group;
	};
	return a.element < b.element;
};

// Returns true if the tag belongs to Group 0002 (File Meta Information).
export fn tag_is_file_meta(t: dicom_tag) bool = {
	return t.group == 0x0002u16;
};

// Returns true if the tag represents a private creator or private data element (odd group).
export fn tag_is_private(t: dicom_tag) bool = {
	return (t.group & 1u16) != 0u16 && t.group > 0x0008u16;
};

// Returns true if the tag is a delimiter item (Group FFFE).
export fn tag_is_delimiter(t: dicom_tag) bool = {
	return t.group == 0xFFFEu16;
};

// Group 0002: File Meta Information Tags
export def TAG_FILE_META_INFORMATION_GROUP_LENGTH: dicom_tag = dicom_tag { group = 0x0002, element = 0x0000 };
export def TAG_FILE_META_INFORMATION_VERSION: dicom_tag      = dicom_tag { group = 0x0002, element = 0x0001 };
export def TAG_MEDIA_STORAGE_SOP_CLASS_UID: dicom_tag        = dicom_tag { group = 0x0002, element = 0x0002 };
export def TAG_MEDIA_STORAGE_SOP_INSTANCE_UID: dicom_tag     = dicom_tag { group = 0x0002, element = 0x0003 };
export def TAG_TRANSFER_SYNTAX_UID: dicom_tag                = dicom_tag { group = 0x0002, element = 0x0010 };
export def TAG_IMPLEMENTATION_CLASS_UID: dicom_tag           = dicom_tag { group = 0x0002, element = 0x0012 };
export def TAG_IMPLEMENTATION_VERSION_NAME: dicom_tag        = dicom_tag { group = 0x0002, element = 0x0013 };
export def TAG_SOURCE_APPLICATION_ENTITY_TITLE: dicom_tag    = dicom_tag { group = 0x0002, element = 0x0016 };

// Group 0008: Identifying Information Tags
export def TAG_SPECIFIC_CHARACTER_SET: dicom_tag             = dicom_tag { group = 0x0008, element = 0x0005 };
export def TAG_IMAGE_TYPE: dicom_tag                         = dicom_tag { group = 0x0008, element = 0x0008 };
export def TAG_SOP_CLASS_UID: dicom_tag                      = dicom_tag { group = 0x0008, element = 0x0016 };
export def TAG_SOP_INSTANCE_UID: dicom_tag                   = dicom_tag { group = 0x0008, element = 0x0018 };
export def TAG_STUDY_DATE: dicom_tag                         = dicom_tag { group = 0x0008, element = 0x0020 };
export def TAG_SERIES_DATE: dicom_tag                        = dicom_tag { group = 0x0008, element = 0x0021 };
export def TAG_ACQUISITION_DATE: dicom_tag                   = dicom_tag { group = 0x0008, element = 0x0022 };
export def TAG_CONTENT_DATE: dicom_tag                       = dicom_tag { group = 0x0008, element = 0x0023 };
export def TAG_STUDY_TIME: dicom_tag                         = dicom_tag { group = 0x0008, element = 0x0030 };
export def TAG_SERIES_TIME: dicom_tag                        = dicom_tag { group = 0x0008, element = 0x0031 };
export def TAG_ACQUISITION_TIME: dicom_tag                   = dicom_tag { group = 0x0008, element = 0x0032 };
export def TAG_CONTENT_TIME: dicom_tag                       = dicom_tag { group = 0x0008, element = 0x0033 };
export def TAG_ACCESSION_NUMBER: dicom_tag                   = dicom_tag { group = 0x0008, element = 0x0050 };
export def TAG_QUERY_RETRIEVE_LEVEL: dicom_tag               = dicom_tag { group = 0x0008, element = 0x0052 };
export def TAG_MODALITY: dicom_tag                           = dicom_tag { group = 0x0008, element = 0x0060 };
export def TAG_MANUFACTURER: dicom_tag                       = dicom_tag { group = 0x0008, element = 0x0070 };
export def TAG_INSTITUTION_NAME: dicom_tag                   = dicom_tag { group = 0x0008, element = 0x0080 };
export def TAG_INSTITUTION_ADDRESS: dicom_tag                = dicom_tag { group = 0x0008, element = 0x0081 };
export def TAG_REFERRING_PHYSICIAN_NAME: dicom_tag           = dicom_tag { group = 0x0008, element = 0x0090 };
export def TAG_STUDY_DESCRIPTION: dicom_tag                  = dicom_tag { group = 0x0008, element = 0x1030 };
export def TAG_SERIES_DESCRIPTION: dicom_tag                 = dicom_tag { group = 0x0008, element = 0x103E };
export def TAG_OPERATORS_NAME: dicom_tag                     = dicom_tag { group = 0x0008, element = 0x1070 };
export def TAG_MANUFACTURER_MODEL_NAME: dicom_tag            = dicom_tag { group = 0x0008, element = 0x1090 };
export def TAG_REFERENCED_STUDY_SEQUENCE: dicom_tag          = dicom_tag { group = 0x0008, element = 0x1110 };
export def TAG_REFERENCED_SERIES_SEQUENCE: dicom_tag         = dicom_tag { group = 0x0008, element = 0x1115 };
export def TAG_REFERENCED_SOP_SEQUENCE: dicom_tag            = dicom_tag { group = 0x0008, element = 0x1199 };

// Group 0010: Patient Demographic Tags
export def TAG_PATIENT_NAME: dicom_tag                       = dicom_tag { group = 0x0010, element = 0x0010 };
export def TAG_PATIENT_ID: dicom_tag                         = dicom_tag { group = 0x0010, element = 0x0020 };
export def TAG_PATIENT_BIRTH_DATE: dicom_tag                 = dicom_tag { group = 0x0010, element = 0x0030 };
export def TAG_PATIENT_SEX: dicom_tag                        = dicom_tag { group = 0x0010, element = 0x0040 };
export def TAG_PATIENT_AGE: dicom_tag                        = dicom_tag { group = 0x0010, element = 0x1010 };
export def TAG_PATIENT_SIZE: dicom_tag                       = dicom_tag { group = 0x0010, element = 0x1020 };
export def TAG_PATIENT_WEIGHT: dicom_tag                     = dicom_tag { group = 0x0010, element = 0x1030 };
export def TAG_MEDICAL_ALERTS: dicom_tag                     = dicom_tag { group = 0x0010, element = 0x2000 };
export def TAG_ETHNIC_GROUP: dicom_tag                       = dicom_tag { group = 0x0010, element = 0x2160 };

// Group 0018: Acquisition Parameters
export def TAG_CONTRAST_BOLUS_AGENT: dicom_tag               = dicom_tag { group = 0x0018, element = 0x0010 };
export def TAG_BODY_PART_EXAMINED: dicom_tag                 = dicom_tag { group = 0x0018, element = 0x0015 };
export def TAG_SCANNING_SEQUENCE: dicom_tag                  = dicom_tag { group = 0x0018, element = 0x0020 };
export def TAG_SEQUENCE_VARIANT: dicom_tag                   = dicom_tag { group = 0x0018, element = 0x0021 };
export def TAG_SLICE_THICKNESS: dicom_tag                    = dicom_tag { group = 0x0018, element = 0x0050 };
export def TAG_KVP: dicom_tag                                = dicom_tag { group = 0x0018, element = 0x0060 };
export def TAG_REPETITION_TIME: dicom_tag                    = dicom_tag { group = 0x0018, element = 0x0080 };
export def TAG_ECHO_TIME: dicom_tag                          = dicom_tag { group = 0x0018, element = 0x0081 };
export def TAG_INVERSION_TIME: dicom_tag                     = dicom_tag { group = 0x0018, element = 0x0082 };
export def TAG_SPACING_BETWEEN_SLICES: dicom_tag             = dicom_tag { group = 0x0018, element = 0x0088 };
export def TAG_DATA_COLLECTION_DIAMETER: dicom_tag           = dicom_tag { group = 0x0018, element = 0x0090 };
export def TAG_DEVICE_SERIAL_NUMBER: dicom_tag               = dicom_tag { group = 0x0018, element = 0x1000 };
export def TAG_GANTRY_DETECTOR_TILT: dicom_tag               = dicom_tag { group = 0x0018, element = 0x1120 };
export def TAG_EXPOSURE_TIME: dicom_tag                      = dicom_tag { group = 0x0018, element = 0x1150 };
export def TAG_XRAY_TUBE_CURRENT: dicom_tag                  = dicom_tag { group = 0x0018, element = 0x1151 };
export def TAG_EXPOSURE: dicom_tag                           = dicom_tag { group = 0x0018, element = 0x1152 };
export def TAG_CONVOLUTION_KERNEL: dicom_tag                 = dicom_tag { group = 0x0018, element = 0x1210 };
export def TAG_PATIENT_POSITION: dicom_tag                   = dicom_tag { group = 0x0018, element = 0x5100 };

// Group 0020: Relationship and Spatial Coordinate Geometry
export def TAG_STUDY_INSTANCE_UID: dicom_tag                 = dicom_tag { group = 0x0020, element = 0x000D };
export def TAG_SERIES_INSTANCE_UID: dicom_tag                = dicom_tag { group = 0x0020, element = 0x000E };
export def TAG_STUDY_ID: dicom_tag                           = dicom_tag { group = 0x0020, element = 0x0010 };
export def TAG_SERIES_NUMBER: dicom_tag                      = dicom_tag { group = 0x0020, element = 0x0011 };
export def TAG_ACQUISITION_NUMBER: dicom_tag                 = dicom_tag { group = 0x0020, element = 0x0012 };
export def TAG_INSTANCE_NUMBER: dicom_tag                    = dicom_tag { group = 0x0020, element = 0x0013 };
export def TAG_PATIENT_ORIENTATION: dicom_tag                = dicom_tag { group = 0x0020, element = 0x0020 };
export def TAG_IMAGE_POSITION_PATIENT: dicom_tag             = dicom_tag { group = 0x0020, element = 0x0032 };
export def TAG_IMAGE_ORIENTATION_PATIENT: dicom_tag          = dicom_tag { group = 0x0020, element = 0x0037 };
export def TAG_FRAME_OF_REFERENCE_UID: dicom_tag             = dicom_tag { group = 0x0020, element = 0x0052 };
export def TAG_POSITION_REFERENCE_INDICATOR: dicom_tag       = dicom_tag { group = 0x0020, element = 0x1040 };
export def TAG_SLICE_LOCATION: dicom_tag                     = dicom_tag { group = 0x0020, element = 0x1041 };

// Group 0028: Image Pixel Presentation and Format
export def TAG_SAMPLES_PER_PIXEL: dicom_tag                  = dicom_tag { group = 0x0028, element = 0x0002 };
export def TAG_PHOTOMETRIC_INTERPRETATION: dicom_tag         = dicom_tag { group = 0x0028, element = 0x0004 };
export def TAG_PLANAR_CONFIGURATION: dicom_tag               = dicom_tag { group = 0x0028, element = 0x0006 };
export def TAG_NUMBER_OF_FRAMES: dicom_tag                   = dicom_tag { group = 0x0028, element = 0x0008 };
export def TAG_ROWS: dicom_tag                               = dicom_tag { group = 0x0028, element = 0x0010 };
export def TAG_COLUMNS: dicom_tag                            = dicom_tag { group = 0x0028, element = 0x0011 };
export def TAG_PIXEL_SPACING: dicom_tag                      = dicom_tag { group = 0x0028, element = 0x0030 };
export def TAG_BITS_ALLOCATED: dicom_tag                     = dicom_tag { group = 0x0028, element = 0x0100 };
export def TAG_BITS_STORED: dicom_tag                        = dicom_tag { group = 0x0028, element = 0x0101 };
export def TAG_HIGH_BIT: dicom_tag                           = dicom_tag { group = 0x0028, element = 0x0102 };
export def TAG_PIXEL_REPRESENTATION: dicom_tag               = dicom_tag { group = 0x0028, element = 0x0103 };
export def TAG_SMALLEST_IMAGE_PIXEL_VALUE: dicom_tag         = dicom_tag { group = 0x0028, element = 0x0106 };
export def TAG_LARGEST_IMAGE_PIXEL_VALUE: dicom_tag          = dicom_tag { group = 0x0028, element = 0x0107 };
export def TAG_WINDOW_CENTER: dicom_tag                      = dicom_tag { group = 0x0028, element = 0x1050 };
export def TAG_WINDOW_WIDTH: dicom_tag                       = dicom_tag { group = 0x0028, element = 0x1051 };
export def TAG_RESCALE_INTERCEPT: dicom_tag                  = dicom_tag { group = 0x0028, element = 0x1052 };
export def TAG_RESCALE_SLOPE: dicom_tag                      = dicom_tag { group = 0x0028, element = 0x1053 };
export def TAG_RESCALE_TYPE: dicom_tag                       = dicom_tag { group = 0x0028, element = 0x1054 };
export def TAG_WINDOW_CENTER_WIDTH_EXPLANATION: dicom_tag    = dicom_tag { group = 0x0028, element = 0x1055 };

// Group 7FE0: Pixel Data Stream
export def TAG_PIXEL_DATA: dicom_tag                         = dicom_tag { group = 0x7FE0, element = 0x0010 };
export def TAG_FLOAT_PIXEL_DATA: dicom_tag                   = dicom_tag { group = 0x7FE0, element = 0x0008 };
export def TAG_DOUBLE_FLOAT_PIXEL_DATA: dicom_tag            = dicom_tag { group = 0x7FE0, element = 0x0009 };

// Group FFFE: Sequence and Item Delimitation Tags
export def TAG_ITEM: dicom_tag                               = dicom_tag { group = 0xFFFE, element = 0xE000 };
export def TAG_ITEM_DELIMITATION_ITEM: dicom_tag             = dicom_tag { group = 0xFFFE, element = 0xE00D };
export def TAG_SEQUENCE_DELIMITATION_ITEM: dicom_tag         = dicom_tag { group = 0xFFFE, element = 0xE0DD };
