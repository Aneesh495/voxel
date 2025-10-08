// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::color
// Photometric interpretation modes, MONOCHROME1 inversion, and Palette Color CLUTs per PS3.3.

export type photometric_mode = enum {
	MONOCHROME1,
	MONOCHROME2,
	RGB,
	PALETTE_COLOR,
	YBR_FULL,
};

export type rgb_color = struct {
	r: u8,
	g: u8,
	b: u8,
};

// Parses a DICOM Photometric Interpretation string into an enum.
export fn parse_photometric_mode(str_bytes: []u8) photometric_mode = {
	if (len(str_bytes) >= 11z && str_bytes[0] == 'M' && str_bytes[10] == '1') {
		return photometric_mode::MONOCHROME1;
	};
	if (len(str_bytes) >= 11z && str_bytes[0] == 'M' && str_bytes[10] == '2') {
		return photometric_mode::MONOCHROME2;
	};
	if (len(str_bytes) >= 3z && str_bytes[0] == 'R' && str_bytes[1] == 'G' && str_bytes[2] == 'B') {
		return photometric_mode::RGB;
	};
	if (len(str_bytes) >= 7z && str_bytes[0] == 'P' && str_bytes[1] == 'A') {
		return photometric_mode::PALETTE_COLOR;
	};
	return photometric_mode::MONOCHROME2;
};

// Maps an 8-bit windowed grayscale pixel according to photometric interpretation.
export fn apply_photometric_grayscale(val: u8, mode: photometric_mode) u8 = {
	switch (mode) {
	case photometric_mode::MONOCHROME1 =>
		// Inverted grayscale: 0 is white, 255 is black
		return 255u8 - val;
	case =>
		return val;
	};
};

// Standard Hot Metal Pseudocolor Palette (often used in PET/CT and SPECT functional imaging).
export fn get_hot_metal_color(val: u8) rgb_color = {
	// 0..85: Black to Dark Red
	// 86..170: Red to Orange/Yellow
	// 171..255: Yellow to White
	if (val < 85u8) {
		const r = (val: u32 * 255u32 / 85u32): u8;
		return rgb_color { r = r, g = 0u8, b = 0u8 };
	} else if (val < 170u8) {
		const g = ((val - 85u8): u32 * 255u32 / 85u32): u8;
		return rgb_color { r = 255u8, g = g, b = 0u8 };
	} else {
		const b = ((val - 170u8): u32 * 255u32 / 85u32): u8;
		return rgb_color { r = 255u8, g = 255u8, b = b };
	};
};
