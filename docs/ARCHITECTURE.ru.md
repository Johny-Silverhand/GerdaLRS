# Архитектура GerdaLRS

Документ описывает **план модулей** поверх ExpressLRS `7684347`. Полный конкурент-стек не реализуется в этом PR: дерево должно оставаться собираемым и совместимым со стоковым ISM 2.4.

Связанные файлы:

| Модуль | Файлы |
| --- | --- |
| Конфиг / флаги | `src/include/gerda_config.h`, `src/user_defines.txt` |
| Локализация | `src/lib/GERDA/gerda_i18n.h`, `src/html/i18n-ru.stub.js` |
| Домены | `src/lib/GERDA/gerda_domain.h`, `src/lib/FHSS/FHSS.cpp` |
| Безопасность | `src/lib/GERDA/gerda_security.h` |
| Дальность/скорость | `src/lib/GERDA/gerda_link.h` |
| Железо | `docs/HARDWARE.ru.md`, `src/hardware/targets.json` |

## 1. База

```
GerdaLRS (этот репозиторий)
  ├── firmware  ← ExpressLRS @ 7684347ee697b3fa4318f99a02b6ae6f5d703307
  └── hardware  ← ExpressLRS/targets @ bda4c92e8a78459b4d3510f94f04dbdcbd0a5576
                  + алиас flyfish.rx_dual.9624r
```

OTA, UID, CRC, FHSS, unified binary+layout — как в ExpressLRS. GerdaLRS добавляет тонкий слой в `src/lib/GERDA/` и документацию.

## 2. Русский Web UI

Сток: HTML в `src/html/`, gzip+PROGMEM через `src/python/build_html.py`. На ESP32-C3 место ограничено.

**Слой локализации (план):**

1. Английские шаблоны остаются источником строк.
2. Словарь `src/html/i18n-ru.stub.js` (ключи = стабильный английский текст или `data-i18n`).
3. После загрузки DOM скрипт подменяет видимые строки.
4. Не дублировать целые `index.html` на русском.

Сейчас: брендинг «Герда / GerdaLRS» в шапке; словарь **не** подключён к `build_html.py`.

**Следующий PR:** `data-i18n` на вкладках Options/WiFi/Update/Model, переключатель языка, перевод предупреждения про bind-phrase.

## 3. Регуляторный домен: ISM_2400 + CUSTOM_2640

### Сток (этот commit)

Для `RADIO_LR1121` таблица 2.4 ГГц — `domainsDualBand[]` в `FHSS.cpp`:

```c
{"ISM2G4", 2400400000, 2479400000, 80, 2440000000}
```

Для SX128x то же 80 каналов в `domains[]`, имя `ISM2G4` или `CE_LBT`.

900 МГц: индекс домена в options (AU915, FCC915, …).

### План GerdaLRS

| Домен | Диапазон (план) | Как включается | Статус |
| --- | --- | --- | --- |
| ISM_2400 | 2400.4–2479.4 МГц, 80 хопов | по умолчанию | сток ELRS |
| CUSTOM_2640 | 2600.4–2679.4 МГц, 80 хопов (тот же span 79 МГц) | `-DGERDA_DOMAIN_CUSTOM_2640` | **заготовка**, не runtime |

Заготовка подменяет имя/частоты hop-таблицы 2.4 на этапе компиляции. Это тот же приём, что в любительских гайдах по правке `FHSS.cpp` (блок `const fhss_config_t domains`).

Позже: поле `gerda-domain` в options JSON и пункт Web UI. TX и RX обязаны совпадать, иначе линка не будет.

### RF caveat

LR1121 на 2.4-платах (FlyFish 9624R, типовой matching) рассчитан на **~2.4–2.5 ГГц**. На ~2640 МГц КСВ и мощность на антенне почти наверняка хуже. Это не «бесплатная дальность».

**Закон:** частоты вне ISM — ответственность пользователя. В репозитории нет обещания легальности CUSTOM_2640.

## 4. Безопасность линка (только дизайн)

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

Stubs: `gerda_kdf_session_key`, `gerda_hmac_tag`, `gerda_replay_check` всегда возвращают `GERDA_SEC_NOT_IMPLEMENTED` и **не** вызываются из `rx_main` / `tx_main`.

Пока stubs не впаяны в OTA, **нельзя** писать в UI «защищённый линк».

Порядок внедрения: спека размера пакета → native-тесты → RX verify → TX tag → UI. Ломать CRSF/airtime без расчёта нельзя.

## 5. Дальность и скорость (план)

| Фича | Идея | Риск |
| --- | --- | --- |
| Adaptive MCS | Менять rate/SF по LQ/SNR с гистерезисом | рассинхрон режимов TX/RX |
| Interference-aware FHSS | Реже ходить на шумные каналы | слишком узкий hop-set упрощает перехват |
| Приоритет управления | ARM / flight mode важнее телеметрии при плохом LQ | голод телеметрии |
| Межпакетный FEC | Чётность на N пакетов | воздух, латентность |

`gerda_link_feature_enabled()` сейчас всегда 0.

## 6. Потоки данных (целевые)

```
TX handset CRSF
  → OTA pack (+ future HMAC)
  → FHSS hop (ISM2G4 или CUST2640)
  → LR1121
RX LR1121
  → hop
  → HMAC/replay (future)
  → CRSF/SBUS к FC
Web UI (Wi-Fi AP GerdaLRS RX/TX)
  → i18n RU (future)
  → domain + options
```

## 7. Чеклист следующих шагов

- [ ] i18n: подключить словарь, перевести главные вкладки.
- [ ] Снять hardware.json с FlyFish 9624R и сравнить с `Generic C3 LR1121.json`.
- [ ] Runtime ISM vs 2640 в options; синхронизация TX/RX.
- [ ] Измерить RSSI/дальность на 2640 с штатной антенной (ожидается хуже).
- [ ] Зафиксировать модель TX, не оставлять только generic gemini.
- [ ] Спека session key / HMAC / nonce; затем код, не наоборот.
- [ ] Adaptive MCS только после таблицы поддерживаемых rates для LR1121.
- [ ] Не включать CUSTOM_2640 в дефолтный `user_defines.txt`.
