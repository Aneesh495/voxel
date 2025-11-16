// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::net::server
// Dual-protocol PACS server orchestrator coordinating DIMSE TCP listener (port 11112)
// and HTTP/1.1 DICOMweb listener (port 8042).

use voxel::types;
use voxel::dimse::association;
use voxel::dimse::service;
use voxel::net::http;
use voxel::net::tcp;
use voxel::storage::hierarchy;

export type server_config = struct {
	ae_title: [16]u8,
	dicom_port: u16,
	http_port: u16,
	storage_path: []u8,
	enable_mpr_caching: bool,
};

export type pacs_server = struct {
	config: server_config,
	is_running: bool,
	active_associations: size,
	total_studies_indexed: size,
	total_instances_indexed: size,
};

// Initializes default configuration for the voxel PACS server.
export fn server_config_default() server_config = {
	let cfg = server_config {
		ae_title = ['V', 'O', 'X', 'E', 'L', '_', 'P', 'A', 'C', 'S', ' ', ' ', ' ', ' ', ' ', ' '],
		dicom_port = 11112u16,
		http_port = 8042u16,
		storage_path = ['d', 'a', 't', 'a'],
		enable_mpr_caching = true,
	};
	return cfg;
};

// Initializes the server orchestrator state.
export fn pacs_server_init(cfg: server_config) pacs_server = {
	return pacs_server {
		config = cfg,
		is_running = false,
		active_associations = 0z,
		total_studies_indexed = 0z,
		total_instances_indexed = 0z,
	};
};
