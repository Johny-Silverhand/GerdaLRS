#pragma once

#include <stdint.h>
#include <stddef.h>

// Inter-packet XOR erasure coding for control payloads.
//
// Not sent on the air in this PR. 50 Hz LoRa on SX1280/LR1121 has TOA ~10.8 ms
// in a 20 ms slot (common.cpp RF perf index for RATE_LORA_2G4_50HZ). A second
// send (DVDA / numOfSends=2) does not fit, and a dedicated parity packet would
// steal an RC slot — the opposite of the Range-profile goal unless TLM is
// already being backed off and a later PR maps parity onto that spare.
//
// This module is the smallest compile-tested primitive: systematic XOR of two
// equal-length payloads (OTA4 8 B or OTA8 13 B). Given one good packet and the
// parity, recover the missing one. See docs/RANGE_EXPERIMENTS.ru.md.

#ifdef __cplusplus
extern "C" {
#endif

#define GERDA_FEC_MAX_LEN 13  // OTA8_PACKET_SIZE

void gerda_fec_xor(const uint8_t *a, const uint8_t *b, uint8_t *out, size_t n);

// missing = good XOR parity. Returns 1 on success, 0 on bad args.
int gerda_fec_recover(const uint8_t *good, const uint8_t *parity, uint8_t *missing, size_t n);

// Two-packet encoder window: push sequential RC payloads; after the second
// push, gerda_fec_parity_ready() is 1 and gerda_fec_parity() holds a XOR b.
void gerda_fec_window_reset(void);
void gerda_fec_window_push(const uint8_t *pkt, size_t n);
int gerda_fec_parity_ready(void);
const uint8_t *gerda_fec_parity(void);
size_t gerda_fec_parity_len(void);

uint32_t gerda_fec_recovery_count(void);
void gerda_fec_recovery_reset(void);

#ifdef __cplusplus
}
#endif
