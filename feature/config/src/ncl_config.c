#include "ncl_config.h"

#include <glib.h>

static char *get_str(GKeyFile *kf, const char *grp, const char *key, const char *def)
{
    char *val = kf ? g_key_file_get_string(kf, grp, key, NULL) : NULL;

    return val ? val : g_strdup(def);
}

static NCL_S32 get_int(GKeyFile *kf, const char *grp, const char *key, NCL_S32 def)
{
    GError *err = NULL;
    NCL_S32 val = kf ? g_key_file_get_integer(kf, grp, key, &err) : def;

    if (err) {
        g_error_free(err);
        return def;
    }
    return val;
}

NCL_ERRORTYPE ncl_config_load(NCL_OUT ncl_config_t *cfg, NCL_IN const char *path)
{
    GKeyFile *kf = NULL;
    NCL_BOOLEAN loaded = NCL_FALSE;

    if (!cfg) {
        return NCL_ErrorBadParameter;
    }
    if (path) {
        kf = g_key_file_new();
        loaded = g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL) ? NCL_TRUE : NCL_FALSE;
        if (!loaded) {
            g_key_file_free(kf);
            kf = NULL;
        }
    }

    cfg->cam_device = get_str(kf, "camera", "device", "/dev/video0");
    cfg->cam_format = get_str(kf, "camera", "format", "mjpg");
    cfg->cam_width = get_int(kf, "camera", "width", 1280);
    cfg->cam_height = get_int(kf, "camera", "height", 720);
    cfg->cam_fps = get_int(kf, "camera", "fps", 30);
    cfg->sink = get_str(kf, "display", "sink", "kms");
    cfg->preview_dir = get_str(kf, "display", "preview_dir", "/tmp/minitv");
    cfg->preview_fps = get_int(kf, "display", "preview_fps", 10);
    cfg->media_dir = get_str(kf, "media", "dir", ".");
    cfg->loop = (kf && g_key_file_has_key(kf, "media", "loop", NULL))
                    ? (g_key_file_get_boolean(kf, "media", "loop", NULL) ? NCL_TRUE : NCL_FALSE)
                    : NCL_TRUE;

    if (kf) {
        g_key_file_free(kf);
    }
    return loaded ? NCL_ErrorNone : NCL_ErrorNotReady;
}

void ncl_config_free(NCL_INOUT ncl_config_t *cfg)
{
    if (!cfg) {
        return;
    }
    g_free(cfg->cam_device);
    g_free(cfg->cam_format);
    g_free(cfg->sink);
    g_free(cfg->preview_dir);
    g_free(cfg->media_dir);
}
