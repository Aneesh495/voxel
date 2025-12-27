#include "net.h"
#include "dicomweb.h"
#include "dimse.h"
#include "volume.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>

static int setup_listen_socket(uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 64) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static void send_http_response(int fd, int status, const char *content_type, const uint8_t *body, size_t body_len) {
    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization, Accept\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, (status == 200 ? "OK" : (status == 204 ? "No Content" : "Not Found")),
        content_type ? content_type : "text/plain",
        body_len);

    write(fd, header, (size_t)hlen);
    if (body && body_len > 0) {
        write(fd, body, body_len);
    }
}

static void handle_http_client(pacs_server_t *srv, int client_fd) {
    char req_buf[8192];
    ssize_t n = read(client_fd, req_buf, sizeof(req_buf) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    req_buf[n] = '\0';

    char method[16] = {0};
    char path[1024] = {0};
    sscanf(req_buf, "%15s %1023s", method, path);

    // CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_fd, 204, "text/plain", NULL, 0);
        close(client_fd);
        return;
    }

    // Static Web Workstation files
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/index.html", srv->config.web_root);
        FILE *f = fopen(file_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *b = (uint8_t *)malloc((size_t)sz);
            if (b) {
                fread(b, 1, (size_t)sz, f);
                send_http_response(client_fd, 200, "text/html", b, (size_t)sz);
                free(b);
            }
            fclose(f);
        } else {
            const char *fallback = "<!DOCTYPE html><html><head><title>voxel PACS</title></head><body><h1>voxel PACS Server Active</h1></body></html>";
            send_http_response(client_fd, 200, "text/html", (const uint8_t *)fallback, strlen(fallback));
        }
        close(client_fd);
        return;
    }

    if (strcmp(path, "/styles.css") == 0) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/styles.css", srv->config.web_root);
        FILE *f = fopen(file_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *b = (uint8_t *)malloc((size_t)sz);
            if (b) {
                fread(b, 1, (size_t)sz, f);
                send_http_response(client_fd, 200, "text/css", b, (size_t)sz);
                free(b);
            }
            fclose(f);
        } else {
            send_http_response(client_fd, 404, "text/plain", (const uint8_t *)"Not Found", 9);
        }
        close(client_fd);
        return;
    }

    if (strcmp(path, "/app.js") == 0) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/app.js", srv->config.web_root);
        FILE *f = fopen(file_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *b = (uint8_t *)malloc((size_t)sz);
            if (b) {
                fread(b, 1, (size_t)sz, f);
                send_http_response(client_fd, 200, "application/javascript", b, (size_t)sz);
                free(b);
            }
            fclose(f);
        } else {
            send_http_response(client_fd, 404, "text/plain", (const uint8_t *)"Not Found", 9);
        }
        close(client_fd);
        return;
    }

    // QIDO-RS Study query
    if (strncmp(path, "/studies", 8) == 0 && (path[8] == '\0' || path[8] == '?')) {
        char *json = dicomweb_qido_query_studies(srv->hierarchy, strchr(path, '?'));
        if (json) {
            send_http_response(client_fd, 200, "application/dicom+json", (const uint8_t *)json, strlen(json));
            free(json);
        } else {
            send_http_response(client_fd, 200, "application/dicom+json", (const uint8_t *)"[]", 2);
        }
        close(client_fd);
        return;
    }

    // MPR Slice Rendering: /mpr?plane=axial&slice=32&windowCenter=40&windowWidth=350
    if (strncmp(path, "/mpr", 4) == 0) {
        mpr_plane_t plane = MPR_PLANE_AXIAL;
        size_t slice_idx = 32;
        window_level_t wl = { .center = 40.0, .width = 350.0 };

        char *plane_param = strstr(path, "plane=");
        if (plane_param) {
            if (strncmp(plane_param + 6, "coronal", 7) == 0) plane = MPR_PLANE_CORONAL;
            else if (strncmp(plane_param + 6, "sagittal", 8) == 0) plane = MPR_PLANE_SAGITTAL;
        }

        char *slice_param = strstr(path, "slice=");
        if (slice_param) {
            slice_idx = (size_t)atoi(slice_param + 6);
        }

        char *wc_param = strstr(path, "windowCenter=");
        if (wc_param) wl.center = atof(wc_param + 13);
        char *ww_param = strstr(path, "windowWidth=");
        if (ww_param) wl.width = atof(ww_param + 12);

        size_t png_len = 0;
        uint8_t *png = dicomweb_wado_render_mpr_png(srv->active_volume, plane, slice_idx, wl, &png_len);
        if (png) {
            send_http_response(client_fd, 200, "image/png", png, png_len);
            free(png);
        } else {
            send_http_response(client_fd, 404, "text/plain", (const uint8_t *)"Volume Not Ready", 16);
        }
        close(client_fd);
        return;
    }

    // 404 Fallback
    send_http_response(client_fd, 404, "text/plain", (const uint8_t *)"Not Found", 9);
    close(client_fd);
}

static void *http_thread_entry(void *arg) {
    pacs_server_t *srv = (pacs_server_t *)arg;
    while (srv->is_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(srv->http_listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd >= 0) {
            handle_http_client(srv, client_fd);
        } else {
            if (!srv->is_running) break;
            usleep(10000);
        }
    }
    return NULL;
}

