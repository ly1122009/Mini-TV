#include "ncl_player.h"

#include <gst/gst.h>

#define CAMERA_RETRY_SEC 2

typedef enum { SRC_NONE, SRC_CAMERA, SRC_FILE } source_kind_t;

struct ncl_player {
    ncl_config_t cfg;
    GstElement *pipeline;
    source_kind_t kind;
    GPtrArray *files;
    NCL_S32 index;
    guint bus_watch;
    guint retry_id;
};

static NCL_ERRORTYPE start_file(ncl_player_t *player, NCL_S32 idx);

/* Returns a newly allocated sink description; caller frees. */
static char *sink_desc(const ncl_config_t *cfg)
{
    if (g_strcmp0(cfg->sink, "kms") == 0) {
        return g_strdup("kmssink driver-name=vc4");  // card0 is v3d (render only) on Pi4
    }
    if (g_strcmp0(cfg->sink, "fb") == 0) {
        return g_strdup("fbdevsink");
    }
    if (g_strcmp0(cfg->sink, "preview") == 0) {
        // headless: keep rewriting one JPEG that conf/preview.html polls over HTTP
        return g_strdup_printf("videorate ! video/x-raw,framerate=%d/1 ! jpegenc quality=80 "
                               "! multifilesink location=%s/frame.jpg",
                               cfg->preview_fps, cfg->preview_dir);
    }
    return g_strdup("autovideosink");
}

static NCL_BOOLEAN has_video_ext(const char *name)
{
    static const char *const exts[] = {".mp4", ".mkv", ".ts", ".avi", ".mov", ".webm", NULL};

    for (int i = 0; exts[i]; i++) {
        if (g_str_has_suffix(name, exts[i])) {
            return NCL_TRUE;
        }
    }
    return NCL_FALSE;
}

static gint cmp_str(gconstpointer a, gconstpointer b)
{
    return g_strcmp0(*(char *const *)a, *(char *const *)b);
}

static void scan_media(ncl_player_t *player)
{
    GDir *dir = g_dir_open(player->cfg.media_dir, 0, NULL);
    const char *name;

    if (!dir) {
        g_printerr("Cannot open media dir: %s\n", player->cfg.media_dir);
        return;
    }
    while ((name = g_dir_read_name(dir))) {
        if (has_video_ext(name)) {
            g_ptr_array_add(player->files, g_build_filename(player->cfg.media_dir, name, NULL));
        }
    }
    g_dir_close(dir);
    g_ptr_array_sort(player->files, cmp_str);
}

static void stop_pipeline(ncl_player_t *player)
{
    if (player->retry_id) {
        g_source_remove(player->retry_id);
        player->retry_id = 0;
    }
    if (player->bus_watch) {
        g_source_remove(player->bus_watch);
        player->bus_watch = 0;
    }
    if (player->pipeline) {
        gst_element_set_state(player->pipeline, GST_STATE_NULL);
        gst_object_unref(player->pipeline);
        player->pipeline = NULL;
    }
}

static gboolean retry_camera(gpointer data)
{
    ncl_player_t *player = data;

    player->retry_id = 0;
    ncl_player_show_camera(player);
    return G_SOURCE_REMOVE;
}

static gboolean on_bus(GstBus *bus, GstMessage *msg, gpointer data)
{
    ncl_player_t *player = data;
    GError *err;
    char *dbg;

    (void)bus;
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &dbg);
        g_printerr("Pipeline error: %s (%s)\n", err->message, dbg ? dbg : "");
        g_error_free(err);
        g_free(dbg);
        if (player->kind == SRC_CAMERA) {  // UVC unplugged/stalled: keep retrying
            stop_pipeline(player);
            player->retry_id = g_timeout_add_seconds(CAMERA_RETRY_SEC, retry_camera, player);
        } else {
            ncl_player_next(player);
        }
        return G_SOURCE_REMOVE;
    case GST_MESSAGE_EOS:
        if (player->kind == SRC_FILE) {
            if (player->cfg.loop || player->index + 1 < (NCL_S32)player->files->len) {
                ncl_player_next(player);
            } else {
                ncl_player_show_camera(player);
            }
            return G_SOURCE_REMOVE;
        }
        break;
    default:
        break;
    }
    return G_SOURCE_CONTINUE;
}

