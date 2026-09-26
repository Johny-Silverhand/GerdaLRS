# Железо GerdaLRS

## FlyFish 9624R 2.4 (основной RX)

Проверено у пользователя:

- Маркировка платы: **9624R1**
- MCU: **ESP32-C3**
- RF: **Semtech LR1121**
- Антенна: **один U.FL** (не true diversity)
- Web UI target: `FlyFish 9624R 2.4`
- Прошивка на устройстве: `master (768434) 2640`

`2640` в строке версии — не стоковый ExpressLRS на `7684347` (там 2.4 = ISM2G4). Это метка кастомной сборки. В GerdaLRS CUSTOM_2640 выбирается в Web UI (`gerda-2g4`), по умолчанию ISM 2.4.

## Чего нет в официальном ExpressLRS

Поиск по `ExpressLRS/targets` `master` на 2026-09-11 (`2b90c511e965304e333d99ca5f724dbc8b7c154f`, файл `targets.json` и каталог `RX/`) даёт **ноль** вхождений `FlyFish` и `9624`. В `ExpressLRS/ExpressLRS` на пине `7684347` отдельного layout тоже нет.

Строка `FlyFish 9624R 2.4` на живом приёмнике — это `product_name` в хвосте прошивки (128 байт после кода), а не отдельный PIO-таргет. Её ставит тот, кто штамповал unified-образ. Публичного `layout_file` именно для 9624R нет.

### Пин-аут, который использует v0.1.1-test

Отдельного layout нет, поэтому RX штампуется как `flyfish.rx_dual.9624r`:

- `product_name`: `FlyFish 9624R 2.4` (та же строка, что в шапке стокового Web UI)
- `lua_name`: `FlyFish 9624R`
- `firmware`: `Unified_ESP32C3_LR1121_RX` (compile-time target `UNIFIED_ESP32C3_LR1121_RX`)
- `layout_file`: `RX/Generic C3 LR1121.json` — тот же файл, что у `generic.rx_dual.c3-plain`

GPIO из этого файла (снимок targets `bda4c92`, без правок):

| Сигнал | GPIO |
| --- | --- |
| UART RX / TX | 20 / 21 |
| SPI MISO / MOSI / SCK | 5 / 4 / 6 |
| radio busy / dio1 / nss / rst | 3 / 1 / 7 / 2 |
| RGB LED (GRB) | 8 |
| кнопка | 9 |

`power_values` `[12,16,19,22]`, `radio_dcdc` true. На upstream master к этому JSON добавлен только `"power_lna_gain": 12`. v0.1.1-test **не** подхватывает эту строку: смещение LBT без дампа платы не меняем.

Это не дамп 9624R1. Если GPIO на плате другие, RF/LED/кнопка могут молчать; ESP32-C3 по Wi‑Fi/UART обычно остаётся живым.

## Куда собирать

Не используйте SX128x-таргеты. Ранее циркулировавший `.bin` **UNIFIED_ESP32_2400_RX / HappyModel / SX128x** к этой плате **не** относится и может оставить радио мёртвым.

### Рекомендуемый официальный путь (меньший риск «выдуманного» железа)

```
generic.rx_dual.c3-plain
  product_name : Generic C3 LR1121 2.4/900 RX
  lua_name     : C3 LR1121 RX
  platform     : esp32-c3
  firmware     : Unified_ESP32C3_LR1121_RX
  layout_file  : RX/Generic C3 LR1121.json
```

PlatformIO env:

- `Unified_ESP32C3_LR1121_RX_via_UART`
- `Unified_ESP32C3_LR1121_RX_via_WIFI`
- `Unified_ESP32C3_LR1121_RX_via_BetaflightPassthrough`

Почему `rx_dual`, а не `rx_2400`: unified LR1121 ищется как frequency=`dual` (`UnifiedConfiguration.py`: имя прошивки без `_2400_`/`_900_`). Категория `generic.rx_2400.c3-plain` — это **SX128x** (`Unified_ESP32C3_2400_RX`), другой чип.

Layout `Generic C3 LR1121.json` (снимок bda4c92): UART 20/21, SPI 4/5/6, radio busy/dio1/nss/rst = 3/1/7/2, LED RGB=8, button=9. Это **generic** разводка, не дамп 9624R1.

### Алиас GerdaLRS (имя как на устройстве)

v0.1.1-test прошивает **этот** путь, не отдельный generic-бин:

```
flyfish.rx_dual.9624r
  product_name : FlyFish 9624R 2.4
  lua_name     : FlyFish 9624R
  firmware     : Unified_ESP32C3_LR1121_RX
  layout_file  : Generic C3 LR1121.json   # тот же файл, что у generic
```

**Риск:** пин-аут не снят с живой платы. Прошивка не должна «кирпичить» ESP32-C3 (Wi‑Fi/UART boot обычно живы), но RF может молчать. Снимите конфиг с рабочего RX: Web UI → Hardware → экспорт JSON, сравните с `src/hardware/RX/Generic C3 LR1121.json`.

Похожий серийный продукт с тем же layout/firmware: `radiomaster.rx_dual.xr2` (RadioMaster XR2 2G4 RX). Это **не** доказательство совместимости FlyFish.

## TX: RadioMaster Ranger Nano 2.4 (v1)

Lua на радио: **RM Ranger Nano**. На пластике/фото модуль может быть подписан как Ranger Micro (JR-bay, USB-C, XT30 6–16.8 V, вентилятор, RP-SMA). Для прошивки используйте **Lua-имя**.

Радио: **SX1280** (не LR1121). Семья Ranger 2.4.

В снимке `src/hardware/targets.json` (ExpressLRS/targets `bda4c92`) таргет **есть**:

```
radiomaster.tx_2400.ranger-nano
  product_name : RadioMaster Ranger Nano 2.4GHz TX
  lua_name     : RM Ranger Nano
  platform     : esp32
  firmware     : Unified_ESP32_2400_TX
  layout_file  : TX/Radiomaster Ranger Micro.json
  overlay      : power_values [-18,-15,-12,-8,-5,0]
  upload       : uart, wifi
  prior_target_name : RadioMaster_Ranger_Nano_2400_TX
```

Configurator (Local): Device **RadioMaster Ranger Nano 2.4GHz TX**. PlatformIO: `Unified_ESP32_2400_TX_via_UART` / `_via_WIFI`.

Не путать с `radiomaster.tx_2400.ranger-micro` (Lua `RM Ranger Micro`) — тот же layout без nano overlay мощности.

**Совместимость с FlyFish LR1121 RX:** стандартные режимы ELRS 2.4 работают cross-chip. DK500 / K1000 и прочие режимы только для LR1121 требуют LR1121 на **обоих** концах — для Ranger Nano это **вне v1**.

Пример Lua (только документация, не дефолт прошивки): Packet Rate 50 Hz, Telem Std 1:16, Switch Wide, Link Normal, Model Match Off, TX Power 1000 mW. LQ 0/50 на скрине = нет линка в тот момент.

## RF: 2.4 vs 2640

Типовой 2.4 ГГц фронтенд LR1121 (фильтры, matching, штатная антенна 2.4) работает около **2400–2500 МГц**. Сдвиг hop-таблицы к ~2640 МГц:

- может быть вне полосы антенны и ФНЧ/ПФ;
- обычно снижает EIRP и чувствительность;
- не делает линк «дальше» сам по себе.

Включайте CUSTOM_2640 только осознанно, на **обоих** концах, и только если это законно у вас.
