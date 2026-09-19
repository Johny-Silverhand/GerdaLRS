#pragma once

#include "gerda_config.h"

// Regulatory domain selection (design + stub).
//
// Stock ExpressLRS:
//   - SX128x 2.4: compile-time ISM_2400 or EU_CE_2400 (LBT) in FHSS.cpp
//   - LR1121 2.4 hop table: domainsDualBand[] = ISM2G4 2400.4–2479.4 MHz
//   - 900 MHz: runtime domain index (AU915/FCC915/EU868/...)
//
// GerdaLRS plan:
//   - Keep ISM_2400 as the default, legal, testd path.
//   - Add selectable CUSTOM_2640 (~2600.4–2679.4 MHz, same 80-channel span
//     as ISM2G4) behind -DGERDA_DOMAIN_CUSTOM_2640. See FHSS.cpp.
//   - Later: runtime selection in Web UI (ISM_2400 vs CUSTOM_2640) stored
//     in options JSON, applied on both TX and RX. TX and RX MUST match.
//
// The attached community guide for editing FHSS.cpp domain tables is the
// reference technique. Do not flash CUSTOM_2640 to one end of the link only.

#if defined(GERDA_DOMAIN_CUSTOM_2640)
#define GERDA_HAS_CUSTOM_2640 1
#else
#define GERDA_HAS_CUSTOM_2640 0
#endif
