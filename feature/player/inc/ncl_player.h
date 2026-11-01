#ifndef __NCL_PLAYER_H__
#define __NCL_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

#include "NCL_Core.h"
#include "NCL_Types.h"
#include "ncl_config.h"

typedef struct ncl_player ncl_player_t;

/** @brief Copies cfg (deep) and scans cfg->media_dir for video files. */
ncl_player_t *ncl_player_new(NCL_IN const ncl_config_t *cfg);
void ncl_player_free(NCL_INOUT ncl_player_t *player);

NCL_ERRORTYPE ncl_player_show_camera(NCL_INOUT ncl_player_t *player);
NCL_ERRORTYPE ncl_player_play_file(NCL_INOUT ncl_player_t *player, NCL_IN const char *path);
NCL_ERRORTYPE ncl_player_next(NCL_INOUT ncl_player_t *player);
NCL_ERRORTYPE ncl_player_prev(NCL_INOUT ncl_player_t *player);
NCL_ERRORTYPE ncl_player_toggle_pause(NCL_INOUT ncl_player_t *player);
NCL_ERRORTYPE ncl_player_seek_rel(NCL_INOUT ncl_player_t *player, NCL_IN NCL_S32 seconds);

#ifdef __cplusplus
}
#endif

#endif  // __NCL_PLAYER_H__
