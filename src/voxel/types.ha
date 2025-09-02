// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::types
// Core primitive types, byte-order conversions, dynamic memory buffers,
// and cryptographic UID generation.

export type endianness = enum {
	LITTLE,
	BIG,
};

export type error_code = enum {
	OK = 0,
	INVALID_ARGUMENT = 1,
	BUFFER_OVERFLOW = 2,
	OUT_OF_MEMORY = 3,
	IO_ERROR = 4,
	CORRUPT_DATA = 5,
	NOT_FOUND = 6,
	UNSUPPORTED_SYNTAX = 7,
	NETWORK_ERROR = 8,
	PARSE_ERROR = 9,
};

// Represents a contiguous slice of bytes with explicit capacity and length.
export type byte_buffer = struct {
	data: []u8,
	cursor: size,
};

// Initializes a byte buffer with a preallocated backing slice.
export fn buffer_init(backing: []u8) byte_buffer = {
	return byte_buffer {
		data = backing,
		cursor = 0z,
	};
};

// Resets buffer read/write position to the origin.
export fn buffer_reset(buf: *byte_buffer) void = {
	buf.cursor = 0z;
};

// Returns remaining readable or writable bytes in the buffer.
export fn buffer_remaining(buf: *byte_buffer) size = {
	if (buf.cursor >= len(buf.data)) {
		return 0z;
	};
	return len(buf.data) - buf.cursor;
};

// Reads a single unsigned byte from the buffer.
export fn buffer_read_u8(buf: *byte_buffer) (u8 | error_code) = {
	if (buf.cursor >= len(buf.data)) {
		return error_code::BUFFER_OVERFLOW;
	};
	const val = buf.data[buf.cursor];
	buf.cursor += 1z;
	return val;
};

// Writes a single unsigned byte to the buffer.
export fn buffer_write_u8(buf: *byte_buffer, val: u8) (void | error_code) = {
	if (buf.cursor >= len(buf.data)) {
		return error_code::BUFFER_OVERFLOW;
	};
	buf.data[buf.cursor] = val;
	buf.cursor += 1z;
};

// Reads a 16-bit unsigned integer with specified endianness.
export fn buffer_read_u16(buf: *byte_buffer, endian: endianness) (u16 | error_code) = {
	if (buffer_remaining(buf) < 2z) {
		return error_code::BUFFER_OVERFLOW;
	};
	const b0 = buf.data[buf.cursor]: u16;
	const b1 = buf.data[buf.cursor + 1z]: u16;
	buf.cursor += 2z;
	match (endian) {
	case endianness::LITTLE =>
		return b0 | (b1 << 8u16);
	case endianness::BIG =>
		return (b0 << 8u16) | b1;
	};
};

// Writes a 16-bit unsigned integer with specified endianness.
export fn buffer_write_u16(buf: *byte_buffer, val: u16, endian: endianness) (void | error_code) = {
	if (buffer_remaining(buf) < 2z) {
		return error_code::BUFFER_OVERFLOW;
	};
	match (endian) {
	case endianness::LITTLE =>
		buf.data[buf.cursor] = (val & 0xFFu16): u8;
		buf.data[buf.cursor + 1z] = ((val >> 8u16) & 0xFFu16): u8;
	case endianness::BIG =>
		buf.data[buf.cursor] = ((val >> 8u16) & 0xFFu16): u8;
		buf.data[buf.cursor + 1z] = (val & 0xFFu16): u8;
	};
	buf.cursor += 2z;
};

// Reads a 32-bit unsigned integer with specified endianness.
export fn buffer_read_u32(buf: *byte_buffer, endian: endianness) (u32 | error_code) = {
	if (buffer_remaining(buf) < 4z) {
		return error_code::BUFFER_OVERFLOW;
	};
	const b0 = buf.data[buf.cursor]: u32;
	const b1 = buf.data[buf.cursor + 1z]: u32;
	const b2 = buf.data[buf.cursor + 2z]: u32;
	const b3 = buf.data[buf.cursor + 3z]: u32;
	buf.cursor += 4z;
	match (endian) {
	case endianness::LITTLE =>
		return b0 | (b1 << 8u32) | (b2 << 16u32) | (b3 << 24u32);
	case endianness::BIG =>
		return (b0 << 24u32) | (b1 << 16u32) | (b2 << 8u32) | b3;
	};
};

// Writes a 32-bit unsigned integer with specified endianness.
export fn buffer_write_u32(buf: *byte_buffer, val: u32, endian: endianness) (void | error_code) = {
	if (buffer_remaining(buf) < 4z) {
		return error_code::BUFFER_OVERFLOW;
	};
	match (endian) {
	case endianness::LITTLE =>
		buf.data[buf.cursor] = (val & 0xFFu32): u8;
		buf.data[buf.cursor + 1z] = ((val >> 8u32) & 0xFFu32): u8;
		buf.data[buf.cursor + 2z] = ((val >> 16u32) & 0xFFu32): u8;
		buf.data[buf.cursor + 3z] = ((val >> 24u32) & 0xFFu32): u8;
	case endianness::BIG =>
		buf.data[buf.cursor] = ((val >> 24u32) & 0xFFu32): u8;
		buf.data[buf.cursor + 1z] = ((val >> 16u32) & 0xFFu32): u8;
		buf.data[buf.cursor + 2z] = ((val >> 8u32) & 0xFFu32): u8;
		buf.data[buf.cursor + 3z] = (val & 0xFFu32): u8;
	};
	buf.cursor += 4z;
};

