# Прошивка GerdaLRS v0.1.1-test

Пара: **TX RadioMaster Ranger Nano 2.4 (SX1280)** + **RX FlyFish 9624R** (ESP32-C3 + LR1121, одна антенна).

Готовые `.bin`: [GitHub Releases](https://github.com/Johny-Silverhand/GerdaLRS/releases) → pre-release **v0.1.1-test**. Corresponding Source (GPL-3.0 §6) — этот репозиторий на том же теге.

`via_UART` и `via_WIFI` — **один и тот же** образ. Отдельных `*-via_WIFI.bin` в этом релизе нет.

## Что качать

| Файл | Куда |
| --- | --- |
| `GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.1-test.bin` | приёмник FlyFish 9624R. Имя в UI: `FlyFish 9624R 2.4`. Пин-аут: `Generic C3 LR1121.json` |
| `GerdaLRS-TX-RangerNano-2.4-v0.1.1-test.bin` | модуль Ranger Nano (Lua `RM Ranger Nano`) |

Не берите бины **v0.1.0-test**: в них вшита фраза `gerda-test-changeme`, и приёмник сбрасывает сохранённую привязку. Не берите SX128x / HappyModel RX и не берите `ranger-micro`, если Lua пишет Nano.

Проверьте `SHA256SUMS` рядом с файлами.

## Фраза привязки

В этих `.bin` **нет** `MY_BINDING_PHRASE` и нет ключа `uid` в options JSON.

На RX это значит: `firmwareOptions.hasUID == false`, `RxConfig::CheckUpdateFlashedUid()` сразу выходит и **не** вызывает `SetUID`. Привязка лежит в NVS (Arduino EEPROM, namespace `eeprom`), раздел отдельно от OTA-приложения. Прошивка по Wi‑Fi меняет только app-слот. Статус Persistent / Bound и байты UID остаются.

На TX отдельного хранилища bind нет. Без фразы UID берётся из MAC Wi‑Fi модуля (`setupBindingFromConfig` в `tx_main.cpp`). После прошивки TX задайте ту же фразу или те же 6 байт UID, что показывает RX. Пока этого нет, линка не будет. Модели Lua в NVS TX сохраняются, сам UID — нет.

Фразу на RX задают на вкладке **Модель** (сохраняется в NVS). На TX вкладки Модель нет: поле фразы на вкладке **Опции**. Текст фразы нигде не хранится, сохраняются только 6 байт UID.

## Способ для этого приёмника — Wi‑Fi из его текущего Web UI

Приёмник уже открыт (часто `http://10.0.0.1/`, либо тот адрес, который у вас в браузере). Это стоковый ExpressLRS `master (768434)`, вкладка **UPDATE** (в переводчике — «Обновление»).

1. Выберите `GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.1-test.bin`. Файл **не** распаковывать: ESP32-C3 ждёт сырой `.bin`, не `.gz`.
2. Дождитесь окончания. Светодиод должен снова замигать, прежде чем снимать питание.
3. Если UI пишет **Targets Mismatch** — см. ниже про `force`. Не подтверждайте, если «Uploaded image» — это TX-бин.
4. Снова зайдите в Web UI. Точка доступа станет `GerdaLRS RX`, пароль `expresslrs`, адрес по умолчанию `http://10.0.0.1/`.
5. Вкладка **Модель**: должно остаться «Привязан» и тот же UID. Новую фразу здесь **не** вводите, если хотите сохранить текущую привязку.

Почему UI обычно принимает файл: сверка идёт не по `product_name`, а по строке таргета. В прошивке она лежит как байты `\xBE\xEF\xCA\xFE` + `TARGET_NAME` (`options.cpp`). Этот бин собран env `Unified_ESP32C3_LR1121_RX`, поэтому внутри есть `UNIFIED_ESP32C3_LR1121_RX` — то же имя, что у unified-сборки master 768434 для C3 LR1121. `product_name` `FlyFish 9624R 2.4` только подпись в шапке.

Если вендор собрал **другой** `TARGET_NAME`, страница вернёт `mismatch` и покажет «Current target» / «Uploaded image». Кнопка **Flash anyway** шлёт `POST /forceupdate` с `action=confirm`: образ уже записан, подтверждение вызывает `Update.end()`. Отмена — `Update.abort()`. Отдельный query `?force` на `/update` делает то же самое (`request->hasArg("force")`). Образ, начинающийся с `0x1F` (gzip), проверка пропускает сама — для этого RX так делать не нужно.

## TX тем же релизом

Прошейте `GerdaLRS-TX-RangerNano-2.4-v0.1.1-test.bin` по Wi‑Fi (UPDATE на модуле) или UART:

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 GerdaLRS-TX-RangerNano-2.4-v0.1.1-test.bin
```

Внутри бина target `UNIFIED_ESP32_2400_TX` и прежнее имя `RADIOMASTER_RANGER_NANO_2400_TX`. Если UI всё равно пишет mismatch — **Flash anyway**, только убедившись, что это Nano, а не приёмник.

Дальше на TX, вкладка **Опции**:

- в поле фразы введите ту же фразу, что когда-то привязывала этот RX, **или** шесть чисел UID с вкладки Модель RX через запятую (например как они показаны на приёмнике);
- сохраните параметры и перезагрузите TX.

Оба конца: диапазон **ISM 2.4**. Пункт CUSTOM_2640 в списке есть, по умолчанию выключен (`gerda-2g4: 0`). Включать только на обоих сразу. Secure Link и умный FHSS по умолчанию выкл.

## UART на RX, если Wi‑Fi неудобен

```bash
esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x10000 GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.1-test.bin
```

Та же запись app-раздела (`0x10000`), NVS не стирается. Полный чип `erase_flash` **сотрёт** привязку — не делайте его.

## Сборка этих бинов

```bash
cd src
# user_defines.txt: фраза закомментирована,
# -DRegulatory_Domain_ISM_2400 и -DRegulatory_Domain_FCC_915
# -DGERDA_DOMAIN_CUSTOM_2640 не задан (2640 остаётся пунктом Web UI)
test ! -f super_defines.txt
pio run -e Unified_ESP32C3_LR1121_RX_via_UART
pio run -e Unified_ESP32_2400_TX_via_UART
python python/stamp_gerda_release.py \
  .pio/build/Unified_ESP32C3_LR1121_RX_via_UART/firmware.bin \
  ../dist/GerdaLRS-RX-FlyFish9624R-ESP32C3-LR1121-v0.1.1-test.bin \
  flyfish.rx_dual.9624r
python python/stamp_gerda_release.py \
  .pio/build/Unified_ESP32_2400_TX_via_UART/firmware.bin \
  ../dist/GerdaLRS-TX-RangerNano-2.4-v0.1.1-test.bin \
  radiomaster.tx_2400.ranger-nano
```

В логе сборки должна быть строка `no binding phrase; RX NVS UID will be left untouched`. В options JSON штампа не должно быть ключа `uid`. `gerda-2g4` равен `0`.

Native: `PLATFORMIO_BUILD_FLAGS="-DRegulatory_Domain_ISM_2400" pio test -e native`.

## Риски

- Пин-аут 9624R в upstream нет. Используется generic C3 LR1121. Если GPIO не совпадут, радио может молчать, чип по Wi‑Fi обычно жив.
- Если нажать «сохранить» на вкладке Модель RX с новой фразой, UID перезапишется уже после прошивки.
- TX без введённой фразы/UID сидит на своём MAC и со старым RX не свяжется.
- `erase_flash` и прошивка v0.1.0-test сбрасывают привязку.
- CUSTOM_2640 на этом фронтенде дальность не увеличивает.
