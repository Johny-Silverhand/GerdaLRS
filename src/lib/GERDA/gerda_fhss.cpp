#include "gerda_fhss.h"
#include <string.h>

#if defined(GERDA_SMART_FHSS_DEFAULT)
uint8_t gerda_smart_fhss = 1;
#else
uint8_t gerda_smart_fhss = 0;
#endif

static uint8_t s_nch;
static uint8_t s_sync;
static uint8_t s_hits[GERDA_FHSS_MAX_CH];
static uint8_t s_miss[GERDA_FHSS_MAX_CH];
static int8_t s_rssi[GERDA_FHSS_MAX_CH];
static uint8_t s_deny[GERDA_FHSS_MAX_CH];
static uint8_t s_deny_n;
static uint8_t s_rssi_set[GERDA_FHSS_MAX_CH];

uint8_t gerda_fhss_max_denylist(uint8_t n_channels)
{
    if (n_channels < 8) {
        return 0; // too few hops to drop any without collapsing diversity
    }
    uint8_t cap = (uint8_t)(n_channels / 4); // 25%
    if (cap < 1) {
        cap = 1;
    }
    return cap;
}

static void rebuild_denylist(void)
{
    memset(s_deny, 0, sizeof(s_deny));
    s_deny_n = 0;
    if (!gerda_smart_fhss || s_nch < 8) {
        return;
    }
    uint8_t cap = gerda_fhss_max_denylist(s_nch);
    for (uint8_t round = 0; round < cap; round++) {
        int best = -1;
        uint8_t best_score = 0;
        for (uint8_t ch = 0; ch < s_nch; ch++) {
            if (ch == s_sync || s_deny[ch]) {
                continue;
            }
            uint16_t samples = (uint16_t)s_hits[ch] + s_miss[ch];
            if (samples < GERDA_FHSS_MIN_SAMPLES) {
                continue;
            }
            uint8_t rate = (uint8_t)((uint16_t)s_miss[ch] * 100u / samples);
            if (rate < GERDA_FHSS_DENY_SCORE) {
                continue;
            }
            if (rate > best_score) {
                best_score = rate;
                best = (int)ch;
            }
        }
        if (best < 0) {
            break;
        }
        s_deny[best] = 1;
        s_deny_n++;
    }
}

void gerda_fhss_reset(uint8_t n_channels, uint8_t sync_ch)
{
    if (n_channels == 0 || n_channels > GERDA_FHSS_MAX_CH) {
        n_channels = (n_channels > GERDA_FHSS_MAX_CH) ? GERDA_FHSS_MAX_CH : 0;
    }
    s_nch = n_channels;
    s_sync = (sync_ch < s_nch) ? sync_ch : 0;
    memset(s_hits, 0, sizeof(s_hits));
    memset(s_miss, 0, sizeof(s_miss));
    memset(s_rssi, 0, sizeof(s_rssi));
    memset(s_rssi_set, 0, sizeof(s_rssi_set));
    memset(s_deny, 0, sizeof(s_deny));
    s_deny_n = 0;
}

void gerda_fhss_record(uint8_t ch, int ok, int8_t rssi)
{
    if (s_nch == 0 || ch >= s_nch) {
        return;
    }
    if (s_hits[ch] == 255 || s_miss[ch] == 255) {
        s_hits[ch] = (uint8_t)(s_hits[ch] >> 1);
        s_miss[ch] = (uint8_t)(s_miss[ch] >> 1);
    }
    if (ok) {
        s_hits[ch]++;
        if (!s_rssi_set[ch]) {
            s_rssi[ch] = rssi;
            s_rssi_set[ch] = 1;
        } else {
            // 7/8 EWMA
            s_rssi[ch] = (int8_t)(((int16_t)s_rssi[ch] * 7 + rssi) / 8);
        }
    } else {
        s_miss[ch]++;
    }
    rebuild_denylist();
}

int gerda_fhss_is_denylisted(uint8_t ch)
{
    if (!gerda_smart_fhss || s_nch == 0 || ch >= s_nch) {
        return 0;
    }
    return s_deny[ch] ? 1 : 0;
}

int gerda_fhss_neutralize_miss(uint8_t ch)
{
    return gerda_fhss_is_denylisted(ch);
}

int gerda_fhss_skip_freq_corr(uint8_t ch)
{
    return gerda_fhss_is_denylisted(ch);
}

uint8_t gerda_fhss_denylist_count(void)
{
    return s_deny_n;
}

uint8_t gerda_fhss_channel_count(void)
{
    return s_nch;
}

uint8_t gerda_fhss_sync_channel(void)
{
    return s_sync;
}

uint8_t gerda_fhss_export_denylist(uint8_t *out, uint8_t max)
{
    uint8_t n = 0;
    if (!out || max == 0) {
        return 0;
    }
    for (uint8_t ch = 0; ch < s_nch && n < max; ch++) {
        if (s_deny[ch]) {
            out[n++] = ch;
        }
    }
    return n;
}

void gerda_fhss_import_denylist(const uint8_t *in, uint8_t n)
{
    memset(s_deny, 0, sizeof(s_deny));
    s_deny_n = 0;
    if (!in || !gerda_smart_fhss) {
        return;
    }
    uint8_t cap = gerda_fhss_max_denylist(s_nch);
    for (uint8_t i = 0; i < n && s_deny_n < cap; i++) {
        uint8_t ch = in[i];
        if (ch < s_nch && ch != s_sync && !s_deny[ch]) {
            s_deny[ch] = 1;
            s_deny_n++;
        }
    }
}

uint8_t gerda_fhss_miss_rate(uint8_t ch)
{
    if (ch >= s_nch) {
        return 255;
    }
    uint16_t samples = (uint16_t)s_hits[ch] + s_miss[ch];
    if (samples < GERDA_FHSS_MIN_SAMPLES) {
        return 255;
    }
    return (uint8_t)((uint16_t)s_miss[ch] * 100u / samples);
}

uint8_t gerda_fhss_hits(uint8_t ch)
{
    if (ch >= s_nch) {
        return 0;
    }
    return s_hits[ch];
}

uint8_t gerda_fhss_misses(uint8_t ch)
{
    if (ch >= s_nch) {
        return 0;
    }
    return s_miss[ch];
}
