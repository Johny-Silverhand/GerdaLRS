# Безопасность линка GerdaLRS

Схема **Gerda Secure Link** поверх ExpressLRS `7684347`. По умолчанию **выключена**: поведение как у стокового ELRS (совместимость с чужим TX/RX).

Вкл. только Gerda↔Gerda с одной bind-phrase. Сток ELRS с Secure ON **не** соединится.

## Что даёт сток ELRS

- Bind-phrase → MD5 → 6-байтовый UID.
- UID участвует в FHSS seed, IQ invert и CRC initializer (`OtaCrcInitializer = UID[4..5] XOR OTA_VERSION_ID`).
- SYNC проверяет только часть UID (байт 4 и верхние биты байта 5).
- CRC14/CRC16 **без ключа**: зная полином, можно подделать пакет.
- Полезная нагрузка (каналы) не аутентифицируется.

Цель Gerda — не «спрятать частоту», а отбросить поддельные пакеты, если нет session key.

## Схема OTA (v1)

```
bind phrase
    → UID (как в ELRS, для пары и FHSS)
    → HKDF-SHA256(
         ikm  = UID || phrase_bytes (phrase может быть пустым, если есть только UID),
         salt = "GerdaLRS-ota-v1",
         info = "ota-mac-v1",
         L    = 16)
    → session_key[16]
```

На RX после bind фразы в RAM нет, только UID. Тогда IKM = UID. Секрет тот же, что у пары (48 бит UID). Это **не** AES-GCM и не защита от того, кто знает phrase.

### Per-packet MAC

Пакет ELRS: 8 байт (OTA4, CRC14) или 13 байт (OTA8, CRC16). Лишних байт нет.

**Тег кладётся в поле CRC XOR'ом** (0 доп. airtime):

1. Сток `OtaGeneratePacketCrc`.
2. HMAC-SHA256(session_key, body_без_CRC || nonce || pkt_len), обрезка до 14 или 16 бит.
3. CRC_air = CRC_stock XOR MAC.

Приём:

1. MAC XOR обратно (тот же ключ и nonce).
2. Сток `OtaValidatePacketCrc`.
3. Если MAC неверный, CRC не сойдётся → пакет **отбрасывается**, каналы/SYNC не обновляются (нет takeover).

В bind-режиме XOR **не** применяется (сток bind MSP).

Nonce для HMAC:

- Обычные пакеты: локальный `OtaNonce` (TX и RX синхронизированы).
- SYNC: `otaSync->nonce` из пакета (у RX локальный nonce ещё неверный).

### Anti-replay

Окно 32 по `uint8_t OtaNonce` (модуль 256):

- дубликаты в окне → drop;
- слишком старые → drop;
- скачок вперёд (≥ окна, как у SYNC) → окно сдвигается.

Ограничение: полный оборот nonce (256 пакетов, ~0.5 с на 500 Гц) теоретически позволяет replay «с прошлого круга», если окно уже ушло. Лечение — epoch в свободных байтах SYNC (Phase 4). Для v1 этого достаточно, чтобы отсечь инъекцию «сбоку» без ключа и простой повтор последнего кадра.

### Auth fail

Drop. Нет failsafe-перехвата, нет смены UID, нет ответа по эфиру.

## Airtime / латентность

| Режим | MAC | Доп. байт в эфире | CPU |
| --- | --- | --- | --- |
| OTA4 (50–500 Hz LoRa 2.4) | 14 бит | 0 | HMAC-SHA256 ~13 байт / пакет |
| OTA8 (8ch/12ch) | 16 бит | 0 | то же |

14 бит ≈ 1/16384 случайных подделок на пакет; 16 бит ≈ 1/65536. Это старт, не AEAD. На ESP32-C3 / ESP32 HMAC на 500 Гц незаметен относительно LoRa TOA.

## Флаги

| Флаг | Где | Default |
| --- | --- | --- |
| `gerda-secure` в options.json / Web UI «Защищённый линк Gerda» | runtime | 0 (OFF) |
| `-DGERDA_SECURE_LINK_DEFAULT` | compile, только заводской default | выкл |

Оба конца должны совпадать. Phrase/UID — как у стоковой пары.

## Хуки в коде ELRS

| Место | Файл | Что |
| --- | --- | --- |
| После CRC TX uplink | `tx_main.cpp` `SendRCdataToRF` | `gerda_ota_apply_mac` (skip bind) |
| Перед CRC RX uplink | `rx_main.cpp` `ProcessRFPacket` | xor + `gerda_replay_accept` на RCDATA |
| После CRC RX downlink TLM | `rx_main.cpp` (отправка TLM) | `gerda_ota_apply_mac` |
| Перед CRC TX downlink TLM | `tx_main.cpp` `ProcessTLMpacket` | xor |
| Nonce SYNC | `gerda_ota_nonce_for_packet` | `pkt[2]` (`OTA_Sync_s.nonce`) |
| После UID | `gerda_on_uid_ready` | HKDF, session 16 байт |

Пакетный формат OTA **не** меняется. Это не второй радиостек.

## Честно vs ELRS

Secure OFF: как ELRS (bind, Wi‑Fi, CRSF, MQTT не трогаем).  
Secure ON: Gerda не примет чужой ELRS и наоборот. Подделка CRC без ключа не проходит. Это не AEAD и не «военный» линк.

Тесты (`src/test/test_gerda/`): SHA-256, RFC 4231 HMAC (в т.ч. 50-байтовый вектор), RFC 5869 HKDF, KDF/UID mismatch, окно replay включая край 31/32, XOR roundtrip, порча payload, ON TX / OFF RX и наоборот (CRC поле не совпадает со стоком).

MAC остаётся в поле CRC (0 доп. байт). 14/16 бит — компромисс airtime; длиннее тег без лишнего байта в OTA4 нельзя. Не называем это AES.

## Ограничения, которые остаются

- Wrap-replay: `OtaNonce` 8 бит, полный круг 256 пакетов. Epoch в `OTA_Sync_s` / `free[4]` OTA8 **не** внедрён: лишний рассинхрон epoch хуже, чем окно 32 против боковой инъекции.
- Session = HKDF(UID). Phrase на RX после bind нет; секрет не сильнее 48 бит UID.
- Bind / Wi‑Fi AP / CRSF — сток, MAC в bind не ставится.
