# GerdaLRS v0.1.1-test

Пре-релиз под живой приёмник **FlyFish 9624R 2.4** (ESP32-C3 + LR1121, Web UI стокового ExpressLRS `master (768434)`) и парный **TX RadioMaster Ranger Nano 2.4**.

Corresponding Source (GPL-3.0 §6): этот репозиторий, тег `v0.1.1-test`.

В бинах **нет** вшитой фразы привязки. Релиз v0.1.0-test с фразой `gerda-test-changeme` для этой пары не использовать: он затирает сохранённый UID.

## Файлы

| Файл | Куда |
| --- | --- |
| `GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.1-test.bin` | RX FlyFish 9624R по Wi‑Fi из его Web UI |
| `GerdaLRS-TX-RangerNano-2.4-v0.1.1-test.bin` | TX Ranger Nano |
| `SHA256SUMS` | контрольные суммы |

Один и тот же `.bin` и для Wi‑Fi, и для UART (`write_flash 0x10000`). Не gzip.

## Пин-аут

В ExpressLRS/targets `master` (`2b90c51`, 2026-09-11) **нет** layout FlyFish 9624R. RX собран как unified `Unified_ESP32C3_LR1121_RX` и проштампован `flyfish.rx_dual.9624r`:

- имя в шапке: `FlyFish 9624R 2.4`
- пины: `src/hardware/RX/Generic C3 LR1121.json` (UART 20/21, SPI 5/4/6, busy/dio1/nss/rst = 3/1/7/2, RGB 8, кнопка 9)
- строка `power_lna_gain` из более нового upstream **не** взята

Target, который ищет Web UI при заливке: `UNIFIED_ESP32C3_LR1121_RX`. Это не `product_name`. Если страница пишет Targets Mismatch, кнопка **Flash anyway** (это `POST /forceupdate`, `action=confirm`).

## Привязка

RX: без ключа `uid` в прошивке `hasUID` ложен, `CheckUpdateFlashedUid` UID не трогает. NVS переживает Wi‑Fi-обновление. На вкладке Модель не вводите новую фразу, если нужно оставить текущий Persistent bind.

TX: без фразы берёт MAC. После прошивки тем же релизом откройте Опции и введите ту же фразу или шесть байт UID с приёмника, сохраните и перезагрузите. Пока UID не совпадут, связи не будет.

## Регион

ISM 2.4 по умолчанию (`gerda-2g4: 0`). Пункт CUSTOM_2640 в Web UI сохранён и выключен. Secure Link и умный FHSS выключены.

Короткая инструкция: [`docs/FLASH.ru.md`](https://github.com/Johny-Silverhand/GerdaLRS/blob/v0.1.1-test/docs/FLASH.ru.md).
