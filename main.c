#include <glib-unix.h>
#include <gst/gst.h>
#include <signal.h>
#include <stdio.h>

#include "ncl_config.h"
#include "ncl_control.h"
#include "ncl_player.h"

#define DEFAULT_CONF_PATH "/etc/minitv/minitv.conf"

static gboolean on_signal(gpointer loop)
{
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
    ncl_config_t cfg;
    ncl_player_t *player;
    GMainLoop *loop;
    const char *conf_path = DEFAULT_CONF_PATH;

    gst_init(&argc, &argv);
    if (argc > 1) {
        conf_path = argv[1];
    }
    if (ncl_config_load(&cfg, conf_path) != NCL_ErrorNone) {
        g_printerr("Config %s not loaded, using defaults\n", conf_path);
    }

    player = ncl_player_new(&cfg);
    ncl_config_free(&cfg);
    if (!player) {
        return 1;
    }

    loop = g_main_loop_new(NULL, FALSE);
    ncl_control_attach_stdin(player, loop);
    g_unix_signal_add(SIGINT, on_signal, loop);
    g_unix_signal_add(SIGTERM, on_signal, loop);

    puts("Keys (then Enter): c=camera n=next p=prev s=pause f=+10s b=-10s q=quit");
    if (argc > 2) {
        ncl_player_play_file(player, argv[2]);
    } else {
        ncl_player_show_camera(player);
    }
    g_main_loop_run(loop);

    ncl_player_free(player);
    g_main_loop_unref(loop);
    return 0;
}
