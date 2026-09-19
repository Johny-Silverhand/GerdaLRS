#pragma once

// Range / speed features (design + TODOs, not implemented).
//
// Planned:
//   1. Adaptive MCS
//        Switch LoRa SF / BW / packet rate from live LQ, SNR, and RSSI
//        with hysteresis so a single fade does not yo-yo the mode.
//        Must stay within rates both ends advertise (ELRS rate index).
//   2. Interference-aware FHSS
//        Weight / skip hop channels with persistent noise (RSSI floor).
//        Keep the sync channel usable; never shrink the hop set so far that
//        the sequence becomes guessable.
//   3. Control-channel priority
//        Prefer channels / AUX (arm, flight mode) over low-priority telemetry
//        when the airtime budget is tight or LQ is falling.
//   4. Inter-packet FEC
//        Optional parity over N consecutive OTA packets so a single hop fade
//        can be recovered. Costs airtime; default off.
//
// TODOs:
//   [ ] Telemetry of per-hop RSSI histogram (debug only)
//   [ ] MCS policy table per RF chip (SX128x vs LR1121)
//   [ ] FEC encoder/decoder prototype on native tests before RX/TX
//   [ ] Never enable experimental MCS/FEC in the default user_defines

enum gerda_link_feature {
    GERDA_LINK_ADAPTIVE_MCS = 0,
    GERDA_LINK_IA_FHSS = 1,
    GERDA_LINK_CC_PRIORITY = 2,
    GERDA_LINK_INTERPACKET_FEC = 3,
};

static inline int gerda_link_feature_enabled(enum gerda_link_feature feature)
{
    (void)feature;
    return 0;
}
