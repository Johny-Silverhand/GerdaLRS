#pragma once

#include "gerda_config.h"
#include <stdint.h>

// Runtime 2.4 GHz hop-set selection (ISM2G4 vs CUSTOM_2640).
// Stored in options.json as "gerda-2g4": 0 (ISM) or 1 (CUSTOM_2640).
// Not part of the packed firmware_options_t layout (avoids breaking ELRS magic).
// TX and RX MUST use the same value or the link will not connect.
extern uint8_t gerda_2g4_band;

static inline uint8_t gerda_2g4_band_index(void)
{
    return (gerda_2g4_band == 1) ? 1 : 0;
}

static inline const char *gerda_2g4_band_name(void)
{
    return gerda_2g4_band_index() ? "CUST2640" : "ISM2G4";
}

// Regulatory domain selection.
//
// Stock ExpressLRS:
//   - SX128x 2.4: compile-time ISM_2400 or EU_CE_2400 (LBT) in FHSS.cpp
//   - LR1121 2.4 hop table: domainsDualBand[] = ISM2G4 2400.4–2479.4 MHz
//   - 900 MHz: runtime domain index (AU915/FCC915/EU868/...)
//
// GerdaLRS:
//   - ISM_2400 is the default, legal, tested path (gerda_2g4_band = 0).
//   - CUSTOM_2640 (~2600.4–2679.4 MHz, same 80-channel span as ISM2G4)
//     is selected at runtime via Web UI / options.json "gerda-2g4".
//   - -DGERDA_DOMAIN_CUSTOM_2640 only changes the factory default to 1.
//   - TX and RX MUST match. Do not enable 2640 on one end only.
//
// The attached community guide for editing FHSS.cpp domain tables is the
// reference technique for the hop table itself.

#if defined(GERDA_DOMAIN_CUSTOM_2640)
#define GERDA_HAS_CUSTOM_2640 1
#else
#define GERDA_HAS_CUSTOM_2640 0
#endif
