#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Gerda Secure Link primitives + OTA MAC (XOR into ELRS CRC field).
// Default OFF. See docs/SECURITY.ru.md.
//
// Stock ELRS (Secure OFF): unkeyed CRC14/16, partial UID on SYNC.
// Secure ON: HKDF-SHA256 session key from UID (+ optional phrase),
// truncated HMAC XOR'd into the CRC field (0 extra airtime), anti-replay window.
// Bind mode does not apply the MAC (stock bind MSP).

#ifdef __cplusplus
extern "C" {
#endif

enum gerda_security_status {
    GERDA_SEC_OK = 0,
    GERDA_SEC_BAD_ARG = -1,
    GERDA_SEC_REPLAY = -2,
    GERDA_SEC_NO_KEY = -3,
};

#define GERDA_SESSION_KEY_LEN 16
#define GERDA_REPLAY_WINDOW 32

// 0 = OFF (stock ELRS interoperable). 1 = ON (Gerda↔Gerda only).
extern uint8_t gerda_secure_link;

void gerda_sha256(const uint8_t *data, size_t len, uint8_t out[32]);

int gerda_hmac_sha256(const uint8_t *key, size_t key_len,
                      const uint8_t *msg, size_t msg_len,
                      uint8_t out[32]);

// HKDF-SHA256 (RFC 5869). out_len up to 32 for v1.
int gerda_hkdf_sha256(const uint8_t *ikm, size_t ikm_len,
                      const uint8_t *salt, size_t salt_len,
                      const uint8_t *info, size_t info_len,
                      uint8_t *out, size_t out_len);

// session_key = HKDF(ikm=UID||phrase, salt="GerdaLRS-ota-v1", info="ota-mac-v1", 16)
int gerda_kdf_session_key(const uint8_t *uid, size_t uid_len,
                          const uint8_t *phrase, size_t phrase_len,
                          uint8_t *out_key, size_t out_len);

int gerda_hmac_tag(const uint8_t *session_key, size_t key_len,
                   const uint8_t *msg, size_t msg_len,
                   uint8_t *out_tag, size_t tag_len);

void gerda_replay_reset(uint8_t nonce);
int gerda_replay_accept(uint8_t nonce);

int gerda_ct_equal(const uint8_t *a, const uint8_t *b, size_t n);

bool gerda_key_ready(void);
bool gerda_secure_active(void);

// XOR truncated HMAC into CRC field (in-place). Involutive: call twice to undo.
// pkt_len 8 = OTA4 (CRC14 in crcHigh/crcLow), 13 = OTA8 (CRC16 LE at end).
void gerda_ota_mac_xor(uint8_t *pkt, uint8_t pkt_len, uint8_t nonce);

// SYNC uses pkt[2] (OTA_Sync_s.nonce); other types use local_nonce.
uint8_t gerda_ota_nonce_for_packet(const uint8_t *pkt, uint8_t pkt_len, uint8_t local_nonce);

// No-op in bind mode or when Secure is OFF / no key.
void gerda_ota_apply_mac(uint8_t *pkt, uint8_t pkt_len, uint8_t nonce, int in_bind);

// After UID is known. Derives session key from UID (phrase already folded into UID).
void gerda_on_uid_ready(const uint8_t *uid, size_t uid_len);

#ifdef __cplusplus
}
#endif
