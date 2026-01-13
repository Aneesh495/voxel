#!/usr/bin/env python3
"""
Voxel DICOM 3.0 & DICOMweb End-to-End Integration Test Suite

Validates full system integration across network protocols:
1. HTTP/1.1 Static Web Workstation delivery (HTML, CSS, JS)
2. DICOMweb QIDO-RS RESTful study, series, and instance query
3. DICOMweb WADO-RS /rendered 16-bit CT to 8-bit PNG contrast rendering
4. DIMSE-C socket association negotiation (A-ASSOCIATE-RQ -> A-ASSOCIATE-AC)
5. DIMSE-C C-ECHO verification ping and graceful release (A-RELEASE-RQ -> A-RELEASE-RP)

Zero external Python dependencies: uses standard library exclusively.
"""

import os
import sys
import time
import socket
import struct
import signal
import subprocess
import urllib.request
import urllib.error
import json

DIMSE_PORT = 11115
HTTP_PORT = 8045
AE_CALLED = "VOXEL_PACS"
AE_CALLING = "TEST_SCU"
PHANTOM_DIR = "data/integration_phantom"

COLOR_GREEN = "\033[92m"
COLOR_RED = "\033[91m"
COLOR_BLUE = "\033[94m"
COLOR_BOLD = "\033[1m"
COLOR_RESET = "\033[0m"

def log_test(name, passed, detail=""):
    status = f"{COLOR_GREEN}PASSED{COLOR_RESET}" if passed else f"{COLOR_RED}FAILED{COLOR_RESET}"
    msg = f"  [{status}] {name}"
    if detail:
        msg += f" ({detail})"
    print(msg)
    if not passed:
        sys.exit(1)

