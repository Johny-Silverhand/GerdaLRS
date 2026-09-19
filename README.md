# GerdaLRS / Герда

**GerdaLRS** — производная прошивка на базе [ExpressLRS](https://github.com/ExpressLRS/ExpressLRS) (GPL-3.0). Цель проекта: русскоязычный Web UI, опциональный домен **CUSTOM_2640** (~2640 МГц) рядом с ISM 2.4, более устойчивый линк (дальность/скорость) и защита сильнее, чем UID от bind-phrase.

Брендинг UI: **Герда / GerdaLRS**. Это независимый форк, не связанный с ExpressLRS LLC.

Основа: ExpressLRS commit **`7684347ee697b3fa4318f99a02b6ae6f5d703307`** (короткий хеш `768434`). История git этого репозитория — импорт дерева (не полный clone ExpressLRS). Подробности: [`docs/UPSTREAM.md`](docs/UPSTREAM.md), атрибуция: [`NOTICE`](NOTICE), лицензия: [`LICENSE`](LICENSE).

## Что уже есть в этом дереве

- Полный исходник ExpressLRS (не README-only).
- Снимок hardware-таргетов ExpressLRS/targets (`bda4c92`) в `src/hardware/`.
- Документация архитектуры и чеклист следующих шагов.
- Заготовки модулей (stubs/TODO), **без** рабочего радио-крипто и без включения 2640 МГц по умолчанию.
- Лёгкий брендинг Web UI (Герда / GerdaLRS). Пароль Wi‑Fi AP по-прежнему `expresslrs`.

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

### Передатчик

Точная модель TX **неизвестна**. Плейсхолдеры LR1121:

| Путь | Firmware env |
| --- | --- |
| `generic.tx_dual.gemini` | `Unified_ESP32_LR1121_TX` (`_via_UART` / `_via_WIFI` / `_via_ETX`) |

Не выбирайте SX128x TX «наугад».

## Дорожная карта

1. **Русский Web UI** — слой локализации поверх HTML ExpressLRS (`src/lib/GERDA/gerda_i18n.h`, `src/html/i18n-ru.stub.js`).
2. **Домен** — ISM 2.4 по умолчанию; опционально `CUSTOM_2640` (`-DGERDA_DOMAIN_CUSTOM_2640`, заготовка в `src/lib/FHSS/FHSS.cpp`).
3. **Безопасность** — post-bind session key (KDF), HMAC пакетов, anti-replay. Пока только дизайн и stubs: `src/lib/GERDA/gerda_security.h`. Сток ELRS UID **не** заменяется.
4. **Дальность/скорость** — adaptive MCS, FHSS с учётом помех, приоритет каналов управления, межпакетный FEC. Дизайн: `src/lib/GERDA/gerda_link.h`.

Архитектура: [`docs/ARCHITECTURE.ru.md`](docs/ARCHITECTURE.ru.md).

## Сборка

Нужны Python 3, Git, [PlatformIO](https://platformio.org/) (VS Code / CLI). Путь к репозиторию **без** кириллицы.

### PlatformIO (локально)

Откройте каталог **`src/`** как проект PlatformIO.

Минимум в `src/user_defines.txt` для RX 2.4 / LR1121:

```
-DMY_BINDING_PHRASE="ваша_фраза"
-DRegulatory_Domain_ISM_2400
-DAUTO_WIFI_ON_INTERVAL=60
-DLOCK_ON_FIRST_CONNECTION
```

Для LR1121 900 МГц-части unified-прошивки задайте 900-домен (например `-DRegulatory_Domain_FCC_915`) — иначе сборка 900/dual упрётся в `#error` в `targets.h`. Для чисто 2.4 LR1121 hop-таблица ISM2G4 живёт в `domainsDualBand[]`.

Среда приёмника FlyFish / generic C3 LR1121:

```text
Unified_ESP32C3_LR1121_RX_via_WIFI
Unified_ESP32C3_LR1121_RX_via_UART
Unified_ESP32C3_LR1121_RX_via_BetaflightPassthrough
```

```bash
cd src
pio run -e Unified_ESP32C3_LR1121_RX_via_UART
```

После сборки PlatformIO спросит (или примите через `board_config`) hardware path. Предпочтительно:

```text
generic.rx_dual.c3-plain
```

или, понимая риск пин-аута:

```text
flyfish.rx_dual.9624r
```

Бинарник: `src/.pio/build/<env>/firmware.bin`. Прошивка по Wi‑Fi: AP `GerdaLRS RX`, пароль `expresslrs`, `http://10.0.0.1/`.

Плейсхолдер TX: `Unified_ESP32_LR1121_TX_via_UART` / `_via_WIFI`.

### ExpressLRS Configurator, режим Local

1. Установите [ExpressLRS Configurator](https://github.com/ExpressLRS/ExpressLRS-Configurator/releases/).
2. Source: **Local**, укажите этот репозиторий.
3. RX: Radio = **LR1121**, MCU = **ESP32-C3**. Device: **Generic C3 LR1121 2.4/900 RX** (`generic.rx_dual.c3-plain`) либо **FlyFish 9624R 2.4**, если список подхватил `src/hardware/targets.json`.
4. Не выбирайте HappyModel / SX128x unified 2.4.

## Радио и закон

Использование частот **вне ISM 2.4 ГГц** (в том числе ~2640 МГц) может требовать лицензии или быть запрещено. **Ответственность на операторе.** Прошивка не заявляет соответствия нормам.

Фронтенд LR1121 на FlyFish 9624R согласован примерно на **2.4–2.5 ГГц**. CUSTOM_2640, скорее всего, ухудшит дальность без подходящей антенны и согласования.

`CUSTOM_2640` **выключен** по умолчанию. Не включайте `-DGERDA_DOMAIN_CUSTOM_2640`, пока не понимаете последствия и не прошиваете **и TX, и RX**.

## GPL-3.0

Исходники должны оставаться открытыми под GNU GPL v3. Не удаляйте `LICENSE` / `NOTICE`. Распространение бинарников требует Corresponding Source (GPL §6).

Upstream README ExpressLRS можно сравнить с https://github.com/ExpressLRS/ExpressLRS/blob/7684347ee697b3fa4318f99a02b6ae6f5d703307/README.md.

---

## English

GerdaLRS is a GPL-3.0 ExpressLRS derivative (upstream `7684347ee697b3fa4318f99a02b6ae6f5d703307`). Focus: Russian Web UI, optional selectable ISM 2.4 vs CUSTOM_2640, stronger-than-UID link security (design/stubs only), and planned range/speed work.

**Primary RX:** FlyFish 9624R 2.4 (ESP32-C3 + LR1121, single U.FL). Official path: `generic.rx_dual.c3-plain` → `Unified_ESP32C3_LR1121_RX` / `Generic C3 LR1121.json`. Unverified alias: `flyfish.rx_dual.9624r`. Do **not** flash SX128x HappyModel binaries. TX model unknown — use Generic LR1121 TX placeholders.

Non-ISM frequencies (including ~2640 MHz) are the operator’s legal responsibility. LR1121 2.4 matching is ~2.4–2.5 GHz; 2640 MHz will likely reduce range. Default RF remains stock ISM 2.4.

## Следующие шаги

- [ ] Подключить `i18n-ru.stub.js` к Web UI (вкладки, опции, предупреждения).
- [ ] Runtime-выбор ISM_2400 / CUSTOM_2640 в options JSON (оба конца линка).
- [ ] Снять `hardware.json` с живого FlyFish 9624R и сверить GPIO с generic layout.
- [ ] Назвать конкретную модель TX и добавить проверенный таргет.
- [ ] Спека OTA для KDF + HMAC + anti-replay **до** включения в пакеты.
- [ ] Native-тесты FHSS для CUSTOM_2640 и политика MCS/FEC.
