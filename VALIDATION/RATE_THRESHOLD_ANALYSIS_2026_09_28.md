# Rate-of-rise threshold analysis — SIH26215

## Why this was reviewed
The production firmware currently uses a 6.0 °C/min rate threshold over a five-sample ~8-second ESP32/SHT30 window for the fast heat-spike layer.

The historical Nepal dataset contains a `rate_c_per_min` feature based on its own logger cadence. Its distribution is much smaller:
- 90th percentile ≈ 0.005 °C/min
- 95th percentile ≈ 0.0125 °C/min
- 99th percentile ≈ 0.0275 °C/min
- maximum ≈ 0.655 °C/min

Using the historical rate feature to directly choose the live 2-second sensor threshold would be invalid because the time window and sensor dynamics differ.

## External-data experiment
For the Nepal dataset, a future-120-minute >8°C target was constructed separately by source file. Among rows with current temperature <8°C and a valid rate feature:
- 32,458 eligible rows
- 1,862 future-breach positives
- rate ROC-AUC ≈ 0.698

The best sample-level F1 found in a simple threshold sweep occurred near 0.0075 °C/min, but this threshold is **not** appropriate for direct use in the ESP32 because the live firmware measures rate over a much shorter window.

At 6.0 °C/min, the historical dataset contained no qualifying positive predictions. This is expected because the threshold is intentionally far above the historical logger-rate range.

## Engineering conclusion
Do not silently replace `6.0` with a historical-data threshold.

The current 6.0 °C/min setting should be described as a **high-rate thermal spike detector**, not a calibrated general warming detector.

A production-calibrated threshold should be selected from a controlled physical SHT30 noise/step-response dataset using the same firmware window and sampling interval. That experiment should measure:
1. stable 2–8°C noise;
2. bag opening / ambient disturbance;
3. controlled warming rates;
4. sensor placement and thermal lag;
5. false-positive rate versus threshold.

## Recommended mentor question
> “Should we keep the rate detector as a deliberately high-severity spike detector, or do you want us to calibrate its threshold from real SHT30 2-second thermal trials before the final field validation?”

## Current production decision
Keep the 6.0 °C/min threshold unchanged until physical calibration exists. Do not claim that 6.0 °C/min is an empirically optimal cold-chain threshold.
