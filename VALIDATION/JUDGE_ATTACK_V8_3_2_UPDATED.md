# SIH26215 — Updated Judge Attack Sheet (v8.3.2)

This sheet supersedes the old critique that was written when the project had only rule-based edge analytics. The current build has an embedded Decision Tree trained offline and executed on the ESP32.

## 1. Where exactly is the AI?
**Answer:** The Decision Tree in `edge_ml_tree_v7.h` is the learned model. We trained it offline, exported the learned tree as C/C++, and run inference locally on the ESP32. The cloud is only the telemetry layer.

## 2. What does the model predict?
**Answer:** It estimates the probability that the current temperature trajectory will cross 8°C within the next 120 minutes. The deployment model is evaluated only when the current bucket is below 8°C.

## 3. Why a Decision Tree?
**Answer:** The model is small, deterministic, fast to evaluate, interpretable, and easy to export to plain C/C++ without a runtime ML framework on the ESP32. The embedded tree header is about 6.1 KB of source, contains 60 decision nodes and has a shallow branching depth, which is practical for an ESP32.

## 4. Why 2-second sensing but 10-minute ML?
**Answer:** The ESP32 samples quickly for safety logic and telemetry, while the ML features are defined at approximately 10-minute cadence. The firmware aggregates the fast stream into buckets instead of feeding a different cadence into the trained model.

## 5. What are the ML features?
**Answer:** Current temperature, 5-sample mean, 5-sample standard deviation, 5-sample slope, 10-minute delta, 20-minute delta, rising fraction, distance to 8°C, count above 6°C, and slope acceleration.

## 6. Why 0.85 threshold and two positive windows?
**Answer:** 0.85 is the frozen deployment threshold selected before the held-out source-file evaluation. Two consecutive positive windows provide a persistence check against one isolated positive model output.

## 7. What are the model results?
**Answer:** On the frozen historical Nepal thermal holdout: 6,110 rows from 12 completely held-out source files; precision 66.3%, recall 68.1%, F1 67.2%, ROC-AUC 0.907.

**Important:** Those are historical-data validation metrics, not Bengaluru field accuracy.

## 8. Did you verify Python and ESP32 model behavior match?
**Answer:** Yes. The embedded C/C++ model was compared against the Python reference implementation on a large vector set and sequential historical replay; no mismatches above the specified tolerance were observed in the recorded equivalence tests.

## 9. What happens when Wi-Fi fails?
**Answer:** Sensor processing continues independently. Cloud-rate telemetry is written to LittleFS and replayed to ThingSpeak after connectivity returns. This protects telemetry data, not real-time remote alerting while disconnected.

## 10. How do the FreeRTOS tasks share data?
**Answer:** The sensor and cloud tasks communicate through a FreeRTOS queue. Sync timing is protected by a mutex. The cloud task consumes queued readings without blocking the sensor task on network I/O.

## 11. Why SHT30 instead of DHT22 in production?
**Answer:** We moved the production path to SHT30 for the better fit to continuous digital temperature/humidity monitoring. DHT22 remains only in the Velxio/Wokwi simulator because that environment is convenient for the presentation demo.

## 12. What about GPS?
**Answer:** GPS is a traceability/context layer, not part of the temperature prediction itself. The timed hardware test showed serial NMEA communication but no satellite fix, so GPS is honestly marked deferred.

## 13. What happens if the current temperature is already above 8°C?
**Answer:** The learned model is not evaluated outside its declared deployment domain. The immediate anomaly layer handles the actual breach, while the model reports a domain guard.

## 14. Is the system production-ready?
**Answer:** No. It is an SIH MVP. Remaining production work includes physical 2–8°C ML validation, battery characterization, enclosure/environmental testing, field thermal validation, and operator-facing alerting during connectivity loss.

## 15. What is the strongest novelty claim you can defend?
**Answer:** Not that cold-chain monitoring itself is new. The defensible contribution is a low-cost, edge-first architecture that combines local trajectory prediction, deterministic anomaly detection, offline telemetry resilience, and traceability in a small embedded system.

## 16. Are the model scores calibrated probabilities?
**Answer:** They are the Decision Tree leaf probability estimates used as the deployment score. We have not established separate probability calibration on the frozen holdout, so we should not claim that a score of 0.90 is a guaranteed 90% real-world event frequency. The locked 0.85 threshold is the operating point selected for the deployment model.
## 17. Why is the fast rate threshold 6.0 °C/min?
**Answer:** It is deliberately a high-severity spike detector over the very short 2-second SHT30 window, not a calibrated general warming-rate threshold. We reviewed the historical Nepal logger-rate distribution, but its cadence differs from the live 2-second sensor stream, so we did not copy a historical threshold into the firmware. The production calibration step is a controlled physical SHT30 noise/thermal-response test.

## 18. Is the “vaccine life consumed %” universal?
**Answer:** No. The current firmware uses a VVM7-based Arrhenius demonstration budget as a generic example. A production deployment must use the actual product/vaccine stability specification and the appropriate VVM category or validated degradation model.

## 19. What happens if the device reboots while an offline backlog transaction is being modified?
**Answer:** The v8.3.2 build has startup recovery for its main, backup and temporary LittleFS files. The main backlog is preserved when authoritative; interrupted transactions can restore the previous backlog.
