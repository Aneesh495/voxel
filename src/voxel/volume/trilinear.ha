// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::trilinear
// Trilinear voxel interpolation, sub-voxel spatial resampling, and boundary handling.

use voxel::volume::grid;

// Evaluates continuous trilinear interpolation at voxel coordinate (x, y, z).
// Returns the interpolated Hounsfield Unit value as an f64.
export fn sample_trilinear(
	g: *grid::volume_grid,
	x: f64,
	y: f64,
	z: f64,
) f64 = {
	// Boundary check: return -1000.0 HU (air) if completely outside volume
	if (x < 0.0 || x > (g.dim_x - 1z): f64 ||
	    y < 0.0 || y > (g.dim_y - 1z): f64 ||
	    z < 0.0 || z > (g.dim_z - 1z): f64) {
		return -1000.0;
	};

	const x0 = (x: size);
	const y0 = (y: size);
	const z0 = (z: size);

	const x1 = if (x0 + 1z < g.dim_x) x0 + 1z else x0;
	const y1 = if (y0 + 1z < g.dim_y) y0 + 1z else y0;
	const z1 = if (z0 + 1z < g.dim_z) z0 + 1z else z0;

	const u = x - (x0: f64);
	const v = y - (y0: f64);
	const w = z - (z0: f64);

	// Retrieve 8 corner voxel values in Hounsfield Units
	const c000 = grid::grid_get(g, x0, y0, z0): f64;
	const c100 = grid::grid_get(g, x1, y0, z0): f64;
	const c010 = grid::grid_get(g, x0, y1, z0): f64;
	const c110 = grid::grid_get(g, x1, y1, z0): f64;
	const c001 = grid::grid_get(g, x0, y0, z1): f64;
	const c101 = grid::grid_get(g, x1, y0, z1): f64;
	const c011 = grid::grid_get(g, x0, y1, z1): f64;
	const c111 = grid::grid_get(g, x1, y1, z1): f64;

	// Interpolate along X
	const c00 = c000 * (1.0 - u) + c100 * u;
	const c10 = c010 * (1.0 - u) + c110 * u;
	const c01 = c001 * (1.0 - u) + c101 * u;
	const c11 = c011 * (1.0 - u) + c111 * u;

	// Interpolate along Y
	const c0 = c00 * (1.0 - v) + c10 * v;
	const c1 = c01 * (1.0 - v) + c11 * v;

	// Interpolate along Z
	return c0 * (1.0 - w) + c1 * w;
};

// Nearest-neighbor sampling: fastest sampling mode for low-latency interactive scrubbing.
export fn sample_nearest(
	g: *grid::volume_grid,
	x: f64,
	y: f64,
	z: f64,
) f64 = {
	const rx = ((x + 0.5): size);
	const ry = ((y + 0.5): size);
	const rz = ((z + 0.5): size);

	if (rx >= g.dim_x || ry >= g.dim_y || rz >= g.dim_z) {
		return -1000.0;
	};

	return grid::grid_get(g, rx, ry, rz): f64;
};
