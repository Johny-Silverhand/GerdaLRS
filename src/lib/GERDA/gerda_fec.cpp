#include "gerda_fec.h"
#include <string.h>

static uint8_t s_prev[GERDA_FEC_MAX_LEN];
static uint8_t s_parity[GERDA_FEC_MAX_LEN];
static size_t s_len;
static uint8_t s_has_prev;
static uint8_t s_parity_ready;
static uint32_t s_recoveries;

void gerda_fec_xor(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t n)
{
    if (!a || !b || !out || n == 0) {
        return;
    }
    for (size_t i = 0; i < n; i++) {
        out[i] = (uint8_t)(a[i] ^ b[i]);
    }
}

int gerda_fec_recover(const uint8_t *good, const uint8_t *parity, uint8_t *missing, size_t n)
{
    if (!good || !parity || !missing || n == 0 || n > GERDA_FEC_MAX_LEN) {
        return 0;
    }
    gerda_fec_xor(good, parity, missing, n);
    if (s_recoveries < 0xFFFFFFFFu) {
        s_recoveries++;
    }
    return 1;
}

void gerda_fec_window_reset(void)
{
    memset(s_prev, 0, sizeof(s_prev));
    memset(s_parity, 0, sizeof(s_parity));
    s_len = 0;
    s_has_prev = 0;
    s_parity_ready = 0;
}

void gerda_fec_window_push(const uint8_t *pkt, size_t n)
{
    if (!pkt || n == 0 || n > GERDA_FEC_MAX_LEN) {
        return;
    }
    if (!s_has_prev) {
        memcpy(s_prev, pkt, n);
        s_len = n;
        s_has_prev = 1;
        s_parity_ready = 0;
        return;
    }
    // Pair complete: parity = prev XOR current, then current becomes prev
    // for the next pair (systematic even/odd grouping).
    if (n != s_len) {
        memcpy(s_prev, pkt, n);
        s_len = n;
        s_parity_ready = 0;
        return;
    }
    gerda_fec_xor(s_prev, pkt, s_parity, n);
    s_parity_ready = 1;
    memcpy(s_prev, pkt, n);
}

int gerda_fec_parity_ready(void)
{
    return s_parity_ready ? 1 : 0;
}

const uint8_t *gerda_fec_parity(void)
{
    return s_parity_ready ? s_parity : 0;
}

size_t gerda_fec_parity_len(void)
{
    return s_parity_ready ? s_len : 0;
}

uint32_t gerda_fec_recovery_count(void)
{
    return s_recoveries;
}

void gerda_fec_recovery_reset(void)
{
    s_recoveries = 0;
}
