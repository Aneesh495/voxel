# voxel Design Charter and System Specification

## 1. Executive Summary

`voxel` is a high-performance medical imaging Picture Archiving and Communication System (PACS), DICOM 3.0 network protocol engine, volumetric Multi-Planar Reconstruction (MPR) processor, and DICOMweb RESTful server implemented with Hare at its architectural core.

Modern clinical imaging environments generate high-resolution volumetric data across computed tomography (CT), magnetic resonance imaging (MRI), positron emission tomography (PET), and digital radiography (CR/DX). Traditional PACS archives frequently suffer from legacy technical debt: bloated memory footprints, sluggish slice stack traversal, rigid monolithic architectures, and poor web integration.

`voxel` was conceived to address these limitations through fundamental systems programming principles:
- Predictable, manual memory management without hidden runtime or garbage collection overhead.
- Strict type safety and tagged union modeling for complex binary wire protocols.
- Zero-copy buffer pooling for multi-megabyte image frame ingestion and transfer.
- SIMD-friendly contiguous memory layouts for volumetric reconstruction grids.
- Native dual-stack protocol support: DICOM DIMSE over TCP (port 11112) and HTTP/1.1 DICOMweb (WADO-RS, QIDO-RS, STOW-RS on port 8042).

```
+-------------------------------------------------------------------------+
|                              voxel PACS                                 |
+------------------------------------+------------------------------------+
|         DICOM DIMSE Protocol       |          DICOMweb HTTP/1.1         |
|  (C-ECHO, C-STORE, C-FIND, C-MOVE) |   (WADO-RS, QIDO-RS, STOW-RS)      |
+------------------------------------+------------------------------------+
|                         Presentation Layer                              |
|          PDU State Machine, Association Negotiation, PDV Codec          |
+-------------------------------------------------------------------------+
|                     DICOM Part 10 Dataset Engine                        |
|   Element Dictionary, VR Validators, Sequence Trees, Binary Serialization|
+-------------------------------------------------------------------------+
|                  Volumetric Reconstruction Pipeline                     |
|    Modality LUT (HU), VOI LUT (Window/Level), 3D Grid Assembly,         |
|    Orthogonal MPR (Axial, Coronal, Sagittal), Trilinear Interpolation,  |
|                     MIP / MinIP / AIP Projections                       |
+-------------------------------------------------------------------------+
|                    Storage & Persistence Engine                         |
|   4-Tier Relational Hierarchy (Patient -> Study -> Series -> Instance), |
|    B-Tree Indexing, Content-Addressable Storage (CAS), CRC32 WAL        |
+-------------------------------------------------------------------------+
```

## 2. Core Architectural Pillars

### 2.1 Protocol Compliance and Binary Integrity
Medical imaging protocols require strict adherence to the NEMA PS3 / ISO 12052 DICOM standard. `voxel` implements:
- DICOM Upper Layer Protocol for TCP/IP (PS3.8) including Association Negotiation (A-ASSOCIATE-RQ, A-ASSOCIATE-AC, A-ASSOCIATE-RJ), Data Transfer (P-DATA-TF), and Association Release/Abort.
- Composite DIMSE-C services (PS3.7): C-ECHO (verification), C-STORE (storage), C-FIND (query), C-MOVE (retrieval orchestration), and C-GET.
- DICOM Part 10 Media Storage (PS3.10): 128-byte preamble, `'D','I','C','M'` magic identifier, File Meta Information header (Group 0002) with transfer syntax detection, and streaming dataset body deserialization.
- Support for key transfer syntaxes:
  - Implicit VR Little Endian (1.2.840.10008.1.2)
  - Explicit VR Little Endian (1.2.840.10008.1.2.1)
  - Explicit VR Big Endian (1.2.840.10008.1.2.2)
  - Deflated Explicit VR Little Endian (1.2.840.10008.1.2.1.99)
  - RLE Lossless (1.2.840.10008.1.2.5)

### 2.2 Volumetric Reconstruction and Spatial Geometry
A key innovation in `voxel` is its native 3D Multi-Planar Reconstruction (MPR) engine. While classic PACS systems treat DICOM files as disconnected 2D slices, `voxel` constructs a continuous volumetric spatial field:
- Patient Coordinate System (RCS): Establishes spatial orientation using `ImageOrientationPatient` direction cosines $[X_x, X_y, X_z, Y_x, Y_y, Y_z]$ and slice normal vector $N = X \times Y$.
- Slice stack ordering: Detects slice sorting along the normal vector $d = S \cdot N$, detecting variable slice spacing or gaps.
- Trilinear Voxel Resampling: Generates arbitrary orthogonal planes (Axial, Coronal, Sagittal) and oblique resampled cuts with 8-point weighted distance interpolation.
- Maximum Intensity Projection (MIP): Computes ray-cast peak absorption projections for vascular angiography visualization.

### 2.3 Relational Storage Model and Content-Addressable Storage
`voxel` models the DICOM clinical hierarchy across four distinct tiers:
1. Patient Tier: PatientID, PatientName, BirthDate, Sex.
2. Study Tier: StudyInstanceUID, StudyDate, StudyTime, AccessionNumber, ReferringPhysician, StudyDescription.
3. Series Tier: SeriesInstanceUID, Modality (CT, MR, CR, DX, US, PT, NM), SeriesNumber, BodyPartExamined.
4. Instance Tier: SOPInstanceUID, InstanceNumber, Rows, Columns, BitsAllocated, PixelSpacing, ImagePositionPatient.

Files are stored immutably in a Content-Addressable Storage (CAS) hierarchy addressed by SHA-256 digests, eliminating redundant on-disk storage of identical SOP instances. Metadata transactions are guarded by an append-only Write-Ahead Log (WAL) with CRC32 frame checksums.

### 2.4 Modern Web Access (DICOMweb)
In addition to legacy DIMSE TCP services, `voxel` provides a zero-dependency HTTP/1.1 DICOMweb server implementing PS3.18:
- QIDO-RS: `/studies`, `/studies/{studyUID}/series`, `/studies/{studyUID}/series/{seriesUID}/instances` query endpoints returning DICOM JSON format.
- WADO-RS: Retrieval of bulk datasets, instances, frames, and on-the-fly rendered PNG images with Window/Level contrast curves applied.
- STOW-RS: Ingestion of multipart/related DICOM streams over HTTP POST.
- Clinical Web Workstation: An embedded interactive multi-viewport viewer enabling real-time MPR exploration, windowing adjustment, and measurement tools directly in any modern browser.

## 3. Scope and Engineering Standards

The project follows rigorous systems engineering principles:
- Clean modular design with strict encapsulation.
- Zero third-party runtime package dependencies for the core server and web viewer.
- Explicit error handling through tagged unions.
- Comprehensive unit, integration, and regression test suites.
- Microbenchmarks measuring throughput and latency under high concurrency.
