// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::mpr
// Multi-Planar Reconstruction (MPR) orthogonal (Axial, Coronal, Sagittal)
// and oblique slicing with physical aspect ratio correction.

use voxel::volume::grid;
use voxel::volume::lut;
use voxel::volume::trilinear;

export type mpr_plane = enum {
	AXIAL,    // XY transverse plane (Z fixed)
	CORONAL,  // XZ frontal plane (Y fixed)
	SAGITTAL, // YZ lateral plane (X fixed)
};

export type mpr_view = struct {
	plane: mpr_plane,
	slice_index: size,
	width: size,
	height: size,
	pixel_spacing_h: f64,
	pixel_spacing_v: f64,
};

// Initializes view parameters and dimensions for a chosen MPR plane.
export fn mpr_view_init(g: *grid::volume_grid, plane: mpr_plane, slice_idx: size) mpr_view = {
	switch (plane) {
	case mpr_plane::AXIAL =>
		const z = if (slice_idx < g.dim_z) slice_idx else g.dim_z / 2z;
		return mpr_view {
			plane = plane,
			slice_index = z,
			width = g.dim_x,
			height = g.dim_y,
			pixel_spacing_h = g.spacing_x,
			pixel_spacing_v = g.spacing_y,
		};
	case mpr_plane::CORONAL =>
		const y = if (slice_idx < g.dim_y) slice_idx else g.dim_y / 2z;
		return mpr_view {
			plane = plane,
			slice_index = y,
			width = g.dim_x,
			height = g.dim_z,
			pixel_spacing_h = g.spacing_x,
			pixel_spacing_v = g.spacing_z,
		};
	case mpr_plane::SAGITTAL =>
		const x = if (slice_idx < g.dim_x) slice_idx else g.dim_x / 2z;
		return mpr_view {
			plane = plane,
			slice_index = x,
			width = g.dim_y,
			height = g.dim_z,
			pixel_spacing_h = g.spacing_y,
			pixel_spacing_v = g.spacing_z,
		};
	};
};

// Renders an 8-bit windowed 2D slice image for the selected MPR plane.
export fn render_mpr_slice(
	g: *grid::volume_grid,
	view: *mpr_view,
	wl: lut::window_level,
	out_pixels: []u8,
) void = {
	const w = view.width;
	const h = view.height;

	switch (view.plane) {
	case mpr_plane::AXIAL =>
		const z = view.slice_index;
		for (let r = 0z; r < h; r += 1z) {
			for (let c = 0z; c < w; c += 1z) {
				const hu = grid::grid_get(g, c, r, z): f64;
				const disp = lut::hu_to_display_linear(hu, wl);
				out_pixels[r * w + c] = disp;
			};
		};

	case mpr_plane::CORONAL =>
		const y = view.slice_index;
		for (let r = 0z; r < h; r += 1z) {
			for (let c = 0z; c < w; c += 1z) {
				// Coronal: c is X, r is Z (inverted for anatomical cranial orientation)
				const z = h - 1z - r;
				const hu = grid::grid_get(g, c, y, z): f64;
				const disp = lut::hu_to_display_linear(hu, wl);
				out_pixels[r * w + c] = disp;
			};
		};

	case mpr_plane::SAGITTAL =>
		const x = view.slice_index;
		for (let r = 0z; r < h; r += 1z) {
			for (let c = 0z; c < w; c += 1z) {
				// Sagittal: c is Y, r is Z
				const z = h - 1z - r;
				const hu = grid::grid_get(g, x, c, z): f64;
				const disp = lut::hu_to_display_linear(hu, wl);
				out_pixels[r * w + c] = disp;
			};
		};
	};
};
