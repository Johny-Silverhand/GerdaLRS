# Прошивка GerdaLRS v0.1.0-test

Пара v1: **TX RadioMaster Ranger Nano 2.4 (SX1280)** + **RX FlyFish 9624R / Generic ESP32-C3 LR1121**.

Готовые `.bin`: [GitHub Releases](https://github.com/Johny-Silverhand/GerdaLRS/releases) → pre-release **v0.1.0-test**. Corresponding Source (GPL-3.0 §6) — этот репозиторий на том же теге.

`via_UART` и `via_WIFI` — **один и тот же** образ. Способ заливки выбираете вы.

## Что качать

| Файл | Куда |
| --- | --- |
| `GerdaLRS-TX-RangerNano-2.4-v0.1.0-test.bin` | модуль Ranger Nano (Lua `RM Ranger Nano`) |
| `GerdaLRS-RX-Generic-ESP32C3-LR1121-v0.1.0-test.bin` | **предпочтительный** RX: layout `Generic C3 LR1121.json` |
| `GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.0-test.bin` | то же железо, другое имя в UI (`FlyFish 9624R 2.4`). Пин-аут **не** снят с живой платы |

Не берите SX128x / HappyModel RX и не берите `ranger-micro`, если Lua пишет Nano.

## ⚠ Фраза привязки в этих .bin

Релизные бинарники собраны с тестовой фразой:

```
gerda-test-changeme
```

Она **не** секрет. Любой, кто скачал тот же релиз, имеет тот же UID. Для модели пересоберите с `-DMY_BINDING_PHRASE="ваша_фраза"` на **обоих** концах или смените фразу в Web UI (Опции) и перезагрузите TX и RX.

Без своей фразы: только стенд / первый прошив. Secure Link и Умный FHSS в этих бинах **выкл**.

## Способ 1 — ExpressLRS Configurator, Local (проще)

1. [Configurator](https://github.com/ExpressLRS/ExpressLRS-Configurator/releases/).
2. Source: **Local**, каталог этого репозитория (ветка/тег `v0.1.0-test`).
3. TX: Device **RadioMaster Ranger Nano 2.4GHz TX**. Своя binding phrase.
4. RX: Radio **LR1121**, MCU **ESP32-C3**, Device **Generic C3 LR1121 2.4/900 RX** (или FlyFish 9624R 2.4 — имя, пин-аут generic).
5. Domain: ISM 2.4 (+ любой 900, например FCC 915, только чтобы RX собрался).
6. UART к модулю / RX, Flash.

Так вы **не** получаете тестовую фразу из GitHub `.bin` — Configurator печёт ваш UID.

## Способ 2 — готовый .bin по UART (esptool)

Нужны Python 3 и `pip install esptool`. Модуль в режиме загрузчика (кнопка boot / USB-C у Nano).

**TX (ESP32), уже есть загрузчик ELRS / предыдущая прошивка:**

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 GerdaLRS-TX-RangerNano-2.4-v0.1.0-test.bin
```

**RX (ESP32-C3):**

```bash
esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 GerdaLRS-RX-Generic-ESP32C3-LR1121-v0.1.0-test.bin
```

Если устройство «пустое» (нет bootloader), соберите локально `pio run` и прошейте полный набор из `src/.pio/build/<env>/` (`bootloader.bin` + `partitions.bin` + `boot_app0.bin` + `firmware.bin`) как делает PlatformIO, либо используйте Configurator.

Порты Windows: `COM3` и т.п. Скорость 460800; при сбоях — 115200.

## Способ 3 — Wi‑Fi (уже крутится ELRS/Gerda)

1. Включите Wi‑Fi на модуле (Lua «Enable WiFi» или 60 с без линка).
2. Сеть `GerdaLRS TX` / `GerdaLRS RX` (или старый `ExpressLRS …`), пароль `expresslrs`.
3. Браузер `http://10.0.0.1/` → вкладка **Обновление** → выберите соответствующий `.bin`.

После смены железа/имени цели Configurator иногда требует «force».

## После прошивки

1. AP: `GerdaLRS TX` / `GerdaLRS RX`, пароль `expresslrs`, `http://10.0.0.1/`.
2. Опции: диапазон **ISM 2.4** на обоих концах. CUSTOM_2640 не для дальности.
3. Профиль для дальних тестов: **Дальность**. Secure / Умный FHSS — выкл, пока не решите иначе.
4. Lua на аппаратуре: 50 Hz, Telem 1:16, Switch Wide, Link Normal, Model Match Off, 1000 mW, Dynamic Power On. См. [`RANGE_EXPERIMENTS.ru.md`](RANGE_EXPERIMENTS.ru.md).

## Сборка своих .bin

```bash
cd src
# свои defines: фраза + домены уже в user_defines.txt (ISM_2400 + FCC_915)
# раскомментируйте -DMY_BINDING_PHRASE="ваша_фраза"
pio run -e Unified_ESP32_2400_TX_via_UART \
  --project-option="board_config=radiomaster.tx_2400.ranger-nano"
pio run -e Unified_ESP32C3_LR1121_RX_via_UART \
  --project-option="board_config=generic.rx_dual.c3-plain"
```

Артефакт: `src/.pio/build/<env>/firmware.bin` (уже с layout и options).

## Риски

- Алиас FlyFish **не** верифицирован дампом `hardware.json` с платы. Если GPIO не совпадут — RF/LED/кнопка могут молчать; ESP32-C3 обычно жив по USB/UART/Wi‑Fi. Безопаснее generic C3 LR1121, затем снять Hardware с рабочего RX.
- Тестовая фраза в GitHub `.bin` общая.
- 750/900 как «дальность» в этом релизе нет. On-air FEC нет.
