#include "gerda_security.h"

// Bind-time hook only. Return values are ignored; packets are still stock ELRS.
void gerda_on_uid_ready(const uint8_t *uid, size_t uid_len)
{
    uint8_t session[16];
    (void)gerda_kdf_session_key(uid, uid_len, nullptr, 0, session, sizeof(session));
    (void)gerda_hmac_tag(session, sizeof(session), uid, uid_len, session, 4);
    (void)gerda_replay_check(0);
}
