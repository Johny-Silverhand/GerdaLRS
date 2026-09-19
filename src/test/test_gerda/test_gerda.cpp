#include <cstdint>
#include <cstring>
#include <cstdio>
#include <unity.h>
#include "gerda_security.h"
#include "gerda_sha256.h"
#include "gerda_link.h"
#include "common.h"

static void bytes_from_hex(const char *hex, uint8_t *out, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        unsigned v = 0;
        TEST_ASSERT_EQUAL(1, sscanf(hex + 2 * i, "%2x", &v));
        out[i] = (uint8_t)v;
    }
}

void test_sha256_empty(void)
{
    uint8_t out[32];
    uint8_t exp[32];
    gerda_sha256((const uint8_t *)"", 0, out);
    bytes_from_hex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", exp, 32);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, 32);
}

void test_sha256_abc(void)
{
    uint8_t out[32];
    uint8_t exp[32];
    gerda_sha256((const uint8_t *)"abc", 3, out);
    bytes_from_hex("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", exp, 32);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, 32);
}

void test_hmac_rfc4231_case1(void)
{
    uint8_t key[20];
    memset(key, 0x0b, 20);
    const uint8_t *msg = (const uint8_t *)"Hi There";
    uint8_t out[32];
    uint8_t exp[32];
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_hmac_sha256(key, 20, msg, 8, out));
    bytes_from_hex("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", exp, 32);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, 32);
}

void test_hmac_rfc4231_case2(void)
{
    const uint8_t *key = (const uint8_t *)"Jefe";
    const uint8_t *msg = (const uint8_t *)"what do ya want for nothing?";
    uint8_t out[32];
    uint8_t exp[32];
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_hmac_sha256(key, 4, msg, 28, out));
    bytes_from_hex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", exp, 32);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, 32);
}

void test_hkdf_rfc5869_case1_32(void)
{
    uint8_t ikm[22];
    memset(ikm, 0x0b, 22);
    uint8_t salt[13];
    bytes_from_hex("000102030405060708090a0b0c", salt, 13);
    uint8_t info[10];
    bytes_from_hex("f0f1f2f3f4f5f6f7f8f9", info, 10);
    uint8_t out[32];
    uint8_t exp[32];
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_hkdf_sha256(ikm, 22, salt, 13, info, 10, out, 32));
    bytes_from_hex("3cb25f25faacd57a9043fbc819d2e1c9b2c05c7942a304beb455d373ca3af908", exp, 32);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, out, 32);
}

void test_kdf_deterministic_and_phrase(void)
{
    uint8_t uid[6] = {1, 2, 3, 4, 5, 6};
    uint8_t uid2[6] = {1, 2, 3, 4, 5, 7};
    const uint8_t *phrase = (const uint8_t *)"gerda";
    uint8_t k1[GERDA_SESSION_KEY_LEN];
    uint8_t k2[GERDA_SESSION_KEY_LEN];
    uint8_t k3[GERDA_SESSION_KEY_LEN];
    uint8_t k4[GERDA_SESSION_KEY_LEN];
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_kdf_session_key(uid, 6, 0, 0, k1, sizeof(k1)));
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_kdf_session_key(uid, 6, 0, 0, k2, sizeof(k2)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(k1, k2, sizeof(k1));
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_kdf_session_key(uid2, 6, 0, 0, k3, sizeof(k3)));
    TEST_ASSERT_FALSE(gerda_ct_equal(k1, k3, sizeof(k1)));
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_kdf_session_key(uid, 6, phrase, 5, k4, sizeof(k4)));
    TEST_ASSERT_FALSE(gerda_ct_equal(k1, k4, sizeof(k1)));
}

void test_replay_window(void)
{
    gerda_replay_reset(10);
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_replay_accept(11));
    TEST_ASSERT_EQUAL(GERDA_SEC_REPLAY, gerda_replay_accept(11));
    TEST_ASSERT_EQUAL(GERDA_SEC_REPLAY, gerda_replay_accept(10));
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_replay_accept(9));
    TEST_ASSERT_EQUAL(GERDA_SEC_REPLAY, gerda_replay_accept(9));
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_replay_accept(50)); // jump >= window resets
    gerda_replay_reset(250);
    TEST_ASSERT_EQUAL(GERDA_SEC_OK, gerda_replay_accept(5)); // forward wrap
    TEST_ASSERT_EQUAL(GERDA_SEC_REPLAY, gerda_replay_accept(5));
    gerda_replay_reset(20);
    TEST_ASSERT_EQUAL(GERDA_SEC_REPLAY, gerda_replay_accept(200)); // too old
}

