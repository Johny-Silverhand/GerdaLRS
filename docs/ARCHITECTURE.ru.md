# Архитектура GerdaLRS

Документ описывает слой GerdaLRS поверх ExpressLRS `7684347`. Полный конкурент-стек не реализуется: дерево должно оставаться собираемым и совместимым со стоковым ISM 2.4.

Связанные файлы:

| Модуль | Файлы | Статус |
| --- | --- | --- |
| Конфиг / флаги | `src/include/gerda_config.h`, `src/user_defines.txt` | есть |
| Локализация | `src/lib/GERDA/gerda_i18n.h`, `src/html/i18n-ru.js`, `src/html/index.html` | RU вкладки + Secure/профили |
| Домены | `src/lib/GERDA/gerda_domain.h`, `src/lib/FHSS/FHSS.cpp`, `src/lib/OPTIONS/options.cpp` | runtime ISM / CUSTOM_2640 |
| Безопасность | `src/lib/GERDA/gerda_security.*`, `gerda_sha256.*`, `docs/SECURITY.ru.md` | XOR-MAC TX/RX, тесты tamper/ON↔OFF, default OFF |
| Дальность/скорость | `src/lib/GERDA/gerda_link.*`, Web UI `gerda-profile` | профили + MSP defer + dynpower RANGE |
| Умный FHSS | `src/lib/GERDA/gerda_fhss.*`, `docs/FHSS.ru.md` | гистограмма + soft denylist, hops не меняем |
| Железо | `docs/HARDWARE.ru.md`, `src/hardware/targets.json` | пара v1 |

## 1. База

```
GerdaLRS (этот репозиторий)
  ├── firmware  ← ExpressLRS @ 7684347ee697b3fa4318f99a02b6ae6f5d703307
  └── hardware  ← ExpressLRS/targets @ bda4c92e8a78459b4d3510f94f04dbdcbd0a5576
                  + алиас flyfish.rx_dual.9624r
```

OTA, UID, CRC, FHSS, unified binary+layout — как в ExpressLRS. GerdaLRS добавляет слой в `src/lib/GERDA/`, options `gerda-2g4` / `gerda-secure` / `gerda-profile`, русский Web UI и документацию.

**Пара v1:** RX FlyFish 9624R 2.4 (ESP32-C3 + LR1121) + TX RadioMaster Ranger Nano 2.4 (SX1280, Lua `RM Ranger Nano`, `radiomaster.tx_2400.ranger-nano`; корпус может быть подписан Micro). Стандартные режимы ELRS 2.4 работают cross-chip; DK500/K1000 — вне v1.

## 2. Русский Web UI

Сток: HTML в `src/html/`, gzip+PROGMEM через `src/python/build_html.py`. На ESP32-C3 место ограничено.

**Слой локализации:**

1. Шаблоны ExpressLRS остаются каркасом.
2. Видимые строки вкладок Модель / Опции / Wi‑Fi / Обновление — на русском.
3. Словарь `src/html/i18n-ru.js` (ключи `data-i18n`) подключается в `scan.js` / `hardware.js` через `@@include`; `gerdaApplyI18n()` после DOM.
4. Не дублировать целые HTML-деревья на русском.

Брендинг «Герда / GerdaLRS» в шапке. AP: `GerdaLRS TX` / `GerdaLRS RX`, пароль `expresslrs`.

Длинные help-тексты PWM/пинов Hardware ещё частично на английском.

## 3. Регуляторный домен: ISM_2400 + CUSTOM_2640

### Сток (этот commit)

Для `RADIO_LR1121` таблица 2.4 ГГц — `domainsDualBand[]` в `FHSS.cpp`:

```c
{"ISM2G4", 2400400000, 2479400000, 80, 2440000000},
{"CUST2640", 2600400000, 2679400000, 80, 2640000000}
```

Для SX128x то же в `domains[]` (индекс 0 = ISM2G4/CE_LBT, индекс 1 = CUST2640).

900 МГц: индекс домена в options (AU915, FCC915, …) **не** используется для выбора 2640.

### Runtime GerdaLRS

| Домен | Диапазон | Как включается | Статус |
| --- | --- | --- | --- |
| ISM_2400 | 2400.4–2479.4 МГц, 80 хопов | `gerda-2g4` = 0 (по умолчанию) | сток ELRS |
| CUSTOM_2640 | 2600.4–2679.4 МГц, 80 хопов (span 79 МГц) | Web UI / `options.json` `"gerda-2g4": 1` | **runtime** |

Поле **не** в packed `firmware_options_t` (чтобы не ломать magic ELRS). Читается в `FHSSrandomiseFHSSsequence()` через `gerda_2g4_band_index()`.

`-DGERDA_DOMAIN_CUSTOM_2640` только меняет заводской default до первого `options.json`.

TX и RX обязаны совпадать, иначе линка не будет.

### RF caveat

LR1121 на 2.4-платах (FlyFish 9624R) и SX1280 Ranger Nano рассчитаны на **~2.4–2.5 ГГц**. На ~2640 МГц КСВ и мощность на антенне почти наверняка хуже. Это не «бесплатная дальность».

**Закон:** частоты вне ISM — ответственность пользователя. В репозитории нет обещания легальности CUSTOM_2640.

