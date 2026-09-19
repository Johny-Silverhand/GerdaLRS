#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Stronger link security than stock bind-phrase UID (design only).
//
// Stock ExpressLRS (this tree):
//   - Binding phrase -> MD5 -> 6-byte UID
//   - UID mixes into FHSS seed / IQ invert / sync fields
//   - Sync packets check only part of the UID
//   - OTA payload is not authenticated; a matching UID is enough to inject
//
// Planned GerdaLRS (NOT implemented — do not claim the link is "secure"):
//   1. Post-bind session key via KDF
//        session_key = HKDF-SHA256(ikm=UID || bind_nonce || "GerdaLRS v1",
//                                  salt=commit_id, info="ota-session")
//      UID remains the pairing secret; session_key is ephemeral per bind.
//   2. Packet auth / HMAC
//        truncated HMAC (e.g. HMAC-SHA256 -> 32–64 bits) over
//        {nonce, packet_type, payload} using session_key.
//      Trade-off: airtime vs forgery resistance. 32 bits is a start, not a
//      substitute for a full AEAD.
//   3. Anti-replay
//        monotonic packet counter / OtaNonce window; reject duplicates and
//        old counters after a bind.
//
// Stubs: gerda_kdf_session_key / gerda_hmac_tag / gerda_replay_check always
// return GERDA_SEC_NOT_IMPLEMENTED. gerda_on_uid_ready() is called from
// rx_main / tx_main after UID setup and does NOT alter OTA packets.
//
// TODOs (next PRs, in order):
//   [ ] Specify exact OTA field layout without breaking CRSF packet size
//   [ ] Implement HKDF on ESP32 (mbedTLS) and a portable fallback
//   [ ] Add HMAC verify on RX before channel unpack
//   [ ] Counter window + bind-time nonce exchange
//   [ ] Do NOT ship a "secure mode" UI toggle until 1–4 land

#ifdef __cplusplus
extern "C" {
#endif

enum gerda_security_status {
    GERDA_SEC_STUB = 0,
    GERDA_SEC_NOT_IMPLEMENTED = -1,
};

static inline int gerda_kdf_session_key(const uint8_t *uid, size_t uid_len,
                                        const uint8_t *bind_nonce, size_t nonce_len,
                                        uint8_t *out_key, size_t out_len)
{
    (void)uid;
    (void)uid_len;
    (void)bind_nonce;
    (void)nonce_len;
    if (out_key && out_len) {
        memset(out_key, 0, out_len);
    }
    return GERDA_SEC_NOT_IMPLEMENTED;
}

static inline int gerda_hmac_tag(const uint8_t *session_key, size_t key_len,
                                 const uint8_t *msg, size_t msg_len,
                                 uint8_t *out_tag, size_t tag_len)
{
    (void)session_key;
    (void)key_len;
    (void)msg;
    (void)msg_len;
    if (out_tag && tag_len) {
        memset(out_tag, 0, tag_len);
    }
    return GERDA_SEC_NOT_IMPLEMENTED;
}

static inline int gerda_replay_check(uint32_t counter)
{
    (void)counter;
    return GERDA_SEC_NOT_IMPLEMENTED;
}

#ifdef __cplusplus
}
#endif

// C++ hook after UID is known (bind / boot). Does not alter OTA packets.
void gerda_on_uid_ready(const uint8_t *uid, size_t uid_len);
