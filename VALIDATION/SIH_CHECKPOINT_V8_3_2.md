# SIH26215 — CHECKPOINT v8.3.2

## Authoritative restart point

Use **v8.3.2** as the latest software checkpoint.

### Production firmware
`cold_chain_monitor_sih_ml_v8_3_2_secure_thingspeak.ino`

### Embedded model
`edge_ml_tree_v7.h`

### Demo firmware
`sketch_ml_velxio.ino` / Velxio ML demo package

### Presentation
`EDGE-AI-PREDICTIVE-COLD-CHAIN-MONITOR_SIH_UPDATED_V8_3_2.pptx`

## What changed from v8.3.1

- LittleFS interrupted-transaction recovery added.
- Wi-Fi persistent storage disabled for reconnect calls; auto-reconnect enabled.
- Freeze-latch wording corrected to avoid universal vaccine claims.
- Production version/model identifier updated to v8.3.2.
- Full source-level Clang syntax audit passed against ESP32/Arduino/FreeRTOS stubs.
- ThingSpeak CA certificate parsed and fingerprint checked.
- ML cadence edge-case harness rerun and passed.

## Current ML contract

- Target: >8°C within next 120 minutes.
- Current-bucket deployment domain: <8°C.
- Bucket size: 10 minutes.
- Minimum bucket coverage: 80%.
- Five valid buckets before first inference.
- Decision Tree threshold: 0.85.
- Two consecutive positive windows required for `ml_prediction = 1`.

## Current model evidence

Primary frozen historical Nepal holdout:
- 6,110 rows
- 12 held-out source files
- Precision 66.3%
- Recall 68.1%
- F1 67.2%
- ROC-AUC 0.907

These do not represent Bengaluru field accuracy.

## Current cloud architecture

ESP32 → HTTPS POST → `https://api.thingspeak.com/update`

- Write API Key
- 8 channel fields
- 15-second free-plan update pacing
- LittleFS offline backlog
- secure root CA validation

## Remaining blockers

### MUST HAVE before final physical demo
- Native ESP32 build/upload.
- ThingSpeak publish verification.
- Physical ML runtime in controlled 2–8°C environment.

### SHOULD HAVE
- Battery/power measurement.
- GPS satellite fix if location is included in live demo.
- Optional probability-calibration analysis.

### DO NOT CLAIM
- Bengaluru field accuracy.
- Guaranteed vaccine-life percentage for every vaccine.
- Real-time remote alerting while disconnected.
- Physical v8.3.2 ML validation until the controlled run is completed.
