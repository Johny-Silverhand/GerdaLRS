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

Поиск по `ExpressLRS/ExpressLRS` и `ExpressLRS/targets` (снимок `bda4c92` и более новый master на момент инициализации) **не** находит `FlyFish` / `9624R`.

Значит, строка `FlyFish 9624R 2.4` пришла из вендорского `product_name` в прошивке продавца, а не из публичного `targets.json`.

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

```
flyfish.rx_dual.9624r
  product_name : FlyFish 9624R 2.4
  lua_name     : FlyFish 9624R
  firmware     : Unified_ESP32C3_LR1121_RX
  layout_file  : Generic C3 LR1121.json   # тот же файл, что у generic
```

**Риск:** пин-аут не верифицирован. Прошивка алиаса не должна «кирпичить» ESP32-C3 (Wi‑Fi/UART boot обычно живы), но RF может молчать. Снимите конфиг с рабочего RX: Web UI → Hardware → экспорт JSON, сравните с `src/hardware/RX/Generic C3 LR1121.json`.

Похожий серийный продукт с тем же layout/firmware: `radiomaster.rx_dual.xr2` (RadioMaster XR2 2G4 RX). Это **не** доказательство совместимости FlyFish.

## TX: RadioMaster Ranger Nano 2.4 (v1)

Lua: **RM Ranger Nano**. На пластике модуля может быть написано Ranger Micro — для прошивки используйте Nano.

- Радио: **SX1280** (не LR1121)
- `radiomaster.tx_2400.ranger-nano`
- `product_name`: RadioMaster Ranger Nano 2.4GHz TX
- `firmware`: Unified_ESP32_2400_TX
- `layout_file`: Radiomaster Ranger Micro.json + overlay `power_values` [-18,-15,-12,-8,-5,0]
- Env: `Unified_ESP32_2400_TX_via_UART` / `_via_WIFI`

Не путать с `radiomaster.tx_2400.ranger-micro` (Lua `RM Ranger Micro`).

**Совместимость с FlyFish LR1121 RX:** обычные пакетные режимы ELRS 2.4 (например 50–500 Hz в зависимости от версии) работают cross-chip. DK500/K1000 и прочие LR1121-only режимы на этом TX недоступны.

Пример Lua (только документация): 50 Hz, Telem 1:16, Switch Wide, Link Normal, Model Match Off, 1000 mW. LQ 0/50 на скрине = нет линка в тот момент.

## RF: 2.4 vs 2640

Типовой 2.4 ГГц фронтенд LR1121 (фильтры, matching, штатная антенна 2.4) работает около **2400–2500 МГц**. Сдвиг hop-таблицы к ~2640 МГц:

- может быть вне полосы антенны и ФНЧ/ПФ;
- обычно снижает EIRP и чувствительность;
- не делает линк «дальше» сам по себе.

Включайте CUSTOM_2640 только осознанно, на **обоих** концах, и только если это законно у вас.
