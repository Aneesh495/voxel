// voxel: High-Performance DICOM 3.0 PACS and Volumetric Tomography Engine
// Module: voxel::net::tcp
// Non-blocking TCP network abstractions, connection framing, and socket state.

use voxel::types;

export type connection_state = enum {
	DISCONNECTED,
	CONNECTING,
	CONNECTED,
	ERROR,
};

export type tcp_connection = struct {
	fd: int,
	state: connection_state,
	peer_ip: [16]u8,
	peer_ip_len: size,
	peer_port: u16,
	rx_buffer: []u8,
	rx_len: size,
	tx_buffer: []u8,
	tx_len: size,
};

// Initializes a TCP connection context with allocated receive and transmit buffers.
export fn tcp_connection_init(
	rx_storage: []u8,
	tx_storage: []u8,
) tcp_connection = {
	return tcp_connection {
		fd = -1,
		state = connection_state::DISCONNECTED,
		peer_ip = [0...],
		peer_ip_len = 0z,
		peer_port = 0u16,
		rx_buffer = rx_storage,
		rx_len = 0z,
		tx_buffer = tx_storage,
		tx_len = 0z,
	};
};

// Resets connection state and clears communication buffers.
export fn tcp_connection_reset(conn: *tcp_connection) void = {
	conn.state = connection_state::DISCONNECTED;
	conn.rx_len = 0z;
	conn.tx_len = 0z;
	conn.peer_port = 0u16;
	conn.peer_ip_len = 0z;
};
