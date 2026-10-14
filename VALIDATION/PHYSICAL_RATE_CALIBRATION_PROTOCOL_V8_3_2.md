# Physical rate-detector calibration protocol — v8.3.2

## Purpose
Calibrate the **fast rule-based spike detector** without changing the trained Decision Tree.

The SHT31/SHT3x family is specified for fast digital measurement; Sensirion lists typical SHT31 temperature accuracy of 0.2°C and a temperature response time of >2 s for SHT31-DIS-F, so a 2-second sample interval is plausible for sensing, but the installed sensor's thermal coupling to the cold-chain container can dominate the observed response. urlSensirion SHT31-DIS-F specificationhttps://sensirion.com/products/catalog/SHT31-DIS-F

## Current firmware rule
- Sample interval: 2 s
- Window: 5 samples
- Approximate elapsed window: 8 s
- Fast spike threshold: 6.0 °C/min
- Persistence: 3 consecutive samples

The 6.0 °C/min value is intentionally treated as a high-severity spike threshold, not a calibrated general warming threshold.

## Experiment A — stable baseline
1. Put the sensor inside the intended enclosure/container at a stable safe temperature.
2. Log at least 10 minutes at 2-second cadence.
3. Measure the five-sample rate for every window.
4. Record maximum, 99th percentile and standard deviation.

## Experiment B — controlled warming
Perform several controlled warming trajectories, ideally by changing the ambient/container condition gradually rather than applying a direct heat source to the sensor.

Suggested trajectory classes:
- slow: ~0.005–0.01 °C/min
- moderate: ~0.02–0.05 °C/min
- fast: ~0.05–0.15 °C/min
- abrupt disturbance: a short large step

These are experiment labels, not recommended production thresholds.

## Experiment C — false-trigger disturbances
Repeat the stable condition while introducing representative disturbances:
- opening/closing the container;
- moving the sensor;
- brief ambient-air exposure;
- handling/jostling;
- changing the sensor position near the payload.

Record whether the 3-sample persistence rule fires.

## Threshold selection
Choose the threshold using measured data from the same sensor, placement and 2-second firmware window.

For each candidate threshold, report:
- false positive rate under stable conditions;
- detection rate for controlled warming;
- warning lead time;
- number of consecutive alerts;
- maximum sensor overshoot/noise.

Do not select the threshold from the historical Nepal logger's `rate_c_per_min` values without cadence normalization. That dataset's rate feature was generated at a different sampling cadence.

## Deliverable
A one-page table:

| Threshold | Stable false positives | Warming detections | Median lead | Decision |
|---:|---:|---:|---:|---|
| candidate 1 | … | … | … | … |
| candidate 2 | … | … | … | … |
| candidate 3 | … | … | … | … |

## Mentor question
“Should we optimize the 6.0 °C/min detector for early warming detection, or keep it as a deliberate high-severity spike detector and let the trained Decision Tree handle normal predictive warming?”
