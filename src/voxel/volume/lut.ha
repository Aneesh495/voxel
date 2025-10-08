// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::lut
// Modality LUT (Rescale Slope/Intercept to HU) and VOI LUT (Window Center/Width)
// contrast transformations and clinical presets per NEMA PS3.3 Section C.11.

export type clinical_preset = enum {
	SOFT_TISSUE, // C: 40, W: 350
	BONE,        // C: 400, W: 2000
	LUNG,        // C: -600, W: 1500
	BRAIN,       // C: 40, W: 80
	LIVER,       // C: 30, W: 150
	MEDIASTINUM, // C: 50, W: 450
};

export type window_level = struct {
	center: f64,
	width: f64,
};

// Returns standard clinical window center and width parameters for a preset.
export fn get_clinical_preset(preset: clinical_preset) window_level = {
	switch (preset) {
	case clinical_preset::SOFT_TISSUE => return window_level { center = 40.0, width = 350.0 };
	case clinical_preset::BONE        => return window_level { center = 400.0, width = 2000.0 };
	case clinical_preset::LUNG        => return window_level { center = -600.0, width = 1500.0 };
	case clinical_preset::BRAIN       => return window_level { center = 40.0, width = 80.0 };
	case clinical_preset::LIVER       => return window_level { center = 30.0, width = 150.0 };
	case clinical_preset::MEDIASTINUM => return window_level { center = 50.0, width = 450.0 };
	};
};

// Applies Modality LUT: converts raw stored pixel value into calibrated Hounsfield Units (HU).
export fn raw_to_hu(raw_val: i32, rescale_slope: f64, rescale_intercept: f64) f64 = {
	return (raw_val: f64) * rescale_slope + rescale_intercept;
};

// Applies linear VOI LUT transformation, mapping Hounsfield Units into an 8-bit display value [0..255].
// Formula per DICOM PS3.3 Section C.11.2.1.2:
// - If hu <= center - 0.5 - (width - 1)/2: 0
// - Else if hu > center - 0.5 + (width - 1)/2: 255
// - Else: floor(((hu - (center - 0.5)) / (width - 1) + 0.5) * 255)
export fn hu_to_display_linear(hu: f64, wl: window_level) u8 = {
	if (wl.width <= 1.0) {
		return if (hu >= wl.center) 255u8 else 0u8;
	};

	const lower = wl.center - 0.5 - (wl.width - 1.0) / 2.0;
	const upper = wl.center - 0.5 + (wl.width - 1.0) / 2.0;

	if (hu <= lower) {
		return 0u8;
	};
	if (hu > upper) {
		return 255u8;
	};

	const normalized = (hu - (wl.center - 0.5)) / (wl.width - 1.0) + 0.5;
	const scaled = normalized * 255.0;
	return (scaled: u8);
};

// Fast precomputed 16-bit integer lookup table for instantaneous slice rendering.
export type fast_lut = struct {
	table: [65536]u8,
	min_raw: i16,
	max_raw: i16,
};

// Generates a 65536-entry fast lookup table caching both Modality and VOI LUT transforms.
export fn generate_fast_lut(
	lut: *fast_lut,
	rescale_slope: f64,
	rescale_intercept: f64,
	wl: window_level,
	is_signed: bool,
) void = {
	for (let i = 0; i < 65536; i += 1) {
		let raw_val: i32 = 0;
		if (is_signed) {
			raw_val = (i: u16): i16: i32;
		} else {
			raw_val = i;
		};
		const hu = raw_to_hu(raw_val, rescale_slope, rescale_intercept);
		lut.table[i] = hu_to_display_linear(hu, wl);
	};
};