// Reads a 64-bit unsigned integer with specified endianness.
export fn buffer_read_u64(buf: *byte_buffer, endian: endianness) (u64 | error_code) = {
	if (buffer_remaining(buf) < 8z) {
		return error_code::BUFFER_OVERFLOW;
	};
	const b0 = buf.data[buf.cursor]: u64;
	const b1 = buf.data[buf.cursor + 1z]: u64;
	const b2 = buf.data[buf.cursor + 2z]: u64;
	const b3 = buf.data[buf.cursor + 3z]: u64;
	const b4 = buf.data[buf.cursor + 4z]: u64;
	const b5 = buf.data[buf.cursor + 5z]: u64;
	const b6 = buf.data[buf.cursor + 6z]: u64;
	const b7 = buf.data[buf.cursor + 7z]: u64;
	buf.cursor += 8z;
	match (endian) {
	case endianness::LITTLE =>
		return b0 | (b1 << 8u64) | (b2 << 16u64) | (b3 << 24u64) |
			(b4 << 32u64) | (b5 << 40u64) | (b6 << 48u64) | (b7 << 56u64);
	case endianness::BIG =>
		return (b0 << 56u64) | (b1 << 48u64) | (b2 << 40u64) | (b3 << 32u64) |
			(b4 << 24u64) | (b5 << 16u64) | (b6 << 8u64) | b7;
	};
};

// Reads a 32-bit IEEE 754 floating point number.
export fn buffer_read_f32(buf: *byte_buffer, endian: endianness) (f32 | error_code) = {
	const bits = buffer_read_u32(buf, endian)?;
	// Binary reinterpretation of u32 to f32
	const ptr = &bits: *f32;
	return *ptr;
};

// Reads a 64-bit IEEE 754 floating point number.
export fn buffer_read_f64(buf: *byte_buffer, endian: endianness) (f64 | error_code) = {
	const bits = buffer_read_u64(buf, endian)?;
	// Binary reinterpretation of u64 to f64
	const ptr = &bits: *f64;
	return *ptr;
};

// Copies raw bytes from the buffer into a target destination slice.
export fn buffer_read_bytes(buf: *byte_buffer, dest: []u8) (size | error_code) = {
	const count = len(dest);
	if (buffer_remaining(buf) < count) {
		return error_code::BUFFER_OVERFLOW;
	};
	dest[..count] = buf.data[buf.cursor..buf.cursor + count];
	buf.cursor += count;
	return count;
};

// Writes raw bytes into the buffer from a source slice.
export fn buffer_write_bytes(buf: *byte_buffer, src: []u8) (size | error_code) = {
	const count = len(src);
	if (buffer_remaining(buf) < count) {
		return error_code::BUFFER_OVERFLOW;
	};
	buf.data[buf.cursor..buf.cursor + count] = src[..count];
	buf.cursor += count;
	return count;
};

// Fast 32-bit MurmurHash3 calculation for tag and UID indexing.
export fn hash_murmur3(data: []u8, seed: u32) u32 = {
	const c1: u32 = 0xcc9e2d51u32;
	const c2: u32 = 0x1b873593u32;
	let h1: u32 = seed;
	const nblocks: size = len(data) / 4z;

	for (let i = 0z; i < nblocks; i += 1z) {
		const idx = i * 4z;
		let k1: u32 = (data[idx]: u32) |
			((data[idx + 1z]: u32) << 8u32) |
			((data[idx + 2z]: u32) << 16u32) |
			((data[idx + 3z]: u32) << 24u32);

		k1 *= c1;
		k1 = (k1 << 15u32) | (k1 >> 17u32);
		k1 *= c2;

		h1 ^= k1;
		h1 = (h1 << 13u32) | (h1 >> 19u32);
		h1 = h1 * 5u32 + 0xe6546b64u32;
	};

	const tail_idx = nblocks * 4z;
	let k2: u32 = 0u32;
	const remaining = len(data) & 3z;

	if (remaining >= 3z) {
		k2 ^= (data[tail_idx + 2z]: u32) << 16u32;
	};
	if (remaining >= 2z) {
		k2 ^= (data[tail_idx + 1z]: u32) << 8u32;
	};
	if (remaining >= 1z) {
		k2 ^= data[tail_idx]: u32;
		k2 *= c1;
		k2 = (k2 << 15u32) | (k2 >> 17u32);
		k2 *= c2;
		h1 ^= k2;
	};

	h1 ^= len(data): u32;
	h1 ^= h1 >> 16u32;
	h1 *= 0x85ebca6bu32;
	h1 ^= h1 >> 13u32;
	h1 *= 0xc2b2ae35u32;
	h1 ^= h1 >> 16u32;

	return h1;
};

// IEEE 802.3 32-bit Cyclic Redundancy Check (CRC32) calculation.
export fn crc32_calculate(data: []u8) u32 = {
	let crc: u32 = 0xFFFFFFFFu32;
	for (let i = 0z; i < len(data); i += 1z) {
		let byte_val = (crc ^ (data[i]: u32)) & 0xFFu32;
		for (let j = 0u32; j < 8u32; j += 1u32) {
			if ((byte_val & 1u32) != 0u32) {
				byte_val = (byte_val >> 1u32) ^ 0xEDB88320u32;
			} else {
				byte_val = byte_val >> 1u32;
			};
		};
		crc = (crc >> 8u32) ^ byte_val;
	};
	return ~crc;
};
