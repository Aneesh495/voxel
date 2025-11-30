#ifndef VOXEL_DICOMWEB_H
#define VOXEL_DICOMWEB_H

#include "storage.h"
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif

// QIDO-RS JSON query handler
char *dicomweb_qido_query_studies(const storage_hierarchy_t *h, const char *query_params);
char *dicomweb_qido_query_series(const storage_hierarchy_t *h, const char *study_uid, const char *query_params);
char *dicomweb_qido_query_instances(const storage_hierarchy_t *h, const char *study_uid, const char *series_uid, const char *query_params);

// WADO-RS handlers
char *dicomweb_wado_get_study_metadata(const storage_hierarchy_t *h, const char *study_uid);
uint8_t *dicomweb_wado_render_instance_png(
    const storage_hierarchy_t *h,
    const char *study_uid,
    const char *series_uid,
    const char *instance_uid,
    window_level_t wl,
    size_t *out_png_len
);
uint8_t *dicomweb_wado_render_mpr_png(
    const volume_grid_t *g,
    mpr_plane_t plane,
    size_t slice_idx,
    window_level_t wl,
    size_t *out_png_len
);

// STOW-RS handler
bool dicomweb_stow_ingest_multipart(
    storage_hierarchy_t *h,
    const uint8_t *body,
    size_t body_len,
    const char *content_type_header,
    size_t *out_stored_count
);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_DICOMWEB_H
