#pragma once

#include "gerda_config.h"
#include <stdint.h>

// Runtime 2.4 GHz hop-set selection (ISM2G4 vs CUSTOM_2640).
// Stored in options.json as "gerda-2g4": 0 (ISM) or 1 (CUSTOM_2640).
// Not part of the packed firmware_options_t layout (avoids breaking ELRS magic).
// TX and RX MUST use the same value or the link will not connect.
extern uint8_t gerda_2g4_band;

static inline uint8_t gerda_2g4_band_index(void)
{
    return (gerda_2g4_band == 1) ? 1 : 0;
}

static inline const char *gerda_2g4_band_name(void)
{
    return gerda_2g4_band_index() ? "CUST2640" : "ISM2G4";
}

// Regulatory domain / band the user picked in the Web UI (options.json "gerda-band").
//
// ExpressLRS picks the sub-GHz hop table at runtime (firmwareOptions.domain, indexes
// 0..7) and the 2.4 table at compile time — except GerdaLRS already switches
// ISM2G4 / CUSTOM_2640 via gerda-2g4. Which physical radio path is on the air is
// still the packet rate (LR1121 900 / 2.4 / dual), not this dropdown.
//
// IDs:
//   0      ISM 2.4 (default). Does not rewrite the stored sub-GHz index.
//   1      CE/EU LBT 2.4. Same frequencies as ISM. LBT is compile-time only
//          (Regulatory_Domain_EU_CE_2400) and this tree rejects that flag on LR1121.
//          Selecting it does not change hops.
//   10..17 sub-GHz only: AU915, FCC915, EU868, IN866, AU433, EU433, US433, US433W.
//   20..27 dual-band: that sub-GHz table + ISM 2.4 (packet rates D150 / D100).
//   30     CUSTOM_2640. Kept available, factory default stays off.
//
// Legacy options (v0.1.1) have domain=1 (FCC915) and gerda-2g4=0 and no gerda-band.
// That domain index is the 900 MHz table used only when the packet rate is sub-GHz.
// Absent gerda-band therefore stays ISM 2.4 and does not retune the radio to 915.
extern uint8_t gerda_band;

enum {
    GERDA_BAND_ISM24 = 0,
    GERDA_BAND_CE_LBT = 1,
    GERDA_BAND_SUBGHZ_BASE = 10,
    GERDA_BAND_DUAL_BASE = 20,
    GERDA_BAND_CUST2640 = 30,
    GERDA_SUBGHZ_DOMAIN_COUNT = 8
};

enum gerda_band_kind_e : uint8_t {
    GERDA_KIND_ISM24 = 0,
    GERDA_KIND_CE_LBT = 1,
    GERDA_KIND_SUBGHZ = 2,
    GERDA_KIND_DUAL = 3,
    GERDA_KIND_CUST2640 = 4
};

struct gerda_band_info_t {
    uint8_t id;
    uint8_t kind;
    uint8_t subghz_index;   // 0..7, or 0xFF
    uint8_t runtime_ok;     // 0 = do not change hop tables
    uint8_t sx1280_choice;  // shown on SX1280 (2.4-only) builds
    const char *code;
    const char *label_ru;
    uint32_t freq_start_hz;
    uint32_t freq_stop_hz;
    uint16_t freq_count;
    uint32_t hz_24_start;   // 2.4 companion table, 0 if none
};

// Options triple persisted beside the existing "domain" and "gerda-2g4" keys.
struct gerda_band_opts_t {
    uint8_t domain;
    uint8_t gerda_2g4;
    uint8_t gerda_band;
    uint8_t has_gerda_band; // 0 = key absent (legacy firmware)
};

const gerda_band_info_t *gerda_band_catalog(unsigned *count);
const gerda_band_info_t *gerda_band_info(uint8_t id);
const char *gerda_band_code(uint8_t id);

// Apply a user selection. CE LBT and unknown ids return false and do not write.
// ISM and CUSTOM_2640 leave *domain as the stored sub-GHz index.
bool gerda_band_select(uint8_t id, uint8_t *domain, uint8_t *g2);

// Round-trip of the options.json fields. Import never yields CE LBT.
void gerda_band_import(const gerda_band_opts_t *in, uint8_t *domain, uint8_t *g2, uint8_t *selected);
bool gerda_band_export(uint8_t id, uint8_t domain_inout, gerda_band_opts_t *out);

#if defined(GERDA_DOMAIN_CUSTOM_2640)
#define GERDA_HAS_CUSTOM_2640 1
#else
#define GERDA_HAS_CUSTOM_2640 0
#endif
