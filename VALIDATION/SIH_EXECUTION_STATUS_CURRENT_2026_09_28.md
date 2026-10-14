# SIH26215 — Current Execution Status — 2026-09-28

## Latest checkpoint
**v8.3.2** secure ThingSpeak production firmware + real embedded Decision Tree.

## Completed in this continuation
- Reviewed the full v8.3.1 production source.
- Added LittleFS startup transaction recovery.
- Disabled persistent Wi-Fi credential writes during reconnects and enabled auto-reconnect.
- Scoped freeze-latch wording to the demonstration target instead of making a universal vaccine claim.
- Updated production version/model identifier to v8.3.2.
- Ran full-source Clang syntax checking through an Arduino/ESP32/FreeRTOS stub layer: PASS.
- Ran Clang static analyzer: PASS with no diagnostics.
- Reran ML cadence edge-case tests: PASS.
- Verified the embedded ThingSpeak CA certificate using OpenSSL: DigiCert Global Root CA, SHA-256 fingerprint `43:48:A0:E9:44:4C:78:CB:26:5E:05:8D:5E:89:44:B4:D8:4F:96:62:BD:26:DB:25:7F:89:34:A4:43:C7:01:61`, validity through 2031-11-10.
- Verified current ThingSpeak API limits/documentation online.
- Reviewed the historical Nepal rate distribution and performed a separate future-120-minute breach/rate-threshold analysis.
- Determined that the current 6.0°C/min live rate threshold should NOT be replaced with the historical logger-rate threshold because the time window/cadence differ. Created a calibration memo instead.
- Fixed Velxio real-mode bucket coverage handling and a double-counting risk in fast anomaly state.
- Fixed Velxio DEMO_MODE domain-reset behavior and added clean looping for repeated demonstrations.
- Recompiled the Velxio source through the DHTesp/FreeRTOS stub layer: PASS.
- Updated judge-defense and manual-validation materials for v8.3.2.
- Updated the mentor presentation source line to v8.3.2 and rendered a 10-page PDF successfully.

## Current model evidence
Frozen Nepal historical-data holdout:
- 6,110 rows
- 12 held-out source files
- Precision 66.3%
- Recall 68.1%
- F1 67.2%
- ROC-AUC 0.907

Do not describe these as Bengaluru field accuracy.

## Current ML contract
- 10-minute model cadence
- 80% minimum bucket coverage
- 5 valid buckets before first inference
- target: >8°C within 120 minutes
- current bucket must be <8°C for learned inference
- threshold 0.85
- two consecutive positive windows for `ml_prediction=1`

## Current cloud contract
- ThingSpeak REST/HTTPS
- `https://api.thingspeak.com/update`
- 8 fields/message
- free update interval: 15 seconds
- persistent LittleFS offline backlog
- secure CA validation enabled

## Latest Velxio demo behavior
Representative actual-tree outputs from the same feature-generation path:
- 5.00°C → 0.046775, no prediction
- 5.40°C → 0.977590, first positive
- 5.80°C → 0.838533, streak reset
- 6.20°C → 0.937800, first positive
- 6.60°C → 0.995853, second positive → predictive warning
- 7.90°C → 0.939685, warning persists
- 8.10°C → domain guard / actual-breach layer

## Remaining work
### MUST HAVE
- Native ESP32 compile/upload of v8.3.2.
- Secure ThingSpeak publish verification.
- Controlled 2–8°C physical ML-runtime test.

### SHOULD HAVE
- Physical SHT30 rate/noise calibration of the 6.0°C/min spike threshold.
- Battery/power measurement.
- GPS fix if location is shown.

### DO NOT CLAIM
- Bengaluru field accuracy.
- Universal vaccine-life percentage from the generic VVM7 demonstration model.
- Real-time remote alerting during a connectivity outage.
- Physical v8.3.2 ML validation before the controlled run.

## Key engineering decision
Do not continue changing the trained Decision Tree merely to add complexity. The highest-value remaining work is physical validation and threshold calibration using the same sensor/cadence as deployment.
