#pragma once

// Russian Web UI localization layer.
//
//  1. ExpressLRS HTML templates remain the structure.
//  2. Visible copy on MODEL / OPTIONS / WIFI / UPDATE is Russian.
//  3. src/html/i18n-ru.js is @@include'd into scan.js / hardware.js
//     and gerdaApplyI18n() runs on DOMContentLoaded (data-i18n keys).
//  4. Do not ship a fully duplicated Russian HTML tree (flash cost on ESP32-C3).

#ifdef __cplusplus
extern "C" {
#endif

enum gerda_ui_lang {
    GERDA_UI_LANG_EN = 0,
    GERDA_UI_LANG_RU = 1,
};

static inline const char *gerda_ui_lang_name(enum gerda_ui_lang lang)
{
    return lang == GERDA_UI_LANG_RU ? "ru" : "en";
}

#ifdef __cplusplus
}
#endif
