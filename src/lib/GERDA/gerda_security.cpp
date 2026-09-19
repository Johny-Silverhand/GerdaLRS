#include "gerda_security.h"
#include "gerda_sha256.h"
#include "gerda_config.h"

#include <string.h>

#if defined(GERDA_SECURE_LINK_DEFAULT)
uint8_t gerda_secure_link = 1;
#else
uint8_t gerda_secure_link = 0;
#endif

static uint8_t s_session[GERDA_SESSION_KEY_LEN];
static uint8_t s_key_ready;
static uint8_t s_replay_inited;
static uint8_t s_replay_last;
static uint32_t s_replay_bits;

static const uint8_t kSalt[] = "GerdaLRS-ota-v1";
static const uint8_t kInfo[] = "ota-mac-v1";

int gerda_ct_equal(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t d = 0;
    if (!a || !b) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        d |= (uint8_t)(a[i] ^ b[i]);
    }
    return d == 0;
}

int gerda_hmac_sha256(const uint8_t *key, size_t key_len,
                      const uint8_t *msg, size_t msg_len,
                      uint8_t out[32])
{
    uint8_t kpad[64];
    uint8_t khash[32];
    const uint8_t *k = key;
    size_t klen = key_len;

    if (!out || (msg_len && !msg)) {
        return GERDA_SEC_BAD_ARG;
    }
    if (!k) {
        k = khash;
        klen = 0;
    }
    if (klen > 64) {
        gerda_sha256(k, klen, khash);
        k = khash;
        klen = 32;
    }
    memset(kpad, 0, 64);
    if (klen) {
        memcpy(kpad, k, klen);
    }
    uint8_t ipad[64];
    uint8_t opad[64];
    for (int i = 0; i < 64; i++) {
        ipad[i] = (uint8_t)(kpad[i] ^ 0x36);
        opad[i] = (uint8_t)(kpad[i] ^ 0x5c);
    }

    // inner = SHA256(ipad || msg)
    // Use a small stack buffer for short messages; otherwise two-pass via temp.
    uint8_t inner_in[64 + 64];
    uint8_t inner[32];
    if (msg_len <= 64) {
        memcpy(inner_in, ipad, 64);
        if (msg_len && msg) {
            memcpy(inner_in + 64, msg, msg_len);
        }
        gerda_sha256(inner_in, 64 + msg_len, inner);
    } else {
        // Streaming not implemented: allocate on heap-less path with block hashing
        // by concatenating in a VLA (C++ native/ESP stack is fine for < 256B OTA).
        uint8_t buf[64 + 256];
        if (msg_len > 256) {
            return GERDA_SEC_BAD_ARG;
        }
        memcpy(buf, ipad, 64);
        memcpy(buf + 64, msg, msg_len);
        gerda_sha256(buf, 64 + msg_len, inner);
    }

    uint8_t outer[64 + 32];
    memcpy(outer, opad, 64);
    memcpy(outer + 64, inner, 32);
    gerda_sha256(outer, 96, out);
    return GERDA_SEC_OK;
}

int gerda_hkdf_sha256(const uint8_t *ikm, size_t ikm_len,
                      const uint8_t *salt, size_t salt_len,
                      const uint8_t *info, size_t info_len,
                      uint8_t *out, size_t out_len)
{
    uint8_t prk[32];
    uint8_t zeros[32];
    const uint8_t *s = salt;
    size_t sl = salt_len;
    if (!ikm || !out || out_len == 0 || out_len > 32) {
        return GERDA_SEC_BAD_ARG;
    }
    if (!s || sl == 0) {
        memset(zeros, 0, 32);
        s = zeros;
        sl = 32;
    }
    if (gerda_hmac_sha256(s, sl, ikm, ikm_len, prk) != GERDA_SEC_OK) {
        return GERDA_SEC_BAD_ARG;
    }
    uint8_t msg[80];
    size_t mlen = 0;
    if (info && info_len) {
        if (info_len > 64) {
            return GERDA_SEC_BAD_ARG;
        }
        memcpy(msg, info, info_len);
        mlen = info_len;
    }
    msg[mlen++] = 0x01;
    uint8_t t[32];
    if (gerda_hmac_sha256(prk, 32, msg, mlen, t) != GERDA_SEC_OK) {
        return GERDA_SEC_BAD_ARG;
    }
    memcpy(out, t, out_len);
    return GERDA_SEC_OK;
}

int gerda_kdf_session_key(const uint8_t *uid, size_t uid_len,
                          const uint8_t *phrase, size_t phrase_len,
                          uint8_t *out_key, size_t out_len)
{
    uint8_t ikm[80];
    if (!uid || !out_key || uid_len == 0 || uid_len > 32 || out_len == 0 || out_len > 32) {
        return GERDA_SEC_BAD_ARG;
    }
    if (phrase_len > 48) {
        return GERDA_SEC_BAD_ARG;
    }
    memcpy(ikm, uid, uid_len);
    if (phrase && phrase_len) {
        memcpy(ikm + uid_len, phrase, phrase_len);
    }
    return gerda_hkdf_sha256(ikm, uid_len + phrase_len,
                             kSalt, sizeof(kSalt) - 1,
                             kInfo, sizeof(kInfo) - 1,
                             out_key, out_len);
}

