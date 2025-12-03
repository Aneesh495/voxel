#include "dicom.h"
#include <string.h>

static const dicom_dict_entry_t g_standard_dictionary[] = {
    { TAG_FILE_META_INFORMATION_GROUP_LENGTH, VR_UL, "FileMetaInformationGroupLength", "File Meta Information Group Length" },
    { TAG_FILE_META_INFORMATION_VERSION,      VR_OB, "FileMetaInformationVersion", "File Meta Information Version" },
    { TAG_MEDIA_STORAGE_SOP_CLASS_UID,        VR_UI, "MediaStorageSOPClassUID", "Media Storage SOP Class UID" },
    { TAG_MEDIA_STORAGE_SOP_INSTANCE_UID,     VR_UI, "MediaStorageSOPInstanceUID", "Media Storage SOP Instance UID" },
    { TAG_TRANSFER_SYNTAX_UID,                VR_UI, "TransferSyntaxUID", "Transfer Syntax UID" },
    { TAG_IMPLEMENTATION_CLASS_UID,           VR_UI, "ImplementationClassUID", "Implementation Class UID" },
    { TAG_IMPLEMENTATION_VERSION_NAME,        VR_SH, "ImplementationVersionName", "Implementation Version Name" },

    { TAG_SPECIFIC_CHARACTER_SET,             VR_CS, "SpecificCharacterSet", "Specific Character Set" },
    { TAG_IMAGE_TYPE,                         VR_CS, "ImageType", "Image Type" },
    { TAG_SOP_CLASS_UID,                      VR_UI, "SOPClassUID", "SOP Class UID" },
    { TAG_SOP_INSTANCE_UID,                   VR_UI, "SOPInstanceUID", "SOP Instance UID" },
    { TAG_STUDY_DATE,                         VR_DA, "StudyDate", "Study Date" },
    { TAG_SERIES_DATE,                        VR_DA, "SeriesDate", "Series Date" },
    { TAG_ACQUISITION_DATE,                   VR_DA, "AcquisitionDate", "Acquisition Date" },
    { TAG_CONTENT_DATE,                       VR_DA, "ContentDate", "Content Date" },
    { TAG_STUDY_TIME,                         VR_TM, "StudyTime", "Study Time" },
    { TAG_SERIES_TIME,                        VR_TM, "SeriesTime", "Series Time" },
    { TAG_ACCESSION_NUMBER,                   VR_SH, "AccessionNumber", "Accession Number" },
    { TAG_QUERY_RETRIEVE_LEVEL,               VR_CS, "QueryRetrieveLevel", "Query/Retrieve Level" },
    { TAG_MODALITY,                           VR_CS, "Modality", "Modality" },
    { TAG_MANUFACTURER,                       VR_LO, "Manufacturer", "Manufacturer" },
    { TAG_INSTITUTION_NAME,                   VR_LO, "InstitutionName", "Institution Name" },
    { TAG_REFERRING_PHYSICIAN_NAME,           VR_PN, "ReferringPhysicianName", "Referring Physician's Name" },
    { TAG_STUDY_DESCRIPTION,                  VR_LO, "StudyDescription", "Study Description" },
    { TAG_SERIES_DESCRIPTION,                 VR_LO, "SeriesDescription", "Series Description" },
    { TAG_MANUFACTURER_MODEL_NAME,            VR_LO, "ManufacturerModelName", "Manufacturer's Model Name" },

    { TAG_PATIENT_NAME,                       VR_PN, "PatientName", "Patient's Name" },
    { TAG_PATIENT_ID,                         VR_LO, "PatientID", "Patient ID" },
    { TAG_PATIENT_BIRTH_DATE,                 VR_DA, "PatientBirthDate", "Patient's Birth Date" },
    { TAG_PATIENT_SEX,                        VR_CS, "PatientSex", "Patient's Sex" },
    { TAG_PATIENT_AGE,                        VR_AS, "PatientAge", "Patient's Age" },

    { TAG_BODY_PART_EXAMINED,                 VR_CS, "BodyPartExamined", "Body Part Examined" },
    { TAG_SLICE_THICKNESS,                    VR_DS, "SliceThickness", "Slice Thickness" },
    { TAG_KVP,                                VR_DS, "KVP", "KVP" },
    { TAG_SPACING_BETWEEN_SLICES,             VR_DS, "SpacingBetweenSlices", "Spacing Between Slices" },

    { TAG_STUDY_INSTANCE_UID,                 VR_UI, "StudyInstanceUID", "Study Instance UID" },
    { TAG_SERIES_INSTANCE_UID,                VR_UI, "SeriesInstanceUID", "Series Instance UID" },
    { TAG_STUDY_ID,                           VR_SH, "StudyID", "Study ID" },
    { TAG_SERIES_NUMBER,                      VR_IS, "SeriesNumber", "Series Number" },
    { TAG_INSTANCE_NUMBER,                    VR_IS, "InstanceNumber", "Instance Number" },
    { TAG_IMAGE_POSITION_PATIENT,             VR_DS, "ImagePositionPatient", "Image Position (Patient)" },
    { TAG_IMAGE_ORIENTATION_PATIENT,          VR_DS, "ImageOrientationPatient", "Image Orientation (Patient)" },
    { TAG_FRAME_OF_REFERENCE_UID,             VR_UI, "FrameOfReferenceUID", "Frame of Reference UID" },
    { TAG_SLICE_LOCATION,                     VR_DS, "SliceLocation", "Slice Location" },

    { TAG_SAMPLES_PER_PIXEL,                  VR_US, "SamplesPerPixel", "Samples per Pixel" },
    { TAG_PHOTOMETRIC_INTERPRETATION,         VR_CS, "PhotometricInterpretation", "Photometric Interpretation" },
    { TAG_ROWS,                               VR_US, "Rows", "Rows" },
    { TAG_COLUMNS,                            VR_US, "Columns", "Columns" },
    { TAG_PIXEL_SPACING,                      VR_DS, "PixelSpacing", "Pixel Spacing" },
    { TAG_BITS_ALLOCATED,                     VR_US, "BitsAllocated", "Bits Allocated" },
    { TAG_BITS_STORED,                        VR_US, "BitsStored", "Bits Stored" },
    { TAG_HIGH_BIT,                           VR_US, "HighBit", "High Bit" },
    { TAG_PIXEL_REPRESENTATION,               VR_US, "PixelRepresentation", "Pixel Representation" },
    { TAG_WINDOW_CENTER,                      VR_DS, "WindowCenter", "Window Center" },
    { TAG_WINDOW_WIDTH,                       VR_DS, "WindowWidth", "Window Width" },
    { TAG_RESCALE_INTERCEPT,                  VR_DS, "RescaleIntercept", "Rescale Intercept" },
    { TAG_RESCALE_SLOPE,                      VR_DS, "RescaleSlope", "Rescale Slope" },

    { TAG_PIXEL_DATA,                         VR_OW, "PixelData", "Pixel Data" },
    { TAG_ITEM,                               VR_UN, "Item", "Item" },
    { TAG_ITEM_DELIMITATION_ITEM,             VR_UN, "ItemDelimitationItem", "Item Delimitation Item" },
    { TAG_SEQUENCE_DELIMITATION_ITEM,         VR_UN, "SequenceDelimitationItem", "Sequence Delimitation Item" }
};

static const size_t g_dict_size = sizeof(g_standard_dictionary) / sizeof(g_standard_dictionary[0]);

const dicom_dict_entry_t *dicom_dict_lookup(dicom_tag_t tag) {
    for (size_t i = 0; i < g_dict_size; i++) {
        if (TAG_EQUAL(g_standard_dictionary[i].tag, tag)) {
            return &g_standard_dictionary[i];
        }
    }
    return NULL;
}

dicom_vr_t dicom_dict_get_vr(dicom_tag_t tag) {
    const dicom_dict_entry_t *entry = dicom_dict_lookup(tag);
    if (entry) {
        return entry->vr;
    }
    if (tag.group == 0x0002) {
        return VR_OB;
    }
    if (tag.group == 0xFFFE) {
        return VR_UN;
    }
    return VR_UN;
}

const char *dicom_dict_get_keyword(dicom_tag_t tag) {
    const dicom_dict_entry_t *entry = dicom_dict_lookup(tag);
    return entry ? entry->keyword : "UnknownTag";
}
