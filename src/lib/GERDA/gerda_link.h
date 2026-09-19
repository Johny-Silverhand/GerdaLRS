#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Range / speed layer for the v1 pair (SX1280 TX Ranger Nano + LR1121 RX FlyFish).
// Cross-chip: standard 2.4 LoRa rates only. DK500/K1000 are out of scope for this TX.
// Lowest shared LoRa rate on both chips is 50 Hz (no 2.4 25 Hz table on SX1280).
//
// Implemented:
//   - Flight profiles (Web UI «Профиль полёта», options.json "gerda-profile"):
//       0 Баланс  — do not override Lua Packet Rate / Telem Ratio
//       1 Дальность — 50 Hz + Telem 1:16, sticky dynpower, MSP defer, TLM backoff
//       2 Скорость  — 500 Hz + Telem 1:64 (still standard LoRa 2.4, not FLRC/FSK)
//   - Control-channel priority: skip MSP uplink when LQ is poor (tx_main SendRCdataToRF)
//   - Dynpower RANGE: boost-to-max at LQ 80 (stock 50), power-down only at LQ 99
//     (stock 95), LQ step-up at 90 (stock 85), drop-boost 15 (stock 20)
//   - Telem backoff (RANGE only): when LQ/SNR poor, SYNC newTlmRatio 1:32 then 1:64
//     so RC occupies more slots. Never NO_TLM, never overrides MSP 1:2 boost.
//
// Not implemented (honest):
//   - Adaptive MCS / rate switching from live LQ (would desync SX1280↔LR1121)
//   - On-air inter-packet FEC: 50 Hz TOA ~10.8 ms in a 20 ms slot; DVDA does not
//     fit. XOR primitive lives in gerda_fec.* for a later parity packet.
//
// CUSTOM_2640 does not improve range on this hardware (RF matching ~2.4–2.5 GHz).
// Secure Link is independent and must stay OFF for max-range flight tests.

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
// DYNPOWER_LQ_THRESH_DN = 95, DYNPOWER_LQ_THRESH_UP = 85,
// DYNPOWER_LQ_BOOST_THRESH_DIFF = 20). RANGE holds max power sooner at the edge.
uint8_t gerda_dynpower_lq_boost_min(void);
uint8_t gerda_dynpower_lq_thresh_dn(void);
uint8_t gerda_dynpower_lq_thresh_up(void);
uint8_t gerda_dynpower_lq_drop_boost(void);

// Defer MSP/airport-unrelated uplink so RC occupies the slot.
// uplink_lq == 0 means "no TLM yet" — do not defer (would starve bind/config).
int gerda_should_defer_msp(uint8_t uplink_lq);

// RANGE-only: sparsify configured TLM_RATIO_* when uplink LQ/SNR is poor.
// Hysteresis enter LQ<70 (or LQ<85 and SNR<0 dB), deep LQ<50, exit LQ>85.
// Never returns NO_TLM / STD / DISARMED. Never denser than configured.
// Floor is 1:64 so dynpower still gets some TLM. uplink_lq==0 → no change.
uint8_t gerda_tlm_backoff_ratio(uint8_t configured_tlm, uint8_t uplink_lq, int8_t snr_db);
void gerda_tlm_backoff_reset(void);
uint8_t gerda_tlm_backoff_level(void); // 0 off, 1 = 1:32, 2 = 1:64

#ifdef __cplusplus
}
#endif

#if defined(TARGET_TX)
// After TxConfig::Load(), before ChangeRadioParams(). No-op for Баланс.
// Implemented in tx_main.cpp so GERDA lib does not LDF-pull CONFIG on native tests.
void gerda_link_apply_tx_config(void);
#endif
