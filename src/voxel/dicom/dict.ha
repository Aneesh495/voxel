// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::dicom::dict
// Standard DICOM data dictionary containing tag-to-VR mappings, keywords,
// and value multiplicity descriptions per NEMA PS3.6.

use voxel::dicom::tag;
use voxel::dicom::vr;

export type dict_entry = struct {
	tag: tag::dicom_tag,
	vr: vr::dicom_vr,
	keyword: str,
	name: str,
	vm: str,
};

// Static lookup table for standard clinical DICOM elements.
// In Implicit VR Little Endian, the data stream omits the VR field entirely.
// The parser relies on this dictionary to decode binary types accurately.
const STANDARD_DICTIONARY: [_]dict_entry = [
	dict_entry { tag = tag::TAG_FILE_META_INFORMATION_GROUP_LENGTH, vr = vr::dicom_vr::UL, keyword = "FileMetaInformationGroupLength", name = "File Meta Information Group Length", vm = "1" },
	dict_entry { tag = tag::TAG_FILE_META_INFORMATION_VERSION, vr = vr::dicom_vr::OB, keyword = "FileMetaInformationVersion", name = "File Meta Information Version", vm = "1" },
	dict_entry { tag = tag::TAG_MEDIA_STORAGE_SOP_CLASS_UID, vr = vr::dicom_vr::UI, keyword = "MediaStorageSOPClassUID", name = "Media Storage SOP Class UID", vm = "1" },
	dict_entry { tag = tag::TAG_MEDIA_STORAGE_SOP_INSTANCE_UID, vr = vr::dicom_vr::UI, keyword = "MediaStorageSOPInstanceUID", name = "Media Storage SOP Instance UID", vm = "1" },
	dict_entry { tag = tag::TAG_TRANSFER_SYNTAX_UID, vr = vr::dicom_vr::UI, keyword = "TransferSyntaxUID", name = "Transfer Syntax UID", vm = "1" },
	dict_entry { tag = tag::TAG_IMPLEMENTATION_CLASS_UID, vr = vr::dicom_vr::UI, keyword = "ImplementationClassUID", name = "Implementation Class UID", vm = "1" },
	dict_entry { tag = tag::TAG_IMPLEMENTATION_VERSION_NAME, vr = vr::dicom_vr::SH, keyword = "ImplementationVersionName", name = "Implementation Version Name", vm = "1" },
	dict_entry { tag = tag::TAG_SOURCE_APPLICATION_ENTITY_TITLE, vr = vr::dicom_vr::AE, keyword = "SourceApplicationEntityTitle", name = "Source Application Entity Title", vm = "1" },

	dict_entry { tag = tag::TAG_SPECIFIC_CHARACTER_SET, vr = vr::dicom_vr::CS, keyword = "SpecificCharacterSet", name = "Specific Character Set", vm = "1-n" },
	dict_entry { tag = tag::TAG_IMAGE_TYPE, vr = vr::dicom_vr::CS, keyword = "ImageType", name = "Image Type", vm = "2-n" },
	dict_entry { tag = tag::TAG_SOP_CLASS_UID, vr = vr::dicom_vr::UI, keyword = "SOPClassUID", name = "SOP Class UID", vm = "1" },
	dict_entry { tag = tag::TAG_SOP_INSTANCE_UID, vr = vr::dicom_vr::UI, keyword = "SOPInstanceUID", name = "SOP Instance UID", vm = "1" },
	dict_entry { tag = tag::TAG_STUDY_DATE, vr = vr::dicom_vr::DA, keyword = "StudyDate", name = "Study Date", vm = "1" },
	dict_entry { tag = tag::TAG_SERIES_DATE, vr = vr::dicom_vr::DA, keyword = "SeriesDate", name = "Series Date", vm = "1" },
	dict_entry { tag = tag::TAG_ACQUISITION_DATE, vr = vr::dicom_vr::DA, keyword = "AcquisitionDate", name = "Acquisition Date", vm = "1" },
	dict_entry { tag = tag::TAG_CONTENT_DATE, vr = vr::dicom_vr::DA, keyword = "ContentDate", name = "Content Date", vm = "1" },
	dict_entry { tag = tag::TAG_STUDY_TIME, vr = vr::dicom_vr::TM, keyword = "StudyTime", name = "Study Time", vm = "1" },
	dict_entry { tag = tag::TAG_SERIES_TIME, vr = vr::dicom_vr::TM, keyword = "SeriesTime", name = "Series Time", vm = "1" },
	dict_entry { tag = tag::TAG_ACQUISITION_TIME, vr = vr::dicom_vr::TM, keyword = "AcquisitionTime", name = "Acquisition Time", vm = "1" },
	dict_entry { tag = tag::TAG_CONTENT_TIME, vr = vr::dicom_vr::TM, keyword = "ContentTime", name = "Content Time", vm = "1" },
	dict_entry { tag = tag::TAG_ACCESSION_NUMBER, vr = vr::dicom_vr::SH, keyword = "AccessionNumber", name = "Accession Number", vm = "1" },
	dict_entry { tag = tag::TAG_QUERY_RETRIEVE_LEVEL, vr = vr::dicom_vr::CS, keyword = "QueryRetrieveLevel", name = "Query/Retrieve Level", vm = "1" },
	dict_entry { tag = tag::TAG_MODALITY, vr = vr::dicom_vr::CS, keyword = "Modality", name = "Modality", vm = "1" },
	dict_entry { tag = tag::TAG_MANUFACTURER, vr = vr::dicom_vr::LO, keyword = "Manufacturer", name = "Manufacturer", vm = "1" },
	dict_entry { tag = tag::TAG_INSTITUTION_NAME, vr = vr::dicom_vr::LO, keyword = "InstitutionName", name = "Institution Name", vm = "1" },
	dict_entry { tag = tag::TAG_INSTITUTION_ADDRESS, vr = vr::dicom_vr::ST, keyword = "InstitutionAddress", name = "Institution Address", vm = "1" },
	dict_entry { tag = tag::TAG_REFERRING_PHYSICIAN_NAME, vr = vr::dicom_vr::PN, keyword = "ReferringPhysicianName", name = "Referring Physician's Name", vm = "1" },
	dict_entry { tag = tag::TAG_STUDY_DESCRIPTION, vr = vr::dicom_vr::LO, keyword = "StudyDescription", name = "Study Description", vm = "1" },
	dict_entry { tag = tag::TAG_SERIES_DESCRIPTION, vr = vr::dicom_vr::LO, keyword = "SeriesDescription", name = "Series Description", vm = "1" },
	dict_entry { tag = tag::TAG_OPERATORS_NAME, vr = vr::dicom_vr::PN, keyword = "OperatorsName", name = "Operators' Name", vm = "1-n" },
	dict_entry { tag = tag::TAG_MANUFACTURER_MODEL_NAME, vr = vr::dicom_vr::LO, keyword = "ManufacturerModelName", name = "Manufacturer's Model Name", vm = "1" },
	dict_entry { tag = tag::TAG_REFERENCED_STUDY_SEQUENCE, vr = vr::dicom_vr::SQ, keyword = "ReferencedStudySequence", name = "Referenced Study Sequence", vm = "1" },
	dict_entry { tag = tag::TAG_REFERENCED_SERIES_SEQUENCE, vr = vr::dicom_vr::SQ, keyword = "ReferencedSeriesSequence", name = "Referenced Series Sequence", vm = "1" },
	dict_entry { tag = tag::TAG_REFERENCED_SOP_SEQUENCE, vr = vr::dicom_vr::SQ, keyword = "ReferencedSOPSequence", name = "Referenced SOP Sequence", vm = "1" },

	dict_entry { tag = tag::TAG_PATIENT_NAME, vr = vr::dicom_vr::PN, keyword = "PatientName", name = "Patient's Name", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_ID, vr = vr::dicom_vr::LO, keyword = "PatientID", name = "Patient ID", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_BIRTH_DATE, vr = vr::dicom_vr::DA, keyword = "PatientBirthDate", name = "Patient's Birth Date", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_SEX, vr = vr::dicom_vr::CS, keyword = "PatientSex", name = "Patient's Sex", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_AGE, vr = vr::dicom_vr::AS, keyword = "PatientAge", name = "Patient's Age", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_SIZE, vr = vr::dicom_vr::DS, keyword = "PatientSize", name = "Patient's Size", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_WEIGHT, vr = vr::dicom_vr::DS, keyword = "PatientWeight", name = "Patient's Weight", vm = "1" },
	dict_entry { tag = tag::TAG_MEDICAL_ALERTS, vr = vr::dicom_vr::LO, keyword = "MedicalAlerts", name = "Medical Alerts", vm = "1" },
	dict_entry { tag = tag::TAG_ETHNIC_GROUP, vr = vr::dicom_vr::SH, keyword = "EthnicGroup", name = "Ethnic Group", vm = "1" },

	dict_entry { tag = tag::TAG_CONTRAST_BOLUS_AGENT, vr = vr::dicom_vr::LO, keyword = "ContrastBolusAgent", name = "Contrast/Bolus Agent", vm = "1" },
	dict_entry { tag = tag::TAG_BODY_PART_EXAMINED, vr = vr::dicom_vr::CS, keyword = "BodyPartExamined", name = "Body Part Examined", vm = "1" },
	dict_entry { tag = tag::TAG_SCANNING_SEQUENCE, vr = vr::dicom_vr::CS, keyword = "ScanningSequence", name = "Scanning Sequence", vm = "1-n" },
	dict_entry { tag = tag::TAG_SEQUENCE_VARIANT, vr = vr::dicom_vr::CS, keyword = "SequenceVariant", name = "Sequence Variant", vm = "1-n" },
	dict_entry { tag = tag::TAG_SLICE_THICKNESS, vr = vr::dicom_vr::DS, keyword = "SliceThickness", name = "Slice Thickness", vm = "1" },
	dict_entry { tag = tag::TAG_KVP, vr = vr::dicom_vr::DS, keyword = "KVP", name = "KVP", vm = "1" },
	dict_entry { tag = tag::TAG_REPETITION_TIME, vr = vr::dicom_vr::DS, keyword = "RepetitionTime", name = "Repetition Time", vm = "1" },
	dict_entry { tag = tag::TAG_ECHO_TIME, vr = vr::dicom_vr::DS, keyword = "EchoTime", name = "Echo Time", vm = "1" },
	dict_entry { tag = tag::TAG_INVERSION_TIME, vr = vr::dicom_vr::DS, keyword = "InversionTime", name = "Inversion Time", vm = "1" },
	dict_entry { tag = tag::TAG_SPACING_BETWEEN_SLICES, vr = vr::dicom_vr::DS, keyword = "SpacingBetweenSlices", name = "Spacing Between Slices", vm = "1" },
	dict_entry { tag = tag::TAG_DATA_COLLECTION_DIAMETER, vr = vr::dicom_vr::DS, keyword = "DataCollectionDiameter", name = "Data Collection Diameter", vm = "1" },
	dict_entry { tag = tag::TAG_DEVICE_SERIAL_NUMBER, vr = vr::dicom_vr::LO, keyword = "DeviceSerialNumber", name = "Device Serial Number", vm = "1" },
	dict_entry { tag = tag::TAG_GANTRY_DETECTOR_TILT, vr = vr::dicom_vr::DS, keyword = "GantryDetectorTilt", name = "Gantry/Detector Tilt", vm = "1" },
	dict_entry { tag = tag::TAG_EXPOSURE_TIME, vr = vr::dicom_vr::IS, keyword = "ExposureTime", name = "Exposure Time", vm = "1" },
	dict_entry { tag = tag::TAG_XRAY_TUBE_CURRENT, vr = vr::dicom_vr::IS, keyword = "XRayTubeCurrent", name = "X-Ray Tube Current", vm = "1" },
	dict_entry { tag = tag::TAG_EXPOSURE, vr = vr::dicom_vr::IS, keyword = "Exposure", name = "Exposure", vm = "1" },
	dict_entry { tag = tag::TAG_CONVOLUTION_KERNEL, vr = vr::dicom_vr::SH, keyword = "ConvolutionKernel", name = "Convolution Kernel", vm = "1-n" },
	dict_entry { tag = tag::TAG_PATIENT_POSITION, vr = vr::dicom_vr::CS, keyword = "PatientPosition", name = "Patient Position", vm = "1" },

	dict_entry { tag = tag::TAG_STUDY_INSTANCE_UID, vr = vr::dicom_vr::UI, keyword = "StudyInstanceUID", name = "Study Instance UID", vm = "1" },
	dict_entry { tag = tag::TAG_SERIES_INSTANCE_UID, vr = vr::dicom_vr::UI, keyword = "SeriesInstanceUID", name = "Series Instance UID", vm = "1" },
	dict_entry { tag = tag::TAG_STUDY_ID, vr = vr::dicom_vr::SH, keyword = "StudyID", name = "Study ID", vm = "1" },
	dict_entry { tag = tag::TAG_SERIES_NUMBER, vr = vr::dicom_vr::IS, keyword = "SeriesNumber", name = "Series Number", vm = "1" },
	dict_entry { tag = tag::TAG_ACQUISITION_NUMBER, vr = vr::dicom_vr::IS, keyword = "AcquisitionNumber", name = "Acquisition Number", vm = "1" },
	dict_entry { tag = tag::TAG_INSTANCE_NUMBER, vr = vr::dicom_vr::IS, keyword = "InstanceNumber", name = "Instance Number", vm = "1" },
	dict_entry { tag = tag::TAG_PATIENT_ORIENTATION, vr = vr::dicom_vr::CS, keyword = "PatientOrientation", name = "Patient Orientation", vm = "2" },
	dict_entry { tag = tag::TAG_IMAGE_POSITION_PATIENT, vr = vr::dicom_vr::DS, keyword = "ImagePositionPatient", name = "Image Position (Patient)", vm = "3" },
	dict_entry { tag = tag::TAG_IMAGE_ORIENTATION_PATIENT, vr = vr::dicom_vr::DS, keyword = "ImageOrientationPatient", name = "Image Orientation (Patient)", vm = "6" },
	dict_entry { tag = tag::TAG_FRAME_OF_REFERENCE_UID, vr = vr::dicom_vr::UI, keyword = "FrameOfReferenceUID", name = "Frame of Reference UID", vm = "1" },
	dict_entry { tag = tag::TAG_POSITION_REFERENCE_INDICATOR, vr = vr::dicom_vr::LO, keyword = "PositionReferenceIndicator", name = "Position Reference Indicator", vm = "1" },
	dict_entry { tag = tag::TAG_SLICE_LOCATION, vr = vr::dicom_vr::DS, keyword = "SliceLocation", name = "Slice Location", vm = "1" },

	dict_entry { tag = tag::TAG_SAMPLES_PER_PIXEL, vr = vr::dicom_vr::US, keyword = "SamplesPerPixel", name = "Samples per Pixel", vm = "1" },
	dict_entry { tag = tag::TAG_PHOTOMETRIC_INTERPRETATION, vr = vr::dicom_vr::CS, keyword = "PhotometricInterpretation", name = "Photometric Interpretation", vm = "1" },
	dict_entry { tag = tag::TAG_PLANAR_CONFIGURATION, vr = vr::dicom_vr::US, keyword = "PlanarConfiguration", name = "Planar Configuration", vm = "1" },
	dict_entry { tag = tag::TAG_NUMBER_OF_FRAMES, vr = vr::dicom_vr::IS, keyword = "NumberOfFrames", name = "Number of Frames", vm = "1" },
	dict_entry { tag = tag::TAG_ROWS, vr = vr::dicom_vr::US, keyword = "Rows", name = "Rows", vm = "1" },
	dict_entry { tag = tag::TAG_COLUMNS, vr = vr::dicom_vr::US, keyword = "Columns", name = "Columns", vm = "1" },
	dict_entry { tag = tag::TAG_PIXEL_SPACING, vr = vr::dicom_vr::DS, keyword = "PixelSpacing", name = "Pixel Spacing", vm = "2" },
	dict_entry { tag = tag::TAG_BITS_ALLOCATED, vr = vr::dicom_vr::US, keyword = "BitsAllocated", name = "Bits Allocated", vm = "1" },
	dict_entry { tag = tag::TAG_BITS_STORED, vr = vr::dicom_vr::US, keyword = "BitsStored", name = "Bits Stored", vm = "1" },
	dict_entry { tag = tag::TAG_HIGH_BIT, vr = vr::dicom_vr::US, keyword = "HighBit", name = "High Bit", vm = "1" },
	dict_entry { tag = tag::TAG_PIXEL_REPRESENTATION, vr = vr::dicom_vr::US, keyword = "PixelRepresentation", name = "Pixel Representation", vm = "1" },
	dict_entry { tag = tag::TAG_SMALLEST_IMAGE_PIXEL_VALUE, vr = vr::dicom_vr::US, keyword = "SmallestImagePixelValue", name = "Smallest Image Pixel Value", vm = "1" },
	dict_entry { tag = tag::TAG_LARGEST_IMAGE_PIXEL_VALUE, vr = vr::dicom_vr::US, keyword = "LargestImagePixelValue", name = "Largest Image Pixel Value", vm = "1" },
	dict_entry { tag = tag::TAG_WINDOW_CENTER, vr = vr::dicom_vr::DS, keyword = "WindowCenter", name = "Window Center", vm = "1-n" },
	dict_entry { tag = tag::TAG_WINDOW_WIDTH, vr = vr::dicom_vr::DS, keyword = "WindowWidth", name = "Window Width", vm = "1-n" },
	dict_entry { tag = tag::TAG_RESCALE_INTERCEPT, vr = vr::dicom_vr::DS, keyword = "RescaleIntercept", name = "Rescale Intercept", vm = "1" },
	dict_entry { tag = tag::TAG_RESCALE_SLOPE, vr = vr::dicom_vr::DS, keyword = "RescaleSlope", name = "Rescale Slope", vm = "1" },
	dict_entry { tag = tag::TAG_RESCALE_TYPE, vr = vr::dicom_vr::LO, keyword = "RescaleType", name = "Rescale Type", vm = "1" },
	dict_entry { tag = tag::TAG_WINDOW_CENTER_WIDTH_EXPLANATION, vr = vr::dicom_vr::LO, keyword = "WindowCenterWidthExplanation", name = "Window Center & Width Explanation", vm = "1-n" },

	dict_entry { tag = tag::TAG_PIXEL_DATA, vr = vr::dicom_vr::OW, keyword = "PixelData", name = "Pixel Data", vm = "1" },
	dict_entry { tag = tag::TAG_FLOAT_PIXEL_DATA, vr = vr::dicom_vr::OF, keyword = "FloatPixelData", name = "Float Pixel Data", vm = "1" },
	dict_entry { tag = tag::TAG_DOUBLE_FLOAT_PIXEL_DATA, vr = vr::dicom_vr::OD, keyword = "DoubleFloatPixelData", name = "Double Float Pixel Data", vm = "1" },

	dict_entry { tag = tag::TAG_ITEM, vr = vr::dicom_vr::UN, keyword = "Item", name = "Item", vm = "1" },
	dict_entry { tag = tag::TAG_ITEM_DELIMITATION_ITEM, vr = vr::dicom_vr::UN, keyword = "ItemDelimitationItem", name = "Item Delimitation Item", vm = "1" },
	dict_entry { tag = tag::TAG_SEQUENCE_DELIMITATION_ITEM, vr = vr::dicom_vr::UN, keyword = "SequenceDelimitationItem", name = "Sequence Delimitation Item", vm = "1" },
];

// Looks up a dictionary entry for a given tag.
export fn dict_lookup(t: tag::dicom_tag) (dict_entry | void) = {
	for (let i = 0z; i < len(STANDARD_DICTIONARY); i += 1z) {
		if (tag::tag_equal(STANDARD_DICTIONARY[i].tag, t)) {
			return STANDARD_DICTIONARY[i];
		};
	};
	return;
};

// Returns the expected Value Representation for a tag, defaulting to UN if unknown.
export fn dict_get_vr(t: tag::dicom_tag) vr::dicom_vr = {
	match (dict_lookup(t)) {
	case let entry: dict_entry =>
		return entry.vr;
	case void =>
		// Check standard rules for delimiters or unknown elements
		if (tag::tag_is_delimiter(t)) {
			return vr::dicom_vr::UN;
		};
		if (tag::tag_is_file_meta(t)) {
			return vr::dicom_vr::OB;
		};
		return vr::dicom_vr::UN;
	};
};

// Returns the standard keyword for a tag, or "UnknownTag" if not in the dictionary.
export fn dict_get_keyword(t: tag::dicom_tag) str = {
	match (dict_lookup(t)) {
	case let entry: dict_entry =>
		return entry.keyword;
	case void =>
		return "UnknownTag";
	};
};
