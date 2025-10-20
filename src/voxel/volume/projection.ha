// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::projection
// Ray-cast Maximum Intensity Projection (MIP), Minimum Intensity Projection (MinIP),
// and Average Intensity Projection (AIP) for vascular angiography and airway imaging.

use voxel::volume::grid;
use voxel::volume::lut;
use voxel::volume::mpr;

export type projection_mode = enum {
	MAXIMUM_INTENSITY, // MIP (vascular angiography, bone)
	MINIMUM_INTENSITY, // MinIP (airways, lung cavities)
	AVERAGE_INTENSITY, // AIP (simulated plain radiography)
};

// Computes a full-depth or slab-depth ray-cast projection along the chosen viewing axis.
export fn compute_projection(
	g: *grid::volume_grid,
	plane: mpr::mpr_plane,
	mode: projection_mode,
	start_depth: size,
	depth_count: size,
	wl: lut::window_level,
	out_pixels: []u8,
) void = {
	switch (plane) {
	case mpr::mpr_plane::AXIAL =>
		// Viewing along Z axis: width = dim_x, height = dim_y
		const w = g.dim_x;
		const h = g.dim_y;
		const max_z = if (start_depth + depth_count < g.dim_z) start_depth + depth_count else g.dim_z;

		for (let y = 0z; y < h; y += 1z) {
			for (let x = 0z; x < w; x += 1z) {
				let max_hu: f64 = -10000.0;
				let min_hu: f64 = 10000.0;
				let sum_hu: f64 = 0.0;
				let count: f64 = 0.0;

				for (let z = start_depth; z < max_z; z += 1z) {
					const val = grid::grid_get(g, x, y, z): f64;
					if (val > max_hu) max_hu = val;
					if (val < min_hu) min_hu = val;
					sum_hu += val;
					count += 1.0;
				};

				let final_hu = max_hu;
				switch (mode) {
				case projection_mode::MAXIMUM_INTENSITY => final_hu = max_hu;
				case projection_mode::MINIMUM_INTENSITY => final_hu = min_hu;
				case projection_mode::AVERAGE_INTENSITY =>
					final_hu = if (count > 0.0) sum_hu / count else -1000.0;
				};

				out_pixels[y * w + x] = lut::hu_to_display_linear(final_hu, wl);
			};
		};

	case mpr::mpr_plane::CORONAL =>
		// Viewing along Y axis: width = dim_x, height = dim_z
		const w = g.dim_x;
		const h = g.dim_z;
		const max_y = if (start_depth + depth_count < g.dim_y) start_depth + depth_count else g.dim_y;

		for (let r = 0z; r < h; r += 1z) {
			const z = h - 1z - r;
			for (let x = 0z; x < w; x += 1z) {
				let max_hu: f64 = -10000.0;
				let min_hu: f64 = 10000.0;
				let sum_hu: f64 = 0.0;
				let count: f64 = 0.0;

				for (let y = start_depth; y < max_y; y += 1z) {
					const val = grid::grid_get(g, x, y, z): f64;
					if (val > max_hu) max_hu = val;
					if (val < min_hu) min_hu = val;
					sum_hu += val;
					count += 1.0;
				};

				let final_hu = max_hu;
				switch (mode) {
				case projection_mode::MAXIMUM_INTENSITY => final_hu = max_hu;
				case projection_mode::MINIMUM_INTENSITY => final_hu = min_hu;
				case projection_mode::AVERAGE_INTENSITY =>
					final_hu = if (count > 0.0) sum_hu / count else -1000.0;
				};

				out_pixels[r * w + x] = lut::hu_to_display_linear(final_hu, wl);
			};
		};

	case mpr::mpr_plane::SAGITTAL =>
		// Viewing along X axis: width = dim_y, height = dim_z
		const w = g.dim_y;
		const h = g.dim_z;
		const max_x = if (start_depth + depth_count < g.dim_x) start_depth + depth_count else g.dim_x;

		for (let r = 0z; r < h; r += 1z) {
			const z = h - 1z - r;
			for (let y = 0z; y < w; y += 1z) {
				let max_hu: f64 = -10000.0;
				let min_hu: f64 = 10000.0;
				let sum_hu: f64 = 0.0;
				let count: f64 = 0.0;

				for (let x = start_depth; x < max_x; x += 1z) {
					const val = grid::grid_get(g, x, y, z): f64;
					if (val > max_hu) max_hu = val;
					if (val < min_hu) min_hu = val;
					sum_hu += val;
					count += 1.0;
				};

				let final_hu = max_hu;
				switch (mode) {
				case projection_mode::MAXIMUM_INTENSITY => final_hu = max_hu;
				case projection_mode::MINIMUM_INTENSITY => final_hu = min_hu;
				case projection_mode::AVERAGE_INTENSITY =>
					final_hu = if (count > 0.0) sum_hu / count else -1000.0;
				};

				out_pixels[r * w + y] = lut::hu_to_display_linear(final_hu, wl);
			};
		};
	};
};
