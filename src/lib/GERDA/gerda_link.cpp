#include "gerda_link.h"

// Rate/tlm numbers must match expresslrs_RFrates_e / expresslrs_tlm_ratio_e
// in include/common.h. Kept numeric so this lib is not pulled into native
// tests via PlatformIO LDF on config.h / radio headers.
#define GERDA_RATE_2G4_50HZ  21  // RATE_LORA_2G4_50HZ (lowest shared SX1280↔LR1121 LoRa)
#define GERDA_RATE_2G4_500HZ 29  // RATE_LORA_2G4_500HZ
#define GERDA_TLM_STD        0   // TLM_RATIO_STD
#define GERDA_TLM_NO_TLM     1   // TLM_RATIO_NO_TLM
#define GERDA_TLM_1_64       3   // TLM_RATIO_1_64
#define GERDA_TLM_1_32       4   // TLM_RATIO_1_32
#define GERDA_TLM_1_16       5   // TLM_RATIO_1_16
#define GERDA_TLM_DISARMED   9   // TLM_RATIO_DISARMED

uint8_t gerda_profile = GERDA_PROFILE_BALANCE;

static uint8_t s_tlm_backoff; // 0 off, 1 mild, 2 deep

int gerda_link_feature_enabled(enum gerda_link_feature feature)
{
    switch (feature) {
    case GERDA_LINK_CC_PRIORITY:
    case GERDA_LINK_IA_FHSS:
        return 1;
    case GERDA_LINK_ADAPTIVE_MCS:
    case GERDA_LINK_INTERPACKET_FEC:
    default:
        return 0; // XOR FEC exists (gerda_fec) but is not on the air (50 Hz TOA)
    }
}

int gerda_profile_rate_tlm(uint8_t profile, uint8_t *rate_enum, uint8_t *tlm)
{
    if (!rate_enum || !tlm) {
        return 0;
    }
    if (profile == GERDA_PROFILE_RANGE) {
        *rate_enum = GERDA_RATE_2G4_50HZ;
        *tlm = GERDA_TLM_1_16;
        return 1;
    }
    if (profile == GERDA_PROFILE_SPEED) {
        *rate_enum = GERDA_RATE_2G4_500HZ;
        *tlm = GERDA_TLM_1_64;
        return 1;
    }
    return 0;
}

const char *gerda_profile_name_ru(uint8_t profile)
{
    if (profile == GERDA_PROFILE_RANGE) {
        return "Дальность";
    }
    if (profile == GERDA_PROFILE_SPEED) {
        return "Скорость";
    }
    return "Баланс";
}

uint8_t gerda_dynpower_lq_boost_min(void)
{
    // Stock ELRS: 50. RANGE boosts to configured max sooner so the link is
    // less likely to sag at the edge before a step-up can land.
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 80 : 50;
}

uint8_t gerda_dynpower_lq_thresh_dn(void)
{
    // Stock ELRS: 95. RANGE only drops power when LQ is essentially full.
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 99 : 95;
}

uint8_t gerda_dynpower_lq_thresh_up(void)
{
    // Stock ELRS: 85. RANGE steps power up while LQ is still "pretty good".
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 90 : 85;
}

uint8_t gerda_dynpower_lq_drop_boost(void)
{
    // Stock ELRS: LQ drop of 20 → instant max. RANGE reacts to a smaller fade.
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 15 : 20;
}

int gerda_should_defer_msp(uint8_t uplink_lq)
{
    if (uplink_lq == 0) {
        return 0;
    }
    if (gerda_profile == GERDA_PROFILE_SPEED) {
        return 0;
    }
    // RANGE: hold RC as soon as LQ is mediocre. Баланс: only in a real fade.
    uint8_t thresh = (gerda_profile == GERDA_PROFILE_RANGE) ? 70 : 40;
    return uplink_lq < thresh;
}

void gerda_tlm_backoff_reset(void)
{
    s_tlm_backoff = 0;
}

uint8_t gerda_tlm_backoff_level(void)
{
    return s_tlm_backoff;
}

static uint8_t sparsify_tlm(uint8_t configured, uint8_t candidate)
{
    // Enum 2..8: smaller value = rarer TLM. Never denser than configured.
    // Floor 1:64 so dynpower still sees some packets.
    if (candidate < GERDA_TLM_1_64) {
        candidate = GERDA_TLM_1_64;
    }
    if (candidate < configured) {
        return candidate;
    }
    return configured;
}

uint8_t gerda_tlm_backoff_ratio(uint8_t configured_tlm, uint8_t uplink_lq, int8_t snr_db)
{
    if (gerda_profile != GERDA_PROFILE_RANGE) {
        s_tlm_backoff = 0;
        return configured_tlm;
    }
    if (configured_tlm <= GERDA_TLM_NO_TLM || configured_tlm >= GERDA_TLM_DISARMED) {
        return configured_tlm;
    }
    if (uplink_lq == 0) {
        return configured_tlm; // no TLM yet — keep boot 1:16 so the link can form
    }

    if (s_tlm_backoff == 0) {
        if (uplink_lq < 70 || (uplink_lq < 85 && snr_db < 0)) {
            s_tlm_backoff = 1;
        }
    } else if (uplink_lq > 85 && snr_db >= 0) {
        s_tlm_backoff = 0;
    } else if (uplink_lq < 50 || (uplink_lq < 70 && snr_db < -3)) {
        s_tlm_backoff = 2;
    } else {
        s_tlm_backoff = 1;
    }

    if (s_tlm_backoff == 0) {
        return configured_tlm;
    }
    uint8_t candidate = (s_tlm_backoff >= 2) ? GERDA_TLM_1_64 : GERDA_TLM_1_32;
    return sparsify_tlm(configured_tlm, candidate);
}
