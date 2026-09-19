#include "gerda_link.h"
#include "common.h"

#if defined(TARGET_TX)
#include "config.h"
#endif

uint8_t gerda_profile = GERDA_PROFILE_BALANCE;

int gerda_link_feature_enabled(enum gerda_link_feature feature)
{
    switch (feature) {
    case GERDA_LINK_CC_PRIORITY:
        return 1;
    case GERDA_LINK_ADAPTIVE_MCS:
    case GERDA_LINK_IA_FHSS:
    case GERDA_LINK_INTERPACKET_FEC:
    default:
        return 0;
    }
}

int gerda_profile_rate_tlm(uint8_t profile, uint8_t *rate_enum, uint8_t *tlm)
{
    if (!rate_enum || !tlm) {
        return 0;
    }
    if (profile == GERDA_PROFILE_RANGE) {
        *rate_enum = (uint8_t)RATE_LORA_2G4_50HZ;
        *tlm = (uint8_t)TLM_RATIO_1_16;
        return 1;
    }
    if (profile == GERDA_PROFILE_SPEED) {
        *rate_enum = (uint8_t)RATE_LORA_2G4_500HZ;
        *tlm = (uint8_t)TLM_RATIO_1_64;
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
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 70 : 50;
}

uint8_t gerda_dynpower_lq_thresh_dn(void)
{
    // Stock ELRS: 95. RANGE only drops power when LQ is essentially full.
    return (gerda_profile == GERDA_PROFILE_RANGE) ? 99 : 95;
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

#if defined(TARGET_TX)
void gerda_link_apply_tx_config(void)
{
    uint8_t rate_enum = 0;
    uint8_t tlm = 0;
    if (!gerda_profile_rate_tlm(gerda_profile, &rate_enum, &tlm)) {
        return;
    }
    config.SetRate(enumRatetoIndex((expresslrs_RFrates_e)rate_enum));
    config.SetTlm(tlm);
}
#endif
