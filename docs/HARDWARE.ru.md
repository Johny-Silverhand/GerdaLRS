# Железо GerdaLRS

## FlyFish 9624R 2.4 (основной RX)

Проверено у пользователя:

- Маркировка платы: **9624R1**
- MCU: **ESP32-C3**
- RF: **Semtech LR1121**
- Антенна: **один U.FL** (не true diversity)
- Web UI target: `FlyFish 9624R 2.4`
- Прошивка на устройстве: `master (768434) 2640`

`2640` в строке версии — не стоковый ExpressLRS на `7684347` (там 2.4 = ISM2G4). Это метка кастомной сборки; в GerdaLRS CUSTOM_2640 пока только заготовка.

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

## TX (плейсхолдеры)

Модель передатчика неизвестна. Пока:

```
generic.tx_dual.gemini
  product_name : Gemini XrossBand 2.4/900 TX
  firmware     : Unified_ESP32_LR1121_TX
  layout_file  : TX/Generic LR1121 Gemini.json
```

Env: `Unified_ESP32_LR1121_TX_via_UART` | `_via_WIFI` | `_via_ETX`.

Gemini layout предполагает разводку generic dual-band TX, не вашу конкретную головку. Прошивка «наугад» может не поднять радио или вентилятор/кнопки. Когда появится точная модель — добавим таргет по схеме, а не по догадке.

## RF: 2.4 vs 2640

Типовой 2.4 ГГц фронтенд LR1121 (фильтры, matching, штатная антенна 2.4) работает около **2400–2500 МГц**. Сдвиг hop-таблицы к ~2640 МГц:

- может быть вне полосы антенны и ФНЧ/ПФ;
- обычно снижает EIRP и чувствительность;
- не делает линк «дальше» сам по себе.

Включайте CUSTOM_2640 только осознанно, на **обоих** концах, и только если это законно у вас.
