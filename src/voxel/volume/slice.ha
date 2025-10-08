// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::volume::slice
// 2D slice geometric modeling, Patient Reference Coordinate System (RCS),
// direction cosine orthogonal basis calculation, and spatial coordinate transforms per PS3.3.

use voxel::types;

export type vec3 = struct {
	x: f64,
	y: f64,
	z: f64,
};

// Computes dot product of two 3D vectors.
export fn vec3_dot(a: vec3, b: vec3) f64 = {
	return a.x * b.x + a.y * b.y + a.z * b.z;
};

// Computes cross product of two 3D vectors: c = a x b.
export fn vec3_cross(a: vec3, b: vec3) vec3 = {
	return vec3 {
		x = a.y * b.z - a.z * b.y,
		y = a.z * b.x - a.x * b.z,
		z = a.x * b.y - a.y * b.x,
	};
};

// Computes Euclidean length (norm) of a 3D vector.
export fn vec3_norm(v: vec3) f64 = {
	const sq = v.x * v.x + v.y * v.y + v.z * v.z;
	// Fast iterative square root (Newton-Raphson)
	if (sq <= 0.0) return 0.0;
	let guess = sq / 2.0;
	if (guess == 0.0) guess = 1.0;
	for (let i = 0; i < 12; i += 1) {
		guess = 0.5 * (guess + sq / guess);
	};
	return guess;
};

// Normalizes vector to unit length.
export fn vec3_normalize(v: vec3) vec3 = {
	const n = vec3_norm(v);
	if (n <= 1e-9) {
		return vec3 { x = 0.0, y = 0.0, z = 0.0 };
	};
	return vec3 {
		x = v.x / n,
		y = v.y / n,
		z = v.z / n,
	};
};

export type slice_geometry = struct {
	rows: u16,
	cols: u16,
	// Position of center of first transmitted voxel (upper left hand corner) in mm
	origin: vec3,
	// Direction cosines of the first row (vector along columns)
	row_cosines: vec3,
	// Direction cosines of the first column (vector along rows)
	col_cosines: vec3,
	// Normal vector to the slice plane: N = row x col
	normal: vec3,
	// Physical distance between center of adjacent pixels: [row_spacing, col_spacing] in mm
	row_spacing: f64,
	col_spacing: f64,
	slice_thickness: f64,
	// Scalar projection along normal axis: d = origin . normal
	projection_dist: f64,
};

// Constructs and normalizes a slice geometry from raw DICOM attributes.
export fn slice_geometry_init(
	rows: u16,
	cols: u16,
	ipp: [3]f64,
	iop: [6]f64,
	pixel_spacing: [2]f64,
	slice_thickness: f64,
) slice_geometry = {
	const origin = vec3 { x = ipp[0], y = ipp[1], z = ipp[2] };
	const row_dir = vec3_normalize(vec3 { x = iop[0], y = iop[1], z = iop[2] });
	const col_dir = vec3_normalize(vec3 { x = iop[3], y = iop[4], z = iop[5] });
	const normal = vec3_normalize(vec3_cross(row_dir, col_dir));
	const dist = vec3_dot(origin, normal);

	return slice_geometry {
		rows = rows,
		cols = cols,
		origin = origin,
		row_cosines = row_dir,
		col_cosines = col_dir,
		normal = normal,
		row_spacing = pixel_spacing[0],
		col_spacing = pixel_spacing[1],
		slice_thickness = slice_thickness,
		projection_dist = dist,
	};
};

// Converts 2D pixel coordinates (row, col) into 3D Patient Reference Coordinate System (mm).
// P(r, c) = Origin + r * row_spacing * col_cosines + c * col_spacing * row_cosines
export fn pixel_to_patient(
	geom: *slice_geometry,
	row: f64,
	col: f64,
) vec3 = {
	const r_offset = row * geom.row_spacing;
	const c_offset = col * geom.col_spacing;

	return vec3 {
		x = geom.origin.x + r_offset * geom.col_cosines.x + c_offset * geom.row_cosines.x,
		y = geom.origin.y + r_offset * geom.col_cosines.y + c_offset * geom.row_cosines.y,
		z = geom.origin.z + r_offset * geom.col_cosines.z + c_offset * geom.row_cosines.z,
	};
};
