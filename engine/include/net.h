#ifndef VOXEL_NET_H
#define VOXEL_NET_H

#include "storage.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ae_title[17];
    uint16_t dicom_port;
    uint16_t http_port;
    char storage_dir[256];
    char web_root[256];
    bool daemon_mode;
} pacs_config_t;

typedef struct {
    pacs_config_t config;
    storage_hierarchy_t *hierarchy;
    volume_grid_t *active_volume;
    int dicom_listen_fd;
    int http_listen_fd;
    volatile bool is_running;
} pacs_server_t;

pacs_server_t *pacs_server_create(const pacs_config_t *cfg);
void pacs_server_destroy(pacs_server_t *srv);
bool pacs_server_start(pacs_server_t *srv);
void pacs_server_stop(pacs_server_t *srv);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_NET_H
