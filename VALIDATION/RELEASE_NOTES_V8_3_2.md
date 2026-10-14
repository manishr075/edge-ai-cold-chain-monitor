# v8.3.2 Release Notes

## Added
- LittleFS interrupted-transaction recovery.
- Wi-Fi reconnect policy that avoids persistent NVS writes.
- Explicit v8.3.2 model/firmware identifier.
- More explicit product-specific wording for freeze detection and the 2–8°C demonstration band.
- Expanded rate-threshold calibration memo.
- Velxio demo uses the actual embedded Decision Tree and resets ML history cleanly at domain excursions.
- Velxio real mode enforces an 80% bucket-coverage gate.

## Verified
- Full firmware source-level Clang syntax check: PASS.
- Clang static analyzer: PASS with no diagnostics.
- ML cadence harness: PASS.
- Velxio source-level syntax check: PASS.
- ThingSpeak CA certificate fingerprint parsed and verified.
- Final deck scanned for stale 55.4/76.3 metrics and MQTT/Ubidots terminology: none found.

## Not changed
- No model retraining.
- No change to the frozen deployment tree.
- No change to the 0.85 threshold.
- No change to the 2-window persistence rule.
- No claim of physical ML validation.

## Next physical gate
Native ESP32 compile/upload, secure ThingSpeak publish, controlled 2–8°C runtime, and sensor/rate calibration.
