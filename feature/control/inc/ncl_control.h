#ifndef __NCL_CONTROL_H__
#define __NCL_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <glib.h>

#include "NCL_Core.h"
#include "ncl_player.h"

/**
 * @brief Read single-key commands from stdin and drive the player:
 *        c=camera n=next p=prev s/space=pause f=+10s b=-10s q=quit (stops loop).
 */
NCL_ERRORTYPE ncl_control_attach_stdin(NCL_INOUT ncl_player_t *player, NCL_INOUT GMainLoop *loop);

#ifdef __cplusplus
}
#endif

#endif  // __NCL_CONTROL_H__
