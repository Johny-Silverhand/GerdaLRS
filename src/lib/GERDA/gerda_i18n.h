#pragma once

// Russian Web UI localization layer (design + stub).
//
// Plan:
//  1. Keep ExpressLRS HTML templates as the source of English strings.
//  2. Add a small i18n table (see src/html/i18n-ru.stub.js) keyed by
//     stable English phrases / data-i18n ids.
//  3. Apply translations in the browser after DOM load so the gzip HTML
//     budget stays close to stock ELRS.
//  4. Do not ship a fully duplicated Russian HTML tree (flash cost on ESP32-C3).
//
// Status: dictionary stub only; index.html is not yet wired to apply it.

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
