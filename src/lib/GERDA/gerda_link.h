#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Range / speed layer for the v1 pair (SX1280 TX Ranger Nano + LR1121 RX FlyFish).
// Cross-chip: standard 2.4 LoRa rates only. DK500/K1000 are out of scope for this TX.
//
// Implemented:
//   - Flight profiles (Web UI «Профиль полёта», options.json "gerda-profile"):
//       0 Баланс  — do not override Lua Packet Rate / Telem Ratio
//       1 Дальность — 50 Hz + Telem 1:16, earlier dynpower boost, MSP defer under low LQ
//       2 Скорость  — 500 Hz + Telem 1:64 (still standard LoRa 2.4, not FLRC/FSK)
//   - Control-channel priority: skip MSP uplink when LQ is poor (tx_main SendRCdataToRF)
//   - Dynpower: RANGE raises the absolute LQ boost-to-max threshold (50 → 70) and
//     makes power-down require LQ 99 instead of 95. Stock path unchanged on Баланс.
//
// Not implemented (Phase 4 — no fake hooks):
//   - Interference-aware FHSS: FHSS.cpp has no per-hop RSSI histogram; hop set is
//     a seeded permutation. Do not shrink it. See docs/ARCHITECTURE.ru.md.
//   - Adaptive MCS / rate switching from live LQ (would desync SX1280↔LR1121)
//   - Inter-packet FEC
//   - Telem-slot steal (ExpressLRS_currTlmDenom is synced via SYNC; changing it
//     mid-flight without a syncspam is not a clean mixer change)
//
// CUSTOM_2640 does not improve range on this hardware (RF matching ~2.4–2.5 GHz).

#ifdef __cplusplus
extern "C" {
#endif

enum gerda_flight_profile {
    GERDA_PROFILE_BALANCE = 0,
    GERDA_PROFILE_RANGE = 1,
    GERDA_PROFILE_SPEED = 2,
};

enum gerda_link_feature {
    GERDA_LINK_ADAPTIVE_MCS = 0,
    GERDA_LINK_IA_FHSS = 1,
    GERDA_LINK_CC_PRIORITY = 2,
    GERDA_LINK_INTERPACKET_FEC = 3,
};

// 0 = Баланс, 1 = Дальность, 2 = Скорость. Default 0 (Lua/EEPROM owns rate).
extern uint8_t gerda_profile;

int gerda_link_feature_enabled(enum gerda_link_feature feature);

// If profile recommends a rate/tlm override, write RATE_LORA_2G4_* and TLM_RATIO_*
// and return 1. Баланс returns 0 and leaves *rate_enum / *tlm untouched.
int gerda_profile_rate_tlm(uint8_t profile, uint8_t *rate_enum, uint8_t *tlm);

const char *gerda_profile_name_ru(uint8_t profile);

// Dynpower vs stock ELRS (dynpower.cpp DYNPOWER_LQ_BOOST_THRESH_MIN = 50,
// DYNPOWER_LQ_THRESH_DN = 95). RANGE holds max power sooner at the edge.
uint8_t gerda_dynpower_lq_boost_min(void);
uint8_t gerda_dynpower_lq_thresh_dn(void);

// Defer MSP/airport-unrelated uplink so RC occupies the slot.
// uplink_lq == 0 means "no TLM yet" — do not defer (would starve bind/config).
int gerda_should_defer_msp(uint8_t uplink_lq);

#ifdef __cplusplus
}
#endif

#if defined(TARGET_TX)
// After TxConfig::Load(), before ChangeRadioParams(). No-op for Баланс.
// Implemented in tx_main.cpp so GERDA lib does not LDF-pull CONFIG on native tests.
void gerda_link_apply_tx_config(void);
#endif
