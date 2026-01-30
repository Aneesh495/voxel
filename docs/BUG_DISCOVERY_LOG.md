# Voxel Bug Discovery & Test-Driven Resolution Log

This document chronicles the edge-case bugs discovered during the development, benchmarking, and cross-platform verification of Voxel, along with the reproduction test cases implemented in `tests/test_bug_reproductions.c` to prevent future regressions.

---

## Bug 1: Transfer Syntax Prefix Overmatch in Part 10 Header Parser

### Severity
Critical (Protocol parser malfunction)

### Symptom
When parsing standard CT and MR datasets encoded with Explicit VR Little Endian (`1.2.840.10008.1.2.1`), the parser incorrectly classified the dataset as Implicit VR Little Endian, resulting in corrupted element offsets and parser crashes on sequence tags.

### Root Cause Analysis
The parser checked transfer syntaxes using `strncmp` with a length of 17 characters:
```c
if (strncmp(ts_str, "1.2.840.10008.1.2", 17) == 0) {
    is_explicit = false; // BUG: "1.2.840.10008.1.2.1" starts with the 17-char prefix!
}
```
Because the string `"1.2.840.10008.1.2.1"` (Explicit VR Little Endian, 19 chars) begins with the exact 17-character prefix of `"1.2.840.10008.1.2"` (Implicit VR Little Endian), `strncmp` returned 0, falsely setting `is_explicit = false`.

### Test-Driven Reproduction
Implemented `test_bug1_transfer_syntax_prefix()` in `tests/test_bug_reproductions.c`. The test serializes a dataset with `1.2.840.10008.1.2.1`, passes the byte stream through the reader, and asserts that `is_explicit` remains true and element lengths match explicit VR layout.

### Resolution
Replaced prefix matching with exact null-terminated `strcmp(ts_str, UID_IMPLICIT_VR_LITTLE_ENDIAN) == 0`.

---

## Bug 2: Two's Complement Signed Pixel Rescaling Underflow

### Severity
High (Visual diagnostic artifact)

### Symptom
On CT slices with `PixelRepresentation = 1` (signed two's complement, such as $-1024$ HU air), pixel values were interpreted as unsigned 16-bit integers ($64512$). Applying linear rescale intercept ($0.0$) and rescale slope ($1.0$) caused air regions to render as ultra-bright white rather than deep black.

### Root Cause Analysis
Raw pixel buffers were cast unconditionally to `uint16_t *` regardless of `(0028,0103)` Pixel Representation. When converting to Hounsfield Units, `(double)u16_val` yielded $+64512.0$ HU instead of $-1024.0$ HU.

### Test-Driven Reproduction
Implemented `test_bug2_signed_pixel_rescale()` in `tests/test_bug_reproductions.c`. Passed an array containing negative two's complement values (`0xFC00` = -1024), evaluated display mapping through `volume_hu_to_display()`, and asserted that air maps to display intensity 0 (black).

### Resolution
Updated voxel decoding in `volume_synthetic.c` and `volume_grid.c` to branch on `pixel_representation == 1`, casting to `int16_t *` before floating-point conversion.

---

## Bug 3: Out-of-Bounds Buffer Read on Sub-Voxel Coordinate Boundary

### Severity
Medium (Memory safety edge case)

### Symptom
When resampling orthogonal planes at the exact outer boundary ($x = N_x - 1.0$), trilinear interpolation evaluated index $x_1 = \lfloor x \rfloor + 1 = N_x$, causing an out-of-bounds array read beyond the volume grid buffer.

### Root Cause Analysis
Floor calculations `x0 = (size_t)floor(x)` followed by `x1 = x0 + 1` lacked clamping when $x \ge N_x - 1$:
```c
size_t x0 = (size_t)x;
size_t x1 = x0 + 1; // Out of bounds if x0 == dim_x - 1!
```

### Test-Driven Reproduction
Implemented `test_bug3_trilinear_boundary_clamping()` in `tests/test_bug_reproductions.c`. Queried coordinate $(N_x - 1.0, N_y - 1.0, N_z - 1.0)$ and verified that the computed value strictly matches the corner voxel with zero segmentation faults.

### Resolution
Enforced boundary clamping on both lower and upper coordinates:
```c
size_t x0 = (x < 0.0) ? 0 : (x >= dim_x) ? (dim_x - 1) : (size_t)x;
size_t x1 = (x0 + 1 < dim_x) ? (x0 + 1) : x0;
```

---

## Bug 4: Inconsistent Space vs Null Padding on Odd-Length Elements

### Severity
Medium (DICOM standard compliance)

### Symptom
Odd-length strings with VR `UI` (Unique Identifier) were padded with space characters (`0x20`) rather than null bytes (`0x00`), violating DICOM PS 3.5 Section 6.2 and failing external PACS validators.

### Root Cause Analysis
A generic padding routine appended space `' '` for all string VRs without distinguishing UI (Unique Identifier), which mandates `0x00` padding.

### Test-Driven Reproduction
Implemented `test_bug4_odd_length_padding()` in `tests/test_bug_reproductions.c`. Inserted an odd-length UID and an odd-length Person Name, verified exact trailing byte values per PS 3.5 rules.

### Resolution
Configured VR-specific padding:
```c
buf[len] = (vr == VR_UI) ? 0x00 : ' ';
```

---

## Bug 5: Thread-Local Static Buffer Collision in Dataset String Queries

### Severity
High (Data corruption under chained queries)

### Symptom
Evaluating multiple `dicom_dataset_get_string()` calls within a single expression or function caused earlier returned strings to be overwritten by subsequent queries.

### Root Cause Analysis
The function returned a pointer to a single shared static buffer:
```c
static char s_tmp[512]; // Overwritten on every call
```

### Test-Driven Reproduction
Caught during relational hierarchy integration test `tests/test_storage_hierarchy.c`, where `patient_id`, `study_uid`, and `series_uid` were evaluated in the same scope and collapsed to identical values.

### Resolution
Implemented a thread-local rotating ring buffer of 16 independent 512-byte slots:
```c
static _Thread_local char s_ring[16][512];
static _Thread_local size_t s_idx = 0;
char *s_tmp = s_ring[s_idx % 16];
s_idx++;
```

---

## Bug 6: Failure to Recursively Create Parent Directories in Storage Engine

### Severity
Medium (Filesystem error on fresh installations)

### Symptom
When starting Voxel on a clean machine without an existing `data/` directory, CAS storage and WAL append operations failed with `ENOENT` because `mkdir("data/test_cas", 0755)` could not create nested directories.

### Root Cause Analysis
Standard POSIX `mkdir()` is non-recursive and fails if intermediate directories are missing.

### Test-Driven Reproduction
Verified during clean automated build: `make clean && make test`.

### Resolution
Implemented `mkdir_recursive()` in `engine/src/storage_hierarchy.c` and `ensure_parent_dir()` in `engine/src/storage_wal.c`, guaranteeing that all parent directories are synthesized on demand before file creation.
