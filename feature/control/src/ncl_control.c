#include "ncl_control.h"

#include <glib-unix.h>
#include <unistd.h>

#define SEEK_STEP_SEC 10

typedef struct {
    ncl_player_t *player;
    GMainLoop *loop;
} control_ctx_t;

static gboolean on_stdin(gint fd, GIOCondition cond, gpointer data)
{
    control_ctx_t *ctx = data;
    char buf[64];
    ssize_t len = read(fd, buf, sizeof(buf));

    (void)cond;

    if (len <= 0) {
        return G_SOURCE_REMOVE;  // stdin closed (e.g. under systemd)
    }
    for (ssize_t i = 0; i < len; i++) {
        switch (buf[i]) {
        case 'c':
            ncl_player_show_camera(ctx->player);
            break;
        case 'n':
            ncl_player_next(ctx->player);
            break;
        case 'p':
            ncl_player_prev(ctx->player);
            break;
        case 's':
        case ' ':
            ncl_player_toggle_pause(ctx->player);
            break;
        case 'f':
            ncl_player_seek_rel(ctx->player, SEEK_STEP_SEC);
            break;
        case 'b':
            ncl_player_seek_rel(ctx->player, -SEEK_STEP_SEC);
            break;
        case 'q':
            g_main_loop_quit(ctx->loop);
            break;
        default:
            break;
        }
    }
    return G_SOURCE_CONTINUE;
}

NCL_ERRORTYPE ncl_control_attach_stdin(NCL_INOUT ncl_player_t *player, NCL_INOUT GMainLoop *loop)
{
    control_ctx_t *ctx;

    if (!player || !loop) {
        return NCL_ErrorBadParameter;
    }
    ctx = g_new0(control_ctx_t, 1);
    ctx->player = player;
    ctx->loop = loop;
    g_unix_fd_add_full(G_PRIORITY_DEFAULT, STDIN_FILENO, G_IO_IN, on_stdin, ctx, g_free);
    return NCL_ErrorNone;
}
