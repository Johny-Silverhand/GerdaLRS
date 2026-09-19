# Архитектура GerdaLRS

Документ описывает слой GerdaLRS поверх ExpressLRS `7684347`. Полный конкурент-стек не реализуется: дерево должно оставаться собираемым и совместимым со стоковым ISM 2.4.

Связанные файлы:

| Модуль | Файлы | Статус |
| --- | --- | --- |
| Конфиг / флаги | `src/include/gerda_config.h`, `src/user_defines.txt` | есть |
| Локализация | `src/lib/GERDA/gerda_i18n.h`, `src/html/i18n-ru.js`, `src/html/index.html` | RU вкладки + таблица строк |
| Домены | `src/lib/GERDA/gerda_domain.h`, `src/lib/FHSS/FHSS.cpp`, `src/lib/OPTIONS/options.cpp` | runtime ISM / CUSTOM_2640 |
| Безопасность | `src/lib/GERDA/gerda_security.h`, `gerda_security.cpp` | stubs + хук UID |
| Дальность/скорость | `src/lib/GERDA/gerda_link.h` | только дизайн |
| Железо | `docs/HARDWARE.ru.md`, `src/hardware/targets.json` | пара v1 |

## 1. База

```
GerdaLRS (этот репозиторий)
  ├── firmware  ← ExpressLRS @ 7684347ee697b3fa4318f99a02b6ae6f5d703307
  └── hardware  ← ExpressLRS/targets @ bda4c92e8a78459b4d3510f94f04dbdcbd0a5576
                  + алиас flyfish.rx_dual.9624r
```

OTA, UID, CRC, FHSS, unified binary+layout — как в ExpressLRS. GerdaLRS добавляет тонкий слой в `src/lib/GERDA/`, options `gerda-2g4`, русский Web UI и документацию.

**Пара v1:** RX FlyFish 9624R 2.4 (ESP32-C3 + LR1121) + TX RadioMaster Ranger Nano 2.4 (SX1280). Стандартные режимы ELRS 2.4 работают cross-chip; DK500/K1000 — вне v1.

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

## 4. Безопасность линка (дизайн + stubs)

Сток ELRS: bind-phrase → MD5 → 6-байтовый UID; UID участвует в FHSS seed / IQ / полях sync; полезная нагрузка **не** аутентифицируется.

Цель GerdaLRS — не «обфускация частоты», а криптография поверх UID:

```
bind phrase
    → UID (как сейчас, для совместимости пары)
    → HKDF-SHA256(UID || bind_nonce || "GerdaLRS v1")
    → session_key
    → truncated HMAC над {nonce, type, payload}
    → окно anti-replay по счётчику
```

Stubs: `gerda_kdf_session_key`, `gerda_hmac_tag`, `gerda_replay_check` всегда возвращают `GERDA_SEC_NOT_IMPLEMENTED`.

Хук `gerda_on_uid_ready(UID, UID_LEN)` вызывается из `rx_main` / `tx_main` после `OtaUpdateCrcInitFromUid()`. Он **не** меняет пакеты OTA. Пока HMAC не в OTA, **нельзя** писать в UI «защищённый линк».

Порядок внедрения: спека размера пакета → native-тесты → RX verify → TX tag → UI. Ломать CRSF/airtime без расчёта нельзя.

## 5. Дальность и скорость (план)

| Фича | Идея | Риск |
| --- | --- | --- |
| Adaptive MCS | Менять rate/SF по LQ/SNR с гистерезисом | рассинхрон режимов TX/RX |
| Interference-aware FHSS | Реже ходить на шумные каналы | слишком узкий hop-set упрощает перехват |
| Приоритет управления | ARM / flight mode важнее телеметрии при плохом LQ | голод телеметрии |
| Межпакетный FEC | Чётность на N пакетов | воздух, латентность |

`gerda_link_feature_enabled()` сейчас всегда 0.

## 6. Потоки данных

```
TX handset CRSF (Ranger Nano SX1280)
  → OTA pack (+ future HMAC)
  → FHSS hop (ISM2G4 или CUST2640)
  → эфир 2.4
RX FlyFish LR1121
  → hop
  → HMAC/replay (future)
  → CRSF/SBUS к FC
Web UI (Wi-Fi AP GerdaLRS RX/TX)
  → i18n RU
  → gerda-2g4 + options
```

## 7. Чеклист следующих шагов

- [x] i18n: словарь + русские вкладки Модель/Опции/Wi‑Fi/Обновление.
- [x] Runtime ISM vs 2640 в options; синхронизация TX/RX (оба конца вручную).
- [x] TX: RadioMaster Ranger Nano 2.4 (`radiomaster.tx_2400.ranger-nano`).
- [x] Хук UID + stubs KDF/HMAC/anti-replay (не крипто на эфире).
- [ ] Доперевести длинные help-тексты PWM / Hardware pins.
- [ ] Снять hardware.json с FlyFish 9624R и сравнить с `Generic C3 LR1121.json`.
- [ ] Измерить RSSI/дальность на 2640 со штатной антенной (ожидается хуже).
- [ ] Спека session key / HMAC / nonce; затем код, не наоборот.
- [ ] Adaptive MCS только после таблицы поддерживаемых rates для SX1280+LR1121.
- [ ] Не включать CUSTOM_2640 в дефолтный `user_defines.txt`.
