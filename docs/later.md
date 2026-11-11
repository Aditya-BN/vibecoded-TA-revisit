# Saved for later

## Battery life on one 18650 (estimates, not measured)
Assumed 3000 mAh cell, ~2400 mAh usable.

| Scenario | Avg current | Runtime |
|---|---|---|
| Current code (`SLEEP_SECONDS 10`) | ~180 mA | ~13 h |
| Sleep 10 min, same hardware | ~60 mA | ~1.5 days |
| + GPS and OLED off while sleeping, bare ESP32 module, good regulator | ~18 mA | ~5–6 days |
| + faster wake-up (50 ms ADS gap, cached WiFi/port/time, GPS backup battery) | ~2.3 mA | ~6 weeks |
| same, one reading every 30 min | ~0.9 mA | ~3 months |

Takeaways:
- Sleep current dominates: GPS (~40 mA), OLED (~10 mA) and the DevKit's USB chip/LDO/LED (~5–10 mA) stay on while the ESP32 sleeps.
- DevKit's AMS1117 LDO needs ~4.4 V in, an 18650 sits at 3.6–3.9 V → brownouts. That is probably why codingTA disabled the brownout detector. Use a low-dropout regulator (HT7833 or similar) or a buck-boost module.
- Dual core makes no real difference to battery life.

## To do
- [ ] INA219 current logging behind a `#define` switch, to replace these estimates with measured numbers.
- [ ] Optional `USE_DUAL_CORE` (upload task on core 0 + FreeRTOS queue) for awake mode.
- [ ] Power optimisations from the table above.
- [ ] Re-calibrate ADS1115 pH with buffer solutions (formula is in volts, reads ~7.5 always).
