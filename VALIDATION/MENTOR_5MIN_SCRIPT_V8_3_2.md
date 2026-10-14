# 5-Minute Mentor Presentation Script — v8.3.2

## 0:00–0:30 — Problem
“During last-mile healthcare logistics, temperature-sensitive products can move toward or outside their required thermal range. A conventional logger tells us what happened. Our objective is to detect the trajectory early enough to support intervention.

Our SIH26215 prototype is a low-cost, edge-first cold-chain monitor built around an ESP32.”

## 0:30–1:10 — System
“The device uses an SHT30 for temperature and humidity, an optional NEO-6M GPS for traceability, FreeRTOS for task separation, and LittleFS for offline telemetry buffering.

The ESP32 samples locally every two seconds. Critical sensing does not wait for cloud communication.”

## 1:10–2:00 — AI / ML
“The learned component is a Decision Tree trained offline on historical thermal data and exported directly into C/C++ as `edge_ml_tree_v7.h`.

We do not feed the raw 2-second stream directly into the model. We aggregate it into approximately 10-minute buckets because that matches the model contract. Five valid buckets are needed before inference.

From those five buckets we create ten features: current temperature, mean, standard deviation, slope, 10-minute delta, 20-minute delta, rising fraction, distance to 8°C, count above 6°C and slope acceleration.

The model estimates the score for crossing 8°C within the next 120 minutes. We use a 0.85 threshold and require two consecutive positive windows before the predictive state becomes active.”

## 2:00–2:40 — Why Edge AI
“The important point is that inference happens on the ESP32. The cloud is only used for telemetry and visualization. This reduces dependence on continuous connectivity for the actual prediction path and keeps the model lightweight enough for the MCU.”

## 2:40–3:20 — Validation
“Our primary validation uses a frozen source-file holdout from historical Nepal thermal data: 6,110 rows from 12 held-out source files.

The frozen results are 66.3% precision, 68.1% recall, 67.2% F1 and 0.907 ROC-AUC.

These are software/model validation results on historical data. We are not presenting them as Bengaluru field accuracy.”

## 3:20–4:00 — Reliability
“When connectivity is available, the ESP32 posts telemetry to ThingSpeak over HTTPS. The current free ThingSpeak limit is 15 seconds per channel update, so local sensing remains 2 seconds but cloud writes are paced at 15 seconds.

When connectivity fails, cloud-rate records are stored in LittleFS and replayed after reconnection. This is offline data resilience, not real-time remote alerting during a network outage.”

## 4:00–4:35 — Demo
“For the software demo, the Velxio build uses the actual embedded Decision Tree and the same feature construction. The thermal trajectory is synthetic and time-accelerated so the ML warm-up can be demonstrated in seconds.

For example, the same tree produces a low score at 5°C, crosses the operating threshold on a warming trajectory, and reaches the two-window persistence condition before the actual 8°C excursion.”

## 4:35–5:00 — Limitations / next step
“The remaining validation is physical: native ESP32 upload, secure ThingSpeak publish, and a controlled 2–8°C thermal trajectory to observe the ML runtime on the real hardware.

We also want to calibrate the fast rate-of-rise detector using real SHT30 2-second data rather than assuming that a historical logger threshold transfers directly.”

## Closing line
“Our contribution is not that cold-chain monitoring is new. It is the combination of low-cost edge prediction, deterministic anomaly detection, offline telemetry resilience and traceability in a compact embedded system.”