int gerda_hmac_tag(const uint8_t *session_key, size_t key_len,
                   const uint8_t *msg, size_t msg_len,
                   uint8_t *out_tag, size_t tag_len)
{
    uint8_t full[32];
    if (!out_tag || tag_len == 0) {
        return GERDA_SEC_BAD_ARG;
    }
    int rc = gerda_hmac_sha256(session_key, key_len, msg, msg_len, full);
    if (rc != GERDA_SEC_OK) {
        return rc;
    }
    if (tag_len > 32) {
        tag_len = 32;
    }
    memcpy(out_tag, full, tag_len);
    return GERDA_SEC_OK;
}

void gerda_replay_reset(uint8_t nonce)
{
    s_replay_inited = 1;
    s_replay_last = nonce;
    s_replay_bits = 1u;
}

int gerda_replay_accept(uint8_t nonce)
{
    if (!s_replay_inited) {
        gerda_replay_reset(nonce);
        return GERDA_SEC_OK;
    }
    uint8_t diff = (uint8_t)(nonce - s_replay_last);
    if (diff == 0) {
        return GERDA_SEC_REPLAY;
    }
    if (diff < 128) {
        // nonce is ahead (including wrap that looks forward within 127)
        if (diff >= GERDA_REPLAY_WINDOW) {
            gerda_replay_reset(nonce);
            return GERDA_SEC_OK;
        }
        s_replay_bits <<= diff;
        s_replay_bits |= 1u;
        s_replay_last = nonce;
        return GERDA_SEC_OK;
    }
    uint8_t back = (uint8_t)(s_replay_last - nonce);
    if (back >= GERDA_REPLAY_WINDOW) {
        return GERDA_SEC_REPLAY;
    }
    uint32_t mask = 1u << back;
    if (s_replay_bits & mask) {
        return GERDA_SEC_REPLAY;
    }
    s_replay_bits |= mask;
    return GERDA_SEC_OK;
}

bool gerda_key_ready(void)
{
    return s_key_ready != 0;
}

bool gerda_secure_active(void)
{
    return gerda_secure_link != 0 && s_key_ready != 0;
}

static void mac_input(const uint8_t *pkt, uint8_t pkt_len, uint8_t nonce, uint8_t *msg, size_t *mlen)
{
    uint8_t tmp[16];
    memset(tmp, 0, sizeof(tmp));
    if (pkt_len > 16) {
        pkt_len = 16;
    }
    memcpy(tmp, pkt, pkt_len);
    if (pkt_len == 8) {
        tmp[0] &= 0x03; // keep type, zero crcHigh
        tmp[7] = 0;
    } else if (pkt_len >= 2) {
        tmp[pkt_len - 2] = 0;
        tmp[pkt_len - 1] = 0;
    }
    memcpy(msg, tmp, pkt_len);
    msg[pkt_len] = nonce;
    msg[pkt_len + 1] = pkt_len;
    *mlen = (size_t)pkt_len + 2;
}

void gerda_ota_mac_xor(uint8_t *pkt, uint8_t pkt_len, uint8_t nonce)
{
    if (!pkt || !gerda_secure_active()) {
        return;
    }
    uint8_t msg[20];
    size_t mlen = 0;
    uint8_t tag[32];
    mac_input(pkt, pkt_len, nonce, msg, &mlen);
    if (gerda_hmac_tag(s_session, GERDA_SESSION_KEY_LEN, msg, mlen, tag, 2) != GERDA_SEC_OK) {
        return;
    }
    if (pkt_len == 8) {
        uint16_t mac14 = (uint16_t)(((uint16_t)tag[0] << 8) | tag[1]) & 0x3FFFu;
        pkt[0] ^= (uint8_t)((mac14 >> 8) << 2); // crcHigh bits 2..7; type bits 0..1 unchanged
        pkt[7] ^= (uint8_t)mac14;
    } else if (pkt_len >= 2) {
        pkt[pkt_len - 2] ^= tag[0];
        pkt[pkt_len - 1] ^= tag[1];
    }
}

uint8_t gerda_ota_nonce_for_packet(const uint8_t *pkt, uint8_t pkt_len, uint8_t local_nonce)
{
    // PACKET_TYPE_SYNC = 0b10. OTA_Sync_s.nonce sits at byte 2 for both OTA4 and OTA8.
    if (pkt && pkt_len >= 3 && ((pkt[0] & 0x03) == 0x02)) {
        return pkt[2];
    }
    return local_nonce;
}

void gerda_ota_apply_mac(uint8_t *pkt, uint8_t pkt_len, uint8_t nonce, int in_bind)
{
    if (in_bind) {
        return;
    }
    gerda_ota_mac_xor(pkt, pkt_len, nonce);
}

void gerda_on_uid_ready(const uint8_t *uid, size_t uid_len)
{
    s_key_ready = 0;
    memset(s_session, 0, sizeof(s_session));
    s_replay_inited = 0;
    if (!uid || uid_len == 0) {
        return;
    }
    if (gerda_kdf_session_key(uid, uid_len, 0, 0, s_session, sizeof(s_session)) == GERDA_SEC_OK) {
        s_key_ready = 1;
    }
}
