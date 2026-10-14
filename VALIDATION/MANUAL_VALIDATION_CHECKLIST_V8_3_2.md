# Manual Validation Checklist — v8.3.2

## P0 — Required before final technical claim
- [ ] Upload v8.3.2 secure firmware.
- [ ] Confirm secure HTTPS boot message.
- [ ] Confirm first positive ThingSpeak entry ID.
- [ ] Confirm ThingSpeak timestamps are at least 15 s apart.
- [ ] Leave device running without reboot for 60 min.
- [ ] Capture `ml_data_quality`, `ml_probability`, `ml_prediction`, `model_version`.

## P1 — ML runtime
- [ ] Use a valid 2–8°C environment.
- [ ] Keep sensor placement fixed.
- [ ] Record start/end temperature.
- [ ] Confirm first ML inference occurs after valid history accumulation.
- [ ] Record any positive model streak.

## P2 — Controlled trajectory
- [ ] Stable condition.
- [ ] Gradual warming.
- [ ] Faster warming.
- [ ] Crossing of 8°C.
- [ ] Recovery/cooling after excursion.
- [ ] Record false-positive/false-negative observations rather than estimating them.

## P3 — Reliability
- [ ] Brief Wi-Fi disconnect.
- [ ] Confirm LittleFS backlog appears.
- [ ] Restore Wi-Fi.
- [ ] Confirm backlog drains one record per cloud-rate slot.
- [ ] Confirm sensor task continues during the outage.

## P4 — Rate-rule calibration (recommended)
- [ ] Measure stable SHT30 2-second noise for at least 10 minutes.
- [ ] Perform a controlled warming step and log the same five-sample rate window used by firmware.
- [ ] Compare observed rate distribution with the 6.0 °C/min spike threshold.
- [ ] Do not change the threshold without recording the resulting false-positive/false-negative behavior.

## P5 — GPS
- [ ] Outdoor open-sky test.
- [ ] Record satellite count and fix validity.
- [ ] Do not block thermal demo on GPS.
