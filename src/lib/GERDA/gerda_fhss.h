#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Interference-aware FHSS (Web UI «Умный FHSS», options.json "gerda-fhss").
// Default OFF = stock ELRS hop sequence, no LQ/AFC tweaks.
//
// v1 does NOT rewrite FHSSsequence. TX and RX must keep the UID-seeded
// permutation in lockstep; skipping a hop on one side desyncs the link.
// When ON:
//   - per-channel EWMA of CRC-ok / CRC-fail / RSSI (from Radio.LastPacketRSSI)
//   - denylist of the worst channels (never the sync channel, cap 25%)
//   - RX/TX still hop there, but a miss on a denylisted hop is not counted
//     against LQ, and RX skips FreqCorrection on that hop (noisy AFC)
//
// Synced TX+RX skip-map would need a shared denylist over TLM/MSP. Export/
// import helpers exist for a later PR; they are not on the air yet.
// See docs/FHSS.ru.md.

#ifdef __cplusplus
extern "C" {
#endif

#define GERDA_FHSS_MAX_CH 80
#define GERDA_FHSS_MIN_SAMPLES 8
#define GERDA_FHSS_DENY_SCORE 50  // miss rate % to be eligible

// 0 = OFF (stock). 1 = ON.
extern uint8_t gerda_smart_fhss;

void gerda_fhss_reset(uint8_t n_channels, uint8_t sync_ch);

// ok=1 CRC/HW success, 0 fail. rssi is dBm (typically negative); ignored on fail.
void gerda_fhss_record(uint8_t ch, int ok, int8_t rssi);

int gerda_fhss_is_denylisted(uint8_t ch);

// 1 → caller should LQCalc.add() so this miss does not lower LQ.
int gerda_fhss_neutralize_miss(uint8_t ch);

// 1 → skip HandleFreqCorr for this hop (RX).
int gerda_fhss_skip_freq_corr(uint8_t ch);

uint8_t gerda_fhss_denylist_count(void);
uint8_t gerda_fhss_channel_count(void);
uint8_t gerda_fhss_sync_channel(void);
uint8_t gerda_fhss_max_denylist(uint8_t n_channels);

// Future synced map (not sent on air in this PR).
uint8_t gerda_fhss_export_denylist(uint8_t *out, uint8_t max);
void gerda_fhss_import_denylist(const uint8_t *in, uint8_t n);

// Test/debug: miss rate 0–100, or 255 if too few samples.
uint8_t gerda_fhss_miss_rate(uint8_t ch);

#ifdef __cplusplus
}
#endif
