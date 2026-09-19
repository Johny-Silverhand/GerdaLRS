#pragma once

// GerdaLRS compile-time switches. Defaults preserve stock ExpressLRS RF and
// bind-phrase UID behaviour. Enable features explicitly from user_defines.txt.
//
// See docs/ARCHITECTURE.ru.md for the design (RU UI, CUSTOM_2640, security,
// range/speed). Do not treat a define as "done" until the matching module
// is implemented and reviewed.

// Optional ~2640 MHz hop set for 2.4 GHz / LR1121 radios.
// NOT an ISM allocation in most jurisdictions. The operator is solely
// responsible for licences and local law. FlyFish 9624R 2.4 (and typical
// LR1121 2.4 front-ends) are matched for ~2.4–2.5 GHz; using 2640 MHz
// without a suitable antenna / matching network will usually hurt range.
//
// Runtime selection is options.json "gerda-2g4" (Web UI). This define only
// sets the factory default before the first options file exists:
//   -DGERDA_DOMAIN_CUSTOM_2640
#if defined(GERDA_DOMAIN_CUSTOM_2640)
#define GERDA_DOMAIN_NAME "CUST2640"
#else
#define GERDA_DOMAIN_NAME "ISM2G4"
#endif

// Localization is in src/html/. Security / MCS stubs live in src/lib/GERDA/.
// Security stubs are hooked after UID setup but do not alter OTA packets.