## 4. Безопасность линка (реализовано за флагом)

Полная схема: [`docs/SECURITY.ru.md`](SECURITY.ru.md).

Сток ELRS: bind-phrase → MD5 → 6-байтовый UID; UID в FHSS / IQ / CRC init; payload **не** аутентифицируется.

Gerda Secure Link (options `gerda-secure`, default **0**):

```
UID (phrase уже внутри MD5)
    → HKDF-SHA256(ikm=UID, salt="GerdaLRS-ota-v1", info="ota-mac-v1", 16)
    → HMAC-SHA256, обрезка 14/16 бит, XOR в поле CRC
    → на RX: XOR обратно, сток CRC, anti-replay окно 32 по OtaNonce
    → fail = drop, без takeover
```

Bind-режим MAC не применяет. Secure OFF — байты эфира как у ELRS.

Хуки: `gerda_ota_apply_mac` сразу после `OtaGeneratePacketCrc` / сразу до `OtaValidatePacketCrc` в `tx_main.cpp` / `rx_main.cpp`. Формат OTA не меняется (не второй радиостек).

Native-тесты: `src/test/test_gerda/` (SHA-256, RFC 4231 HMAC, RFC 5869 HKDF, KDF, replay, XOR, tamper, ON↔OFF).

## 5. Дальность и скорость

Пара v1: Ranger Nano **SX1280** + FlyFish **LR1121**. Только стандартные LoRa 2.4 (50 / 150 / 250 / 333 / 500 Гц и т.д.). DK500/K1000 — вне скоупа этого TX.

| Фича | Статус | Где |
| --- | --- | --- |
| Профиль Баланс | есть | не трогает Lua Packet Rate / Telem |
| Профиль Дальность | есть | TX boot: `RATE_LORA_2G4_50HZ` + `TLM_RATIO_1_16`; dynpower LQ boost min 70 (сток 50), power-down только при LQ≥99 (сток 95) |
| Профиль Скорость | есть | TX boot: `RATE_LORA_2G4_500HZ` + `TLM_RATIO_1_64` (не FLRC) |
| Приоритет RC vs MSP | есть | `tx_main.cpp` `SendRCdataToRF`: `gerda_should_defer_msp(uplink_LQ)` |
| Telem backoff (слот TLM) | **нет** | `ExpressLRS_currTlmDenom` синхронизируется SYNC (`tx_main.cpp` `GenerateSyncPacketData` / `rx_main.cpp` `ProcessRfPacket_SYNC`); менять mid-flight без syncspam — не чистый хук. TODO Phase 4 |
| Adaptive MCS | **нет** | риск рассинхрона SX1280↔LR1121 |
| IA-FHSS | **soft denylist** | гистограмма RSSI/CRC; hops **не** меняются; LQ/AFC смягчение. Синхронный skip-map — не в эфире. `docs/FHSS.ru.md` |
| Межпакетный FEC | **нет** | airtime |
| CUSTOM_2640 | опция | **не** улучшает дальность на этом фронтенде |

Lua-facing: отдельного пункта «Профиль Герда» в EdgeTX Lua нет (размер скрипта). Профиль живёт в Web UI; Lua по-прежнему показывает Packet Rate / Telem Ratio. После выбора Дальность/Скорость перезагрузка TX записывает rate/tlm в EEPROM-конфиг модели — дальше Lua их видит.

`gerda_link_feature_enabled(CC_PRIORITY | IA_FHSS)` = 1; MCS / FEC = 0. Runtime «Умный FHSS»: `gerda_smart_fhss`.

## 6. Потоки данных

```
TX handset CRSF (Ranger Nano SX1280)
  → OTA pack + (если Secure ON) HMAC XOR в CRC
  → FHSS hop (ISM2G4 или CUST2640)
  → эфир 2.4
RX FlyFish LR1121
  → hop
  → (если Secure ON) XOR + CRC + replay window
  → CRSF/SBUS к FC
Web UI (Wi-Fi AP GerdaLRS RX/TX)
  → i18n RU
  → gerda-2g4 + gerda-secure + gerda-profile + gerda-fhss
```

## 7. Чеклист следующих шагов

- [x] i18n: словарь + русские вкладки Модель/Опции/Wi‑Fi/Обновление.
- [x] Runtime ISM vs 2640 в options; синхронизация TX/RX (оба конца вручную).
- [x] TX: RadioMaster Ranger Nano 2.4 (`radiomaster.tx_2400.ranger-nano`).
- [x] Secure Link: спека, SHA-256/HMAC/HKDF, XOR-MAC в TX/RX, native-тесты, UI-флаг default OFF.
- [x] Профили Баланс/Дальность/Скорость + MSP defer + dynpower RANGE.
- [x] Умный FHSS: гистограмма + soft denylist (hops как у ELRS).
- [ ] Доперевести длинные help-тексты PWM / Hardware pins.
- [ ] Снять hardware.json с FlyFish 9624R и сравнить с `Generic C3 LR1121.json`.
- [ ] Измерить RSSI/дальность на 2640 со штатной антенной (ожидается хуже).
- [ ] Synced FHSS skip-map по TLM/MSP; epoch anti-replay; adaptive MCS.
- [ ] Не включать CUSTOM_2640, Secure и Smart FHSS в дефолтный `user_defines.txt`.
