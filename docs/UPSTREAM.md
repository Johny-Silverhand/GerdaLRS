# Upstream pin

GerdaLRS is based on a specific ExpressLRS revision so firmware strings like
`master (768434)` can be traced to source.

| Tree | Repository | Full SHA | Notes |
| --- | --- | --- | --- |
| Firmware | https://github.com/ExpressLRS/ExpressLRS | `7684347ee697b3fa4318f99a02b6ae6f5d703307` | Short hash `768434`. Source tree imported into GerdaLRS (this repo does not carry the full ExpressLRS git ancestry). |
| Hardware targets | https://github.com/ExpressLRS/targets | `bda4c92e8a78459b4d3510f94f04dbdcbd0a5576` | 2025-04-24 snapshot, contemporaneous with the firmware pin. Vendored at `src/hardware/`. |

The user-reported RX Web UI string `master (768434) 2640` matches this firmware
short hash. The `2640` suffix is **not** in stock ExpressLRS at that commit;
stock 2.4 / LR1121 hopping remains ISM 2400–2479 MHz. GerdaLRS adds a runtime
CUSTOM_2640 hop table selected via Web UI `gerda-2g4` (see `docs/ARCHITECTURE.ru.md`).
Default remains ISM 2.4.

FlyFish 9624R was rechecked against ExpressLRS/targets `master`
`2b90c511e965304e333d99ca5f724dbc8b7c154f` (2026-09-11). There is still no
FlyFish / 9624R target or layout. v0.1.1-test stamps
`flyfish.rx_dual.9624r` onto `RX/Generic C3 LR1121.json`.

To inspect the exact upstream commit on GitHub:

```text
https://github.com/ExpressLRS/ExpressLRS/commit/7684347ee697b3fa4318f99a02b6ae6f5d703307
```
