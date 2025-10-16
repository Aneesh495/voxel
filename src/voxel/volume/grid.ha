// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::grid
// 3D regular volumetric grid assembly, slice normal sorting, and voxel memory layout.

use voxel::types;
use voxel::volume::slice;

export type volume_grid = struct {
	dim_x: size,
	dim_y: size,
	dim_z: size,
	spacing_x: f64,
	spacing_y: f64,
	spacing_z: f64,
	origin: slice::vec3,
	row_dir: slice::vec3,
	col_dir: slice::vec3,
	slice_dir: slice::vec3,
	voxels: []i16,
};

// Initializes a 3D volumetric grid structure with preallocated flat voxel storage.
export fn grid_init(
	voxels: []i16,
	dim_x: size,
	dim_y: size,
	dim_z: size,
	spacing_x: f64,
	spacing_y: f64,
	spacing_z: f64,
	origin: slice::vec3,
	row_dir: slice::vec3,
	col_dir: slice::vec3,
) volume_grid = {
	const slice_dir = slice::vec3_cross(row_dir, col_dir);

	return volume_grid {
		dim_x = dim_x,
		dim_y = dim_y,
		dim_z = dim_z,
		spacing_x = spacing_x,
		spacing_y = spacing_y,
		spacing_z = spacing_z,
		origin = origin,
		row_dir = row_dir,
		col_dir = col_dir,
		slice_dir = slice_dir,
		voxels = voxels,
	};
};

// Computes flat linear memory index for 3D coordinates (x, y, z).
export fn grid_index(grid: *volume_grid, x: size, y: size, z: size) size = {
	return z * (grid.dim_x * grid.dim_y) + y * grid.dim_x + x;
};

// Retrieves a voxel value in Hounsfield Units, returning -1000 (Air) if out of bounds.
export fn grid_get(grid: *volume_grid, x: size, y: size, z: size) i16 = {
	if (x >= grid.dim_x || y >= grid.dim_y || z >= grid.dim_z) {
		return -1000i16;
	};
	const idx = grid_index(grid, x, y, z);
	if (idx >= len(grid.voxels)) {
		return -1000i16;
	};
	return grid.voxels[idx];
};

// Stores a voxel value into the contiguous volume buffer.
export fn grid_set(grid: *volume_grid, x: size, y: size, z: size, val: i16) void = {
	if (x >= grid.dim_x || y >= grid.dim_y || z >= grid.dim_z) {
		return;
	};
	const idx = grid_index(grid, x, y, z);
	if (idx < len(grid.voxels)) {
		grid.voxels[idx] = val;
	};
};

// Structure representing an unsorted slice candidate with its normal projection distance.
export type slice_sort_entry = struct {
	slice_index: size,
	projection_dist: f64,
};

// Sorts an array of slice entries along the slice normal axis using insertion sort.
export fn sort_slices_by_normal(entries: []slice_sort_entry) void = {
	const count = len(entries);
	for (let i = 1z; i < count; i += 1z) {
		const key = entries[i];
		let j = i;
		while (j > 0z && entries[j - 1z].projection_dist > key.projection_dist) {
			entries[j] = entries[j - 1z];
			j -= 1z;
		};
		entries[j] = key;
	};
};
