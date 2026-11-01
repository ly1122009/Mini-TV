#ifndef __NCL_CONFIG_H__
#define __NCL_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "NCL_Core.h"
#include "NCL_Types.h"

typedef struct ncl_config {
    NCL_STRING cam_device;
    NCL_STRING cam_format;  // "mjpg" | "yuyv"
    NCL_S32 cam_width;
    NCL_S32 cam_height;
    NCL_S32 cam_fps;
    NCL_STRING sink;  // "kms" | "fb" | "preview" | "auto"
    NCL_STRING preview_dir;  // preview sink writes frame.jpg here
    NCL_S32 preview_fps;
    NCL_STRING media_dir;
    NCL_BOOLEAN loop;
} ncl_config_t;

/**
 * @brief Fill cfg with defaults, then override from the ini file at path (if it loads).
 * @return NCL_ErrorNone when the file was read, NCL_ErrorNotReady when defaults are used.
 */
NCL_ERRORTYPE ncl_config_load(NCL_OUT ncl_config_t *cfg, NCL_IN const char *path);
void ncl_config_free(NCL_INOUT ncl_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif  // __NCL_CONFIG_H__