static void start(ncl_player_t *player, GstElement *pipeline, source_kind_t kind)
{
    GstBus *bus;

    stop_pipeline(player);
    player->pipeline = pipeline;
    player->kind = kind;
    bus = gst_element_get_bus(pipeline);
    player->bus_watch = gst_bus_add_watch(bus, on_bus, player);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

NCL_ERRORTYPE ncl_player_show_camera(NCL_INOUT ncl_player_t *player)
{
    const ncl_config_t *cfg;
    char *sink, *desc;
    GError *err = NULL;
    GstElement *pipeline;

    if (!player) {
        return NCL_ErrorBadParameter;
    }
    cfg = &player->cfg;
    sink = sink_desc(cfg);
    if (g_strcmp0(cfg->cam_format, "yuyv") == 0) {
        desc = g_strdup_printf("v4l2src device=%s ! video/x-raw,format=YUY2,width=%d,height=%d,"
                               "framerate=%d/1 ! videoconvert ! videoscale ! %s",
                               cfg->cam_device, cfg->cam_width, cfg->cam_height, cfg->cam_fps,
                               sink);
    } else {
        desc = g_strdup_printf("v4l2src device=%s ! image/jpeg,width=%d,height=%d,framerate=%d/1 "
                               "! jpegdec ! videoconvert ! videoscale ! %s",
                               cfg->cam_device, cfg->cam_width, cfg->cam_height, cfg->cam_fps,
                               sink);
    }
    pipeline = gst_parse_launch(desc, &err);
    g_free(desc);
    g_free(sink);
    if (!pipeline) {
        g_printerr("Camera pipeline: %s\n", err->message);
        g_error_free(err);
        return NCL_ErrorUndefined;
    }
    if (err) {
        g_error_free(err);
    }
    g_print("Source: camera %s\n", cfg->cam_device);
    start(player, pipeline, SRC_CAMERA);
    return NCL_ErrorNone;
}

static NCL_ERRORTYPE start_file(ncl_player_t *player, NCL_S32 idx)
{
    GstElement *pipeline, *vsink;
    char *uri, *sink, *sdesc;
    GError *err = NULL;
    NCL_S32 count = (NCL_S32)player->files->len;

    if (count == 0) {
        g_printerr("No video files in %s\n", player->cfg.media_dir);
        return NCL_ErrorNotReady;
    }
    player->index = ((idx % count) + count) % count;

    sink = sink_desc(&player->cfg);
    sdesc = g_strdup_printf("videoconvert ! videoscale ! %s", sink);
    g_free(sink);
    vsink = gst_parse_bin_from_description(sdesc, TRUE, &err);
    g_free(sdesc);
    if (!vsink) {
        g_printerr("Sink: %s\n", err->message);
        g_error_free(err);
        return NCL_ErrorUndefined;
    }
    if (err) {
        g_error_free(err);
    }

    uri = gst_filename_to_uri(g_ptr_array_index(player->files, player->index), NULL);
    pipeline = gst_element_factory_make("playbin", "player");
    g_object_set(pipeline, "uri", uri, "video-sink", vsink, NULL);
    g_print("Source: file %s\n", uri);
    g_free(uri);
    start(player, pipeline, SRC_FILE);
    return NCL_ErrorNone;
}

NCL_ERRORTYPE ncl_player_play_file(NCL_INOUT ncl_player_t *player, NCL_IN const char *path)
{
    if (!player || !path) {
        return NCL_ErrorBadParameter;
    }
    g_ptr_array_insert(player->files, 0, g_strdup(path));
    return start_file(player, 0);
}

NCL_ERRORTYPE ncl_player_next(NCL_INOUT ncl_player_t *player)
{
    return player ? start_file(player, player->index + 1) : NCL_ErrorBadParameter;
}

NCL_ERRORTYPE ncl_player_prev(NCL_INOUT ncl_player_t *player)
{
    return player ? start_file(player, player->index - 1) : NCL_ErrorBadParameter;
}

NCL_ERRORTYPE ncl_player_toggle_pause(NCL_INOUT ncl_player_t *player)
{
    GstState state;

    if (!player || !player->pipeline) {
        return NCL_ErrorBadParameter;
    }
    gst_element_get_state(player->pipeline, &state, NULL, 0);
    gst_element_set_state(player->pipeline,
                          state == GST_STATE_PLAYING ? GST_STATE_PAUSED : GST_STATE_PLAYING);
    return NCL_ErrorNone;
}

NCL_ERRORTYPE ncl_player_seek_rel(NCL_INOUT ncl_player_t *player, NCL_IN NCL_S32 seconds)
{
    gint64 pos;

    if (!player || !player->pipeline) {
        return NCL_ErrorBadParameter;
    }
    if (player->kind != SRC_FILE) {
        return NCL_ErrorInvalidState;
    }
    if (!gst_element_query_position(player->pipeline, GST_FORMAT_TIME, &pos)) {
        return NCL_ErrorNotReady;
    }
    pos += (gint64)seconds * GST_SECOND;
    if (pos < 0) {
        pos = 0;
    }
    gst_element_seek_simple(player->pipeline, GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, pos);
    return NCL_ErrorNone;
}

ncl_player_t *ncl_player_new(NCL_IN const ncl_config_t *cfg)
{
    ncl_player_t *player;

    if (!cfg) {
        return NULL;
    }
    player = g_new0(ncl_player_t, 1);
    player->cfg = *cfg;
    player->cfg.cam_device = g_strdup(cfg->cam_device);
    player->cfg.cam_format = g_strdup(cfg->cam_format);
    player->cfg.sink = g_strdup(cfg->sink);
    player->cfg.preview_dir = g_strdup(cfg->preview_dir);
    player->cfg.media_dir = g_strdup(cfg->media_dir);
    g_mkdir_with_parents(player->cfg.preview_dir, 0755);
    player->files = g_ptr_array_new_with_free_func(g_free);
    player->index = -1;
    scan_media(player);
    return player;
}

void ncl_player_free(NCL_INOUT ncl_player_t *player)
{
    if (!player) {
        return;
    }
    stop_pipeline(player);
    g_ptr_array_free(player->files, TRUE);
    ncl_config_free(&player->cfg);
    g_free(player);
}
