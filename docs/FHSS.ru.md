# Умный FHSS GerdaLRS

По умолчанию **выкл**. Hop-таблица — стоковый ExpressLRS: UID-seeded permutation `FHSSsequence[]` / `FHSSsequence_DualBand[]` в `src/lib/FHSS/FHSS.cpp`. TX Ranger Nano (SX1280) и RX FlyFish/generic C3 (LR1121) обязаны ходить **по одной и той же последовательности**, иначе линк разъедется.

## Что есть в радиостеке (не выдумка)

| Сигнал | Где | Заметка |
| --- | --- | --- |
| RSSI последнего пакета | `Radio.LastPacketRSSI` (`SX1280.cpp`, `LR1121.cpp` `GetLastPacketStats`) | после успешного RX |
| SNR | `Radio.LastPacketSNRRaw` | то же |
| HW CRC fail | `ProcessRFPacket` / `ProcessTLMpacket` `status != SX12XX_RX_OK` | IRQ пришёл, CRC радио нет |
| SW CRC fail | `OtaValidatePacketCrc` (и Gerda MAC XOR, если Secure ON) | пакет отброшен |
| Индекс канала | `FHSSsequence[FHSSptr]` или DualBand | 80 каналов на ISM 2.4 / CUST2640 |
| Sync-канал | `freq_count / 2` | каждые N хопов, для соединения |

Отдельного RSSI-на-хоп в стоковом ELRS **нет**. `FHSSgetNextFreq()` только инкрементирует указатель.

## v1 (этот PR) — soft denylist, без смены hops

Флаг: Web UI «Умный FHSS», `options.json` `"gerda-fhss"`, default 0.

1. Гистограмма на канал (до 80): hits, misses, EWMA RSSI.
2. Канал в denylist, если miss rate ≥ 50% при ≥ 8 сэмплах, это не sync-канал, и размер списка ≤ 25% таблицы (0 при < 8 каналах).
3. TX и RX **по-прежнему ходят** на denylist-частоту.
4. Miss на таком канале **не** снижает LQ (`LQCalc.add()` перед `inc()`).
5. RX не крутит `HandleFreqCorr` на denylist-хопе (шумный AFC).

Это помогает, когда часть ISM занята Wi‑Fi: LQ не падает на «законных» дырах, линк не расходится.

Для A/B дальности на паре Ranger Nano + FlyFish держите флаг **выкл** в первом прогоне (сток vs Gerda Дальность), затем включите, если трасса в городе. Hop CRC miss % и denylist видны в Web UI (`/config` → `gerda.fhss_miss`) после посадки. См. `docs/RANGE_EXPERIMENTS.ru.md`.

## Чего нет (честно)

**Синхронный skip-map** (TX не передаёт на плохом канале, RX не ждёт там) требует, чтобы оба конца имели **один** список. Иначе RX слушает пустоту → LQ/disconnect. Хелперы `gerda_fhss_export_denylist` / `import_denylist` готовы для редкого TLM/MSP, **в эфир не вшиты**.

Переставлять `FHSSsequence[]` только на RX нельзя.

## Файлы

- `src/lib/GERDA/gerda_fhss.*` — математика
- `rx_main.cpp` `ProcessRFPacket` / `HWtimerCallbackTick` / `HandleFreqCorr`
- `tx_main.cpp` `ProcessTLMpacket` / TLM LQ tick
- Native: `src/test/test_gerda/test_gerda.cpp` `test_smart_fhss_histogram_and_diversity`