void test_ota_mac_xor_roundtrip_and_flag(void)
{
    uint8_t uid[6] = {9, 8, 7, 6, 5, 4};
    uint8_t pkt[8];
    uint8_t orig[8];
    memset(pkt, 0xA5, sizeof(pkt));
    pkt[0] = 0x00; // RCDATA + zero crcHigh
    pkt[7] = 0x3C;
    memcpy(orig, pkt, sizeof(pkt));

    gerda_secure_link = 0;
    gerda_on_uid_ready(uid, sizeof(uid));
    gerda_ota_mac_xor(pkt, 8, 42);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(orig, pkt, 8); // Secure OFF = stock CRC field

    gerda_secure_link = 1;
    gerda_on_uid_ready(uid, sizeof(uid));
    TEST_ASSERT_TRUE(gerda_secure_active());
    gerda_ota_mac_xor(pkt, 8, 42);
    TEST_ASSERT_FALSE(gerda_ct_equal(orig, pkt, 8));
    TEST_ASSERT_EQUAL_UINT8(orig[0] & 0x03, pkt[0] & 0x03); // type bits preserved
    gerda_ota_mac_xor(pkt, 8, 42);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(orig, pkt, 8);

    uint8_t other[8];
    memcpy(pkt, orig, 8);
    gerda_ota_mac_xor(pkt, 8, 42);
    memcpy(other, orig, 8);
    gerda_ota_mac_xor(other, 8, 43);
    TEST_ASSERT_FALSE(gerda_ct_equal(pkt, other, 8));

    memcpy(pkt, orig, 8);
    gerda_ota_apply_mac(pkt, 8, 42, 1); // bind skip
    TEST_ASSERT_EQUAL_UINT8_ARRAY(orig, pkt, 8);

    uint8_t ota8[13];
    uint8_t ota8o[13];
    memset(ota8, 0x11, sizeof(ota8));
    ota8[0] = 0x02; // SYNC type
    ota8[2] = 99;   // nonce in packet
    memcpy(ota8o, ota8, sizeof(ota8));
    TEST_ASSERT_EQUAL_UINT8(99, gerda_ota_nonce_for_packet(ota8, 13, 1));
    gerda_ota_mac_xor(ota8, 13, 99);
    TEST_ASSERT_FALSE(gerda_ct_equal(ota8o, ota8, 13));
    gerda_ota_mac_xor(ota8, 13, 99);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ota8o, ota8, 13);

    gerda_secure_link = 0;
}

void test_flight_profiles(void)
{
    uint8_t rate = 0xFF, tlm = 0xFF;
    gerda_profile = GERDA_PROFILE_BALANCE;
    TEST_ASSERT_EQUAL(0, gerda_profile_rate_tlm(gerda_profile, &rate, &tlm));
    TEST_ASSERT_EQUAL_STRING("Баланс", gerda_profile_name_ru(0));
    TEST_ASSERT_EQUAL_UINT8(50, gerda_dynpower_lq_boost_min());
    TEST_ASSERT_EQUAL_UINT8(95, gerda_dynpower_lq_thresh_dn());
    TEST_ASSERT_EQUAL(0, gerda_should_defer_msp(0));
    TEST_ASSERT_EQUAL(0, gerda_should_defer_msp(50));
    TEST_ASSERT_EQUAL(1, gerda_should_defer_msp(30));

    TEST_ASSERT_EQUAL(1, gerda_profile_rate_tlm(GERDA_PROFILE_RANGE, &rate, &tlm));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)RATE_LORA_2G4_50HZ, rate);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)TLM_RATIO_1_16, tlm);
    TEST_ASSERT_EQUAL_STRING("Дальность", gerda_profile_name_ru(GERDA_PROFILE_RANGE));
    gerda_profile = GERDA_PROFILE_RANGE;
    TEST_ASSERT_EQUAL_UINT8(70, gerda_dynpower_lq_boost_min());
    TEST_ASSERT_EQUAL_UINT8(99, gerda_dynpower_lq_thresh_dn());
    TEST_ASSERT_EQUAL(1, gerda_should_defer_msp(50));
    TEST_ASSERT_EQUAL(0, gerda_should_defer_msp(80));

    TEST_ASSERT_EQUAL(1, gerda_profile_rate_tlm(GERDA_PROFILE_SPEED, &rate, &tlm));
    TEST_ASSERT_EQUAL_UINT8((uint8_t)RATE_LORA_2G4_500HZ, rate);
    TEST_ASSERT_EQUAL_UINT8((uint8_t)TLM_RATIO_1_64, tlm);
    TEST_ASSERT_EQUAL_STRING("Скорость", gerda_profile_name_ru(GERDA_PROFILE_SPEED));
    gerda_profile = GERDA_PROFILE_SPEED;
    TEST_ASSERT_EQUAL(0, gerda_should_defer_msp(20));
    TEST_ASSERT_EQUAL_UINT8(50, gerda_dynpower_lq_boost_min());

    TEST_ASSERT_EQUAL(0, gerda_link_feature_enabled(GERDA_LINK_IA_FHSS));
    TEST_ASSERT_EQUAL(0, gerda_link_feature_enabled(GERDA_LINK_ADAPTIVE_MCS));
    TEST_ASSERT_EQUAL(1, gerda_link_feature_enabled(GERDA_LINK_CC_PRIORITY));
    TEST_ASSERT_EQUAL(0, gerda_link_feature_enabled(GERDA_LINK_INTERPACKET_FEC));

    gerda_profile = GERDA_PROFILE_BALANCE;
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_sha256_empty);
    RUN_TEST(test_sha256_abc);
    RUN_TEST(test_hmac_rfc4231_case1);
    RUN_TEST(test_hmac_rfc4231_case2);
    RUN_TEST(test_hkdf_rfc5869_case1_32);
    RUN_TEST(test_kdf_deterministic_and_phrase);
    RUN_TEST(test_replay_window);
    RUN_TEST(test_ota_mac_xor_roundtrip_and_flag);
    RUN_TEST(test_flight_profiles);
    return UNITY_END();
}