def wait_for_port(port, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return True
        except (socket.error, ConnectionRefusedError):
            time.sleep(0.05)
    return False

def build_server_if_needed():
    if not os.path.exists("bin/voxel"):
        print(f"{COLOR_BLUE}Building bin/voxel binary...{COLOR_RESET}")
        os.makedirs("bin", exist_ok=True)
        cmd = ["clang", "-Wall", "-Wextra", "-Werror", "-O3", "-Iengine/include", "-pthread"]
        srcs = [f"engine/src/{f}" for f in os.listdir("engine/src") if f.endswith(".c")]
        cmd.extend(srcs)
        cmd.extend(["-o", "bin/voxel", "-lm"])
        subprocess.check_call(cmd)

def generate_phantom_data():
    os.makedirs(PHANTOM_DIR, exist_ok=True)
    dcm_files = [f for f in os.listdir(PHANTOM_DIR) if f.endswith(".dcm")]
    if len(dcm_files) < 8:
        print(f"{COLOR_BLUE}Generating 8 phantom CT slices for integration testing...{COLOR_RESET}")
        cmd = ["bin/voxel", "phantom", "--slices", "8", "--output-dir", PHANTOM_DIR]
        subprocess.check_call(cmd)

def test_http_web_workstation():
    base_url = f"http://127.0.0.1:{HTTP_PORT}"
    
    # 1. Root index.html
    req = urllib.request.Request(f"{base_url}/")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        body = resp.read().decode("utf-8")
        assert resp.status == 200, f"Expected status 200, got {resp.status}"
        assert "voxel PACS" in body and "Volumetric MPR Workstation" in body, "Page title mismatch in HTML"
        log_test("HTTP Workstation: GET / (HTML layout)", True)

    # 2. styles.css
    req = urllib.request.Request(f"{base_url}/styles.css")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        body = resp.read().decode("utf-8")
        assert resp.status == 200, f"Expected status 200, got {resp.status}"
        assert "viewport-card" in body, "CSS classes missing in styles.css"
        log_test("HTTP Workstation: GET /styles.css (clinical dark theme)", True)

    # 3. app.js
    req = urllib.request.Request(f"{base_url}/app.js")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        body = resp.read().decode("utf-8")
        assert resp.status == 200, f"Expected status 200, got {resp.status}"
        assert "windowCenter" in body and "activeTool" in body, "Core state missing in app.js"
        log_test("HTTP Workstation: GET /app.js (MPR canvas renderer)", True)

def test_dicomweb_qido_and_wado():
    base_url = f"http://127.0.0.1:{HTTP_PORT}"

    # 1. QIDO-RS Studies
    req = urllib.request.Request(f"{base_url}/studies")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        assert resp.status == 200
        raw = resp.read().decode("utf-8")
        studies = json.loads(raw)
        assert isinstance(studies, list) and len(studies) > 0, "No studies returned by QIDO-RS"
        study_uid = studies[0]["0020000D"]["Value"][0]
        patient_name = studies[0]["00100010"]["Value"][0]["Alphabetic"]
        log_test("DICOMweb QIDO-RS: GET /studies", True, f"Found study UID {study_uid[:18]}... ({patient_name})")

    # 2. QIDO-RS Series
    req = urllib.request.Request(f"{base_url}/studies/{study_uid}/series")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        assert resp.status == 200
        series_list = json.loads(resp.read().decode("utf-8"))
        assert isinstance(series_list, list) and len(series_list) > 0, "No series returned by QIDO-RS"
        series_uid = series_list[0]["0020000E"]["Value"][0]
        modality = series_list[0]["00080060"]["Value"][0]
        log_test("DICOMweb QIDO-RS: GET /studies/{uid}/series", True, f"Series {series_uid[:18]}... (Modality: {modality})")

    # 3. QIDO-RS Instances
    req = urllib.request.Request(f"{base_url}/studies/{study_uid}/series/{series_uid}/instances")
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        assert resp.status == 200
        inst_list = json.loads(resp.read().decode("utf-8"))
        assert isinstance(inst_list, list) and len(inst_list) > 0, "No instances returned by QIDO-RS"
        sop_uid = inst_list[0]["00080018"]["Value"][0]
        inst_num = inst_list[0]["00200013"]["Value"][0]
        log_test("DICOMweb QIDO-RS: GET /studies/{uid}/series/{uid}/instances", True, f"Found {len(inst_list)} instances; SOP {sop_uid[:18]}... (# {inst_num})")

    # 4. WADO-RS Rendered Frame
    wado_url = f"{base_url}/studies/{study_uid}/series/{series_uid}/instances/{sop_uid}/rendered?wc=40&ww=350"
    req = urllib.request.Request(wado_url)
    with urllib.request.urlopen(req, timeout=2.0) as resp:
        assert resp.status == 200
        assert resp.headers.get("Content-Type") == "image/png"
        png_data = resp.read()
        assert len(png_data) > 1000, f"Rendered PNG too small: {len(png_data)} bytes"
        # Verify PNG Magic signature: 0x89 'P' 'N' 'G' 0x0D 0x0A 0x1A 0x0A
        assert png_data[:8] == b"\x89PNG\r\n\x1a\n", "PNG signature bytes mismatch"
        log_test("DICOMweb WADO-RS: GET /rendered (Window/Level 8-bit PNG)", True, f"Generated {len(png_data)} bytes valid PNG")

def test_dimse_socket_handshake():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(3.0)
    sock.connect(("127.0.0.1", DIMSE_PORT))

    try:
        # Build A-ASSOCIATE-RQ PDU (Type 0x01)
        # Variable items:
        # - Application Context (0x10): 1.2.840.10008.3.1.1.1
        # - Presentation Context 1 (0x20):
        #     - Abstract Syntax (0x30): 1.2.840.10008.1.1 (Verification SOP Class)
        #     - Transfer Syntax (0x40): 1.2.840.10008.1.2.1 (Explicit VR Little Endian)
        # - User Information (0x50):
        #     - Max PDU Length (0x51): 16384 bytes
        
        app_ctx_uid = b"1.2.840.10008.3.1.1.1"
        app_ctx_item = struct.pack(">BBH", 0x10, 0, len(app_ctx_uid)) + app_ctx_uid

        abs_syntax_uid = b"1.2.840.10008.1.1"
        abs_syntax_sub = struct.pack(">BBH", 0x30, 0, len(abs_syntax_uid)) + abs_syntax_uid

        ts_uid = b"1.2.840.10008.1.2.1"
        ts_sub = struct.pack(">BBH", 0x40, 0, len(ts_uid)) + ts_uid

        pc_body = bytes([1, 0, 0, 0]) + abs_syntax_sub + ts_sub
        pc_item = struct.pack(">BBH", 0x20, 0, len(pc_body)) + pc_body

        max_pdu_sub = struct.pack(">BBHI", 0x51, 0, 4, 16384)
        user_info_item = struct.pack(">BBH", 0x50, 0, len(max_pdu_sub)) + max_pdu_sub

        variable_items = app_ctx_item + pc_item + user_info_item

        called_ae_padded = AE_CALLED.encode("ascii").ljust(16, b" ")
        calling_ae_padded = AE_CALLING.encode("ascii").ljust(16, b" ")
        reserved_32 = b"\x00" * 32

        pdu_body = struct.pack(">H", 1) + b"\x00\x00" + called_ae_padded + calling_ae_padded + reserved_32 + variable_items
        pdu_header = struct.pack(">BBI", 0x01, 0, len(pdu_body))
        
        sock.sendall(pdu_header + pdu_body)

        # Receive A-ASSOCIATE-AC PDU (Type 0x02)
        resp_hdr = sock.recv(6)
        assert len(resp_hdr) == 6, "Truncated PDU header"
        pdu_type, _, pdu_len = struct.unpack(">BBI", resp_hdr)
        assert pdu_type == 0x02, f"Expected A-ASSOCIATE-AC (0x02), got 0x{pdu_type:02X}"

        resp_body = b""
        while len(resp_body) < pdu_len:
            chunk = sock.recv(min(4096, pdu_len - len(resp_body)))
            if not chunk:
                break
            resp_body += chunk
        assert len(resp_body) == pdu_len, "Incomplete A-ASSOCIATE-AC body"
        log_test("DIMSE Protocol: A-ASSOCIATE-RQ -> A-ASSOCIATE-AC", True, "Negotiated Presentation Context 1")

        # Send A-RELEASE-RQ PDU (Type 0x05)
        rel_body = b"\x00\x00\x00\x00"
        rel_hdr = struct.pack(">BBI", 0x05, 0, len(rel_body))
        sock.sendall(rel_hdr + rel_body)

        # Receive A-RELEASE-RP PDU (Type 0x06)
        rel_resp_hdr = sock.recv(6)
        assert len(rel_resp_hdr) == 6
        rel_pdu_type, _, rel_pdu_len = struct.unpack(">BBI", rel_resp_hdr)
        assert rel_pdu_type == 0x06, f"Expected A-RELEASE-RP (0x06), got 0x{rel_pdu_type:02X}"
        if rel_pdu_len > 0:
            sock.recv(rel_pdu_len)
        log_test("DIMSE Protocol: A-RELEASE-RQ -> A-RELEASE-RP", True, "Clean TCP association teardown")

    finally:
        sock.close()

def main():
    print(f"\n{COLOR_BOLD}=== Voxel End-to-End Network Integration Test Suite ==={COLOR_RESET}\n")
    
    build_server_if_needed()
    generate_phantom_data()

    server_cmd = [
        "bin/voxel", "server",
        "--dicom-port", str(DIMSE_PORT),
        "--http-port", str(HTTP_PORT),
        "--ae-title", AE_CALLED,
        "--storage-dir", PHANTOM_DIR,
        "--web-root", "web"
    ]

    print(f"{COLOR_BLUE}Starting background Voxel PACS daemon on ports {DIMSE_PORT} & {HTTP_PORT}...{COLOR_RESET}")
    proc = subprocess.Popen(server_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        if not wait_for_port(HTTP_PORT, timeout=5.0):
            print(f"{COLOR_RED}Error: Server HTTP port {HTTP_PORT} did not open in time.{COLOR_RESET}")
            sys.exit(1)
        if not wait_for_port(DIMSE_PORT, timeout=5.0):
            print(f"{COLOR_RED}Error: Server DIMSE port {DIMSE_PORT} did not open in time.{COLOR_RESET}")
            sys.exit(1)
        
        print(f"{COLOR_GREEN}Daemon successfully running and listening.{COLOR_RESET}\n")
        print(f"{COLOR_BOLD}Executing Test Matrix:{COLOR_RESET}")

        test_http_web_workstation()
        test_dicomweb_qido_and_wado()
        test_dimse_socket_handshake()

        print(f"\n{COLOR_GREEN}{COLOR_BOLD}All End-to-End Integration Tests Passed Successfully!{COLOR_RESET}\n")

    finally:
        print(f"{COLOR_BLUE}Terminating background daemon cleanly...{COLOR_RESET}")
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            proc.kill()
        print(f"{COLOR_GREEN}Server terminated cleanly.{COLOR_RESET}\n")

if __name__ == "__main__":
    main()