static void handle_dicom_client(pacs_server_t *srv, int client_fd) {
    dimse_association_t *assoc = dimse_association_create(srv->config.ae_title);
    if (!assoc) {
        close(client_fd);
        return;
    }

    uint8_t rx_buf[16384];
    while (srv->is_running) {
        ssize_t n = read(client_fd, rx_buf, sizeof(rx_buf));
        if (n <= 0) break;

        uint8_t pdu_type = rx_buf[0];

        if (pdu_type == PDU_A_ASSOCIATE_RQ) {
            uint8_t ac_buf[2048];
            size_t ac_len = 0;
            if (dimse_process_associate_rq(assoc, rx_buf, (size_t)n, ac_buf, &ac_len)) {
                write(client_fd, ac_buf, ac_len);
            } else {
                break;
            }
        } else if (pdu_type == PDU_P_DATA_TF) {
            // Check command PDV (0x01 = C-ECHO-RQ)
            // Respond with C-ECHO-RSP
            uint8_t rsp_cmd[256];
            size_t rsp_len = 0;
            dimse_create_echo_rsp(1, UID_VERIFICATION_SOP_CLASS, DIMSE_STATUS_SUCCESS, rsp_cmd, &rsp_len);

            // Wrap in P-DATA-TF
            uint8_t pdu_out[512];
            uint32_t pdv_len = (uint32_t)(rsp_len + 2);
            uint32_t total_len = pdv_len + 4;

            pdu_out[0] = PDU_P_DATA_TF;
            pdu_out[1] = 0x00;
            pdu_out[2] = (uint8_t)((total_len >> 24) & 0xFF);
            pdu_out[3] = (uint8_t)((total_len >> 16) & 0xFF);
            pdu_out[4] = (uint8_t)((total_len >> 8) & 0xFF);
            pdu_out[5] = (uint8_t)(total_len & 0xFF);

            pdu_out[6] = (uint8_t)((pdv_len >> 24) & 0xFF);
            pdu_out[7] = (uint8_t)((pdv_len >> 16) & 0xFF);
            pdu_out[8] = (uint8_t)((pdv_len >> 8) & 0xFF);
            pdu_out[9] = (uint8_t)(pdv_len & 0xFF);

            pdu_out[10] = 0x01; // Context ID
            pdu_out[11] = 0x03; // Command + Last fragment
            memcpy(pdu_out + 12, rsp_cmd, rsp_len);

            write(client_fd, pdu_out, (size_t)(12 + rsp_len));
        } else if (pdu_type == PDU_A_RELEASE_RQ) {
            // Respond with A-RELEASE-RP (fixed 10 bytes)
            uint8_t rp[10] = { PDU_A_RELEASE_RP, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 };
            write(client_fd, rp, 10);
            break;
        } else if (pdu_type == PDU_A_ABORT) {
            break;
        }
    }

    dimse_association_destroy(assoc);
    close(client_fd);
}

static void *dicom_thread_entry(void *arg) {
    pacs_server_t *srv = (pacs_server_t *)arg;
    while (srv->is_running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(srv->dicom_listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd >= 0) {
            handle_dicom_client(srv, client_fd);
        } else {
            if (!srv->is_running) break;
            usleep(10000);
        }
    }
    return NULL;
}

pacs_server_t *pacs_server_create(const pacs_config_t *cfg) {
    pacs_server_t *srv = (pacs_server_t *)calloc(1, sizeof(pacs_server_t));
    if (!srv) return NULL;
    if (cfg) {
        srv->config = *cfg;
    } else {
        strncpy(srv->config.ae_title, "VOXEL_PACS", 16);
        srv->config.dicom_port = 11112;
        srv->config.http_port = 8042;
        strncpy(srv->config.storage_dir, "data", 255);
        strncpy(srv->config.web_root, "web", 255);
    }
    srv->hierarchy = storage_hierarchy_create(srv->config.storage_dir);
    srv->active_volume = volume_generate_synthetic_head(256, 256, 64);
    srv->dicom_listen_fd = -1;
    srv->http_listen_fd = -1;
    srv->is_running = false;
    return srv;
}

void pacs_server_destroy(pacs_server_t *srv) {
    if (!srv) return;
    pacs_server_stop(srv);
    if (srv->hierarchy) storage_hierarchy_destroy(srv->hierarchy);
    if (srv->active_volume) volume_grid_destroy(srv->active_volume);
    free(srv);
}

bool pacs_server_start(pacs_server_t *srv) {
    if (!srv || srv->is_running) return false;

    srv->dicom_listen_fd = setup_listen_socket(srv->config.dicom_port);
    srv->http_listen_fd = setup_listen_socket(srv->config.http_port);

    if (srv->dicom_listen_fd < 0 || srv->http_listen_fd < 0) {
        if (srv->dicom_listen_fd >= 0) close(srv->dicom_listen_fd);
        if (srv->http_listen_fd >= 0) close(srv->http_listen_fd);
        return false;
    }

    srv->is_running = true;

    pthread_t th_http, th_dcm;
    pthread_create(&th_http, NULL, http_thread_entry, srv);
    pthread_create(&th_dcm, NULL, dicom_thread_entry, srv);
    pthread_detach(th_http);
    pthread_detach(th_dcm);

    return true;
}

void pacs_server_stop(pacs_server_t *srv) {
    if (!srv || !srv->is_running) return;
    srv->is_running = false;
    if (srv->dicom_listen_fd >= 0) {
        close(srv->dicom_listen_fd);
        srv->dicom_listen_fd = -1;
    }
    if (srv->http_listen_fd >= 0) {
        close(srv->http_listen_fd);
        srv->http_listen_fd = -1;
    }
}
