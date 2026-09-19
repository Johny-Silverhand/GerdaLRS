# GerdaLRS / Герда

**GerdaLRS** — производная прошивка на базе [ExpressLRS](https://github.com/ExpressLRS/ExpressLRS) (GPL-3.0). Цель проекта: русскоязычный Web UI, опциональный домен **CUSTOM_2640** (~2640 МГц) рядом с ISM 2.4, более устойчивый линк (дальность/скорость) и защита сильнее, чем UID от bind-phrase.

Брендинг UI: **Герда / GerdaLRS**. Это независимый форк, не связанный с ExpressLRS LLC.

Основа: ExpressLRS commit **`7684347ee697b3fa4318f99a02b6ae6f5d703307`** (короткий хеш `768434`). История git этого репозитория — импорт дерева (не полный clone ExpressLRS). Подробности: [`docs/UPSTREAM.md`](docs/UPSTREAM.md), атрибуция: [`NOTICE`](NOTICE), лицензия: [`LICENSE`](LICENSE).

## Что уже есть в этом дереве

- Полный исходник ExpressLRS (не README-only).
- Снимок hardware-таргетов ExpressLRS/targets (`bda4c92`) в `src/hardware/`.
- Русский Web UI (вкладки Модель/Опции/Wi‑Fi/Обновление) и таблица строк `src/html/i18n-ru.js`.
- Выбор **ISM 2.4 / CUSTOM_2640** в Web UI (`gerda-2g4` в options.json); по умолчанию ISM.
- Stubs безопасности (KDF/HMAC/anti-replay) + хук после UID; **не** крипто на эфире.
- Брендинг Герда / GerdaLRS. Пароль Wi‑Fi AP: `expresslrs`.

## Оборудование

### Приёмник (основной)

| | |
| --- | --- |
| Плата | **FlyFish 9624R 2.4** (маркировка 9624R1), один U.FL |
| MCU | ESP32-C3 |
| Радио | Semtech **LR1121** |
| Строка в стоковом Web UI | `FlyFish 9624R 2.4`, прошивка `master (768434) 2640` |

В официальном `targets.json` **нет** вендорского ключа FlyFish. Не прошивайте SX128x / HappyModel `UNIFIED_ESP32_2400_RX` — это другой чип.

| Назначение | Путь в `src/hardware/targets.json` | Firmware env | Layout |
| --- | --- | --- | --- |
| Ближайший официальный таргет | **`generic.rx_dual.c3-plain`** | `Unified_ESP32C3_LR1121_RX` | `RX/Generic C3 LR1121.json` |
| Алиас GerdaLRS (пин-аут **не** снят с платы) | **`flyfish.rx_dual.9624r`** | тот же | тот же |
| Похожий серийный C3+LR1121 2.4 RX | `radiomaster.rx_dual.xr2` | тот же | тот же |

Алиас `flyfish.rx_dual.9624r` только подставляет имя `FlyFish 9624R 2.4` на generic C3 LR1121. Если GPIO не совпадут, RF/LED/кнопка могут не работать; ESP32-C3 обычно остаётся доступен по UART/Wi‑Fi. Перед массовой прошивкой снимите `hardware.json` с уже работающего RX (вкладка Hardware).

Подробности: [`docs/HARDWARE.ru.md`](docs/HARDWARE.ru.md).

### Передатчик (v1)

Lua на аппаратуре показывает **RM Ranger Nano**. Корпус/фото могут быть подписаны как Ranger Micro — для прошивки ориентируйтесь на **Lua-имя**.

Это модуль JR-bay **2.4 ГГц SX1280** (не LR1121). USB-C, XT30 6–16.8 V, вентилятор, RP-SMA.

| | |
| --- | --- |
| Configurator / product_name | **RadioMaster Ranger Nano 2.4GHz TX** |
| Lua | `RM Ranger Nano` |
| Путь | **`radiomaster.tx_2400.ranger-nano`** |
| Firmware env | `Unified_ESP32_2400_TX` (`_via_UART` / `_via_WIFI`) |
| Layout | `TX/Radiomaster Ranger Micro.json` (Nano использует тот же layout + overlay мощности) |

Рядом в `targets.json` есть `radiomaster.tx_2400.ranger-micro` (`RM Ranger Micro`) — **не** выбирайте его, если Lua пишет Nano.

**Совместимость пары v1:** стандартные режимы ELRS 2.4 работают между SX1280 TX и LR1121 RX. Режимы только для LR1121 (DK500 / K1000 и т.п.) требуют LR1121 на **обоих** концах — для Ranger Nano это **вне v1**.

Пример настроек Lua (не дефолт прошивки): Packet Rate 50 Hz, Telem Std 1:16, Switch Wide, Link Normal, Model Match Off, TX Power 1000 mW.

## Дорожная карта

1. **Русский Web UI** — таблица `src/html/i18n-ru.js`, вкладки уже на русском; дальше доперевести длинные help-тексты.
2. **Домен** — ISM 2.4 по умолчанию; CUSTOM_2640 выбирается в Web UI (оба конца должны совпадать).
3. **Безопасность** — хук `gerda_on_uid_ready` + stubs KDF/HMAC/anti-replay. Сток UID на эфире. Не заявлять «защищённый линк».
4. **Дальность/скорость** — adaptive MCS, FHSS с учётом помех, приоритет каналов, FEC. Пока дизайн: `gerda_link.h`.

Архитектура: [`docs/ARCHITECTURE.ru.md`](docs/ARCHITECTURE.ru.md).

## Сборка

Нужны Python 3, Git, [PlatformIO](https://platformio.org/) (VS Code / CLI). Путь к репозиторию **без** кириллицы.

### PlatformIO (локально)

Откройте каталог **`src/`** как проект PlatformIO.

Минимум в `src/user_defines.txt` для **этой пары** (SX1280 TX + LR1121 RX):

```
-DMY_BINDING_PHRASE="ваша_фраза"
-DRegulatory_Domain_ISM_2400
-DRegulatory_Domain_FCC_915
-DAUTO_WIFI_ON_INTERVAL=60
-DLOCK_ON_FIRST_CONNECTION
```

`ISM_2400` нужен Ranger Nano (SX1280). `FCC_915` (или другой 900-домен) нужен, чтобы сборка LR1121 RX прошла `#error` в `targets.h`; hop 2.4 всё равно берётся из `domainsDualBand[]` / Web UI `gerda-2g4`.

**TX — RadioMaster Ranger Nano 2.4:**

```bash
cd src
pio run -e Unified_ESP32_2400_TX_via_UART
```

В списке конфигурации: `RadioMaster Ranger Nano 2.4GHz TX` / путь `radiomaster.tx_2400.ranger-nano`.

**RX — FlyFish 9624R 2.4:**

```bash
cd src
pio run -e Unified_ESP32C3_LR1121_RX_via_UART
```

Путь: `generic.rx_dual.c3-plain` (безопаснее) или `flyfish.rx_dual.9624r` (имя как на устройстве, пин-аут не снят).

Не собирайте HappyModel / `Unified_ESP32_2400_RX` (SX128x) для FlyFish.

Бинарник: `src/.pio/build/<env>/firmware.bin`. Wi‑Fi AP: `GerdaLRS RX` / `GerdaLRS TX`, пароль `expresslrs`, `http://10.0.0.1/`. Диапазон 2.4/2640 — вкладка **Опции**.

### ExpressLRS Configurator, режим Local

1. Установите [ExpressLRS Configurator](https://github.com/ExpressLRS/ExpressLRS-Configurator/releases/).
2. Source: **Local**, укажите этот репозиторий.
3. **TX:** Device **RadioMaster Ranger Nano 2.4GHz TX** (`radiomaster.tx_2400.ranger-nano`). Radio SX1280 / Unified ESP32 2400 TX.
4. **RX:** Radio **LR1121**, MCU **ESP32-C3**. Device **Generic C3 LR1121 2.4/900 RX** или **FlyFish 9624R 2.4**.
5. Не выбирайте HappyModel SX128x RX для FlyFish и не берите Ranger Micro, если Lua пишет Nano.

## Радио и закон

Использование частот **вне ISM 2.4 ГГц** (в том числе ~2640 МГц) может требовать лицензии или быть запрещено. **Ответственность на операторе.** Прошивка не заявляет соответствия нормам.

Фронтенд LR1121 на FlyFish 9624R согласован примерно на **2.4–2.5 ГГц**. CUSTOM_2640, скорее всего, ухудшит дальность без подходящей антенны и согласования.

`CUSTOM_2640` **выключен** по умолчанию (ISM 2.4). Включать только в Web UI на **TX и RX** сразу. `-DGERDA_DOMAIN_CUSTOM_2640` лишь меняет заводской default на 2640.

## GPL-3.0

Исходники должны оставаться открытыми под GNU GPL v3. Не удаляйте `LICENSE` / `NOTICE`. Распространение бинарников требует Corresponding Source (GPL §6).

Upstream README ExpressLRS можно сравнить с https://github.com/ExpressLRS/ExpressLRS/blob/7684347ee697b3fa4318f99a02b6ae6f5d703307/README.md.

---

## English

GerdaLRS is a GPL-3.0 ExpressLRS derivative (upstream `7684347ee697b3fa4318f99a02b6ae6f5d703307`). Focus: Russian Web UI, selectable ISM 2.4 vs CUSTOM_2640, stronger-than-UID security (stubs/hooks only), planned range/speed work.

**v1 hardware pair**

- **RX:** FlyFish 9624R 2.4 (ESP32-C3 + LR1121). Path: `generic.rx_dual.c3-plain` or unverified alias `flyfish.rx_dual.9624r`. Env: `Unified_ESP32C3_LR1121_RX_via_UART`. Never flash SX128x HappyModel RX binaries.
- **TX:** RadioMaster Ranger Nano 2.4 (Lua `RM Ranger Nano`; casing may say Micro). Path: `radiomaster.tx_2400.ranger-nano`. Env: `Unified_ESP32_2400_TX_via_UART`. SX1280, not LR1121. Standard 2.4 ELRS rates interoperate with the LR1121 RX; DK500/K1000 are out of scope for this TX.

Non-ISM frequencies (including ~2640 MHz) are the operator’s legal responsibility. 2.4 matching is ~2.4–2.5 GHz; 2640 MHz will likely reduce range. Default RF remains ISM 2.4; both ends must match if you select CUSTOM_2640 in the Web UI.

## Следующие шаги

- [x] Русские вкладки Web UI + `i18n-ru.js`.
- [x] Runtime ISM / CUSTOM_2640 в options (`gerda-2g4`).
- [x] TX: RadioMaster Ranger Nano 2.4 (`radiomaster.tx_2400.ranger-nano`).
- [x] Native-тесты hop-таблицы CUSTOM_2640.
- [ ] Доперевести длинные help-тексты PWM/Hardware pins.
- [ ] Снять `hardware.json` с живого FlyFish 9624R.
- [ ] Спека OTA для настоящего HMAC **до** изменения пакетов.
