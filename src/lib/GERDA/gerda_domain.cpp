#include "gerda_domain.h"

uint8_t gerda_band = GERDA_BAND_ISM24;

static const gerda_band_info_t kCatalog[] = {
    {GERDA_BAND_ISM24, GERDA_KIND_ISM24, 0xFF, 1, 1,
        "ISM2G4", "ISM 2.4",
        2400400000u, 2479400000u, 80, 2400400000u},
    {GERDA_BAND_CE_LBT, GERDA_KIND_CE_LBT, 0xFF, 0, 1,
        "CE_LBT", "CE/EU LBT 2.4",
        2400400000u, 2479400000u, 80, 2400400000u},
    {10, GERDA_KIND_SUBGHZ, 0, 1, 0, "AU915", "AU 915", 915500000u, 926900000u, 20, 0},
    {11, GERDA_KIND_SUBGHZ, 1, 1, 0, "FCC915", "FCC 915", 903500000u, 926900000u, 40, 0},
    {12, GERDA_KIND_SUBGHZ, 2, 1, 0, "EU868", "EU 868", 865275000u, 869575000u, 13, 0},
    {13, GERDA_KIND_SUBGHZ, 3, 1, 0, "IN866", "IN 866", 865375000u, 866950000u, 4, 0},
    {14, GERDA_KIND_SUBGHZ, 4, 1, 0, "AU433", "AU 433", 433420000u, 434420000u, 3, 0},
    {15, GERDA_KIND_SUBGHZ, 5, 1, 0, "EU433", "EU 433", 433100000u, 434450000u, 3, 0},
    {16, GERDA_KIND_SUBGHZ, 6, 1, 0, "US433", "US 433", 433250000u, 438000000u, 8, 0},
    {17, GERDA_KIND_SUBGHZ, 7, 1, 0, "US433W", "US 433 Wide", 423500000u, 438000000u, 20, 0},
    {20, GERDA_KIND_DUAL, 0, 1, 0, "D-AU915", "Dual AU 915 + ISM 2.4", 915500000u, 926900000u, 20, 2400400000u},
    {21, GERDA_KIND_DUAL, 1, 1, 0, "D-FCC915", "Dual FCC 915 + ISM 2.4", 903500000u, 926900000u, 40, 2400400000u},
    {22, GERDA_KIND_DUAL, 2, 1, 0, "D-EU868", "Dual EU 868 + ISM 2.4", 865275000u, 869575000u, 13, 2400400000u},
    {23, GERDA_KIND_DUAL, 3, 1, 0, "D-IN866", "Dual IN 866 + ISM 2.4", 865375000u, 866950000u, 4, 2400400000u},
    {24, GERDA_KIND_DUAL, 4, 1, 0, "D-AU433", "Dual AU 433 + ISM 2.4", 433420000u, 434420000u, 3, 2400400000u},
    {25, GERDA_KIND_DUAL, 5, 1, 0, "D-EU433", "Dual EU 433 + ISM 2.4", 433100000u, 434450000u, 3, 2400400000u},
    {26, GERDA_KIND_DUAL, 6, 1, 0, "D-US433", "Dual US 433 + ISM 2.4", 433250000u, 438000000u, 8, 2400400000u},
    {27, GERDA_KIND_DUAL, 7, 1, 0, "D-US433W", "Dual US 433 Wide + ISM 2.4", 423500000u, 438000000u, 20, 2400400000u},
    {GERDA_BAND_CUST2640, GERDA_KIND_CUST2640, 0xFF, 1, 1,
        "CUST2640", "CUSTOM_2640",
        2600400000u, 2679400000u, 80, 2600400000u},
};

const gerda_band_info_t *gerda_band_catalog(unsigned *count)
{
    if (count) {
        *count = sizeof(kCatalog) / sizeof(kCatalog[0]);
    }
    return kCatalog;
}

const gerda_band_info_t *gerda_band_info(uint8_t id)
{
    const unsigned n = sizeof(kCatalog) / sizeof(kCatalog[0]);
    for (unsigned i = 0; i < n; i++) {
        if (kCatalog[i].id == id) {
            return &kCatalog[i];
        }
    }
    return 0;
}

const char *gerda_band_code(uint8_t id)
{
    const gerda_band_info_t *info = gerda_band_info(id);
    return info ? info->code : "ISM2G4";
}

bool gerda_band_select(uint8_t id, uint8_t *domain, uint8_t *g2)
{
    if (!domain || !g2) {
        return false;
    }
    uint8_t d = (*domain > 7) ? 0 : *domain;
    const gerda_band_info_t *info = gerda_band_info(id);
    if (!info || !info->runtime_ok) {
        return false;
    }
    if (info->kind == GERDA_KIND_SUBGHZ || info->kind == GERDA_KIND_DUAL) {
        d = info->subghz_index;
        *g2 = 0;
    } else if (info->kind == GERDA_KIND_CUST2640) {
        *g2 = 1;
    } else if (info->kind == GERDA_KIND_ISM24) {
        *g2 = 0;
    } else {
        return false;
    }
    *domain = d;
    return true;
}

void gerda_band_import(const gerda_band_opts_t *in, uint8_t *domain, uint8_t *g2, uint8_t *selected)
{
    uint8_t d = 0;
    uint8_t g = 0;
    if (in) {
        d = (in->domain > 7) ? 0 : in->domain;
        g = (in->gerda_2g4 == 1) ? 1 : 0;
    }
    if (!in || !in->has_gerda_band) {
        // v0.1.1 and earlier: domain is the 900 MHz table index, not the 2.4 band.
        if (domain) *domain = d;
        if (g2) *g2 = g;
        if (selected) *selected = g ? (uint8_t)GERDA_BAND_CUST2640 : (uint8_t)GERDA_BAND_ISM24;
        return;
    }
    uint8_t sel = in->gerda_band;
    if (!gerda_band_select(sel, &d, &g)) {
        sel = g ? (uint8_t)GERDA_BAND_CUST2640 : (uint8_t)GERDA_BAND_ISM24;
        gerda_band_select(sel, &d, &g);
    }
    if (domain) *domain = d;
    if (g2) *g2 = g;
    if (selected) *selected = sel;
}

bool gerda_band_export(uint8_t id, uint8_t domain_inout, gerda_band_opts_t *out)
{
    if (!out) {
        return false;
    }
    uint8_t d = domain_inout;
    uint8_t g = 0;
    if (!gerda_band_select(id, &d, &g)) {
        return false;
    }
    out->domain = d;
    out->gerda_2g4 = g;
    out->gerda_band = id;
    out->has_gerda_band = 1;
    return true;
}
