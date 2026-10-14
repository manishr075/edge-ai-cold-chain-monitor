# Edge-AI Predictive Cold-Chain Monitor

**Smart India Hackathon (SIH) | Problem Statement: SIH26215**  
**Current checkpoint:** v8.3.2  
**Project type:** ESP32-based IoT hardware prototype with on-device machine-learning inference

## 1. Overview

The Edge-AI Predictive Cold-Chain Monitor is designed to monitor temperature-sensitive goods during storage and transport. It combines a temperature/humidity sensor, an ESP32 microcontroller, a lightweight decision-tree model, local alerts, GPS context (when a GPS fix is available), and cloud telemetry.

The system has two complementary functions:

1. **Temperature-breach monitoring:** detects when the measured temperature reaches or exceeds the configured 8°C threshold.
2. **Predictive warning:** when the current temperature is still below 8°C, estimates whether it may exceed 8°C within the next 120 minutes using recent sensor history and an embedded decision-tree model.

The goal is to provide an early warning while connectivity is unavailable or delayed, then upload telemetry when the network is available.

> **Prototype status:** software-level checks and historical-dataset evaluation are documented. Native ESP32 toolchain compilation/upload and controlled physical thermal validation remain pending. This repository does not claim completed end-to-end hardware validation.

## 2. Key Features

- **Temperature and humidity sensing** using the SHT30 sensor.
- **On-device inference** using a lightweight decision tree; cloud access is not required to run the model.
- **Two-stage warning logic:** direct threshold-breach handling and a separate predictive-warning path.
- **Prediction horizon:** estimates the risk of crossing 8°C within 120 minutes.
- **Prediction safeguards:** five valid 10-minute buckets before inference, at least 80% bucket coverage, a probability threshold of 0.85, and two consecutive positive windows before activating a predictive warning.
- **Local alerting** through the firmware's alert output.
- **GPS context** when a compatible GPS module has a valid fix.
- **Offline buffering** using LittleFS, with queued telemetry intended to be sent after connectivity returns.
- **Cloud telemetry** over HTTPS to ThingSpeak, subject to network and account configuration.
- **Reconnect and delivery handling** for intermittent Wi-Fi, including sequence/deduplication and retry logic.
- **Validation documentation** describing model evaluation, software checks, known limitations, and physical test procedures.

## 3. System Architecture

```text
SHT30 temperature/humidity ─┐
                            ├─> ESP32 firmware
GPS receiver (optional fix) ┘       |
                                    ├─> Threshold-breach logic
Recent temperature history ─────────┤
                                    ├─> Embedded decision tree
                                    |       |
                                    |       └─> Predictive warning
                                    |
                                    ├─> Local alert output
                                    |
                                    ├─> LittleFS offline queue
                                    |
                                    └─> HTTPS telemetry ──> ThingSpeak
```

The ESP32 performs the decision logic locally. ThingSpeak is used for telemetry visualization/storage; it is not the inference engine.

## 4. Prediction Logic

The model target is:

**Will the temperature exceed 8°C within the next 120 minutes?**

The firmware separates an existing threshold breach from a forecast of a future breach:

- If the current temperature is already **8°C or higher**, the predictive model is suppressed and the direct breach-handling path takes precedence.
- If the temperature is below 8°C, the model can evaluate the recent history once the required data coverage is available.
- The model runs on a 10-minute bucket cadence.
- At least five valid buckets and 80% minimum bucket coverage are required before inference.
- A predicted probability of at least 0.85 is considered positive.
- Two consecutive positive prediction windows are required to activate the predictive warning.

These are implementation settings, not a guarantee that every real-world excursion will be predicted correctly. Sensor placement, thermal mass, airflow, calibration, sampling gaps, and transport conditions can affect performance.

## 5. Hardware

| Component | Role |
|---|---|
| ESP32 development board | Runs firmware, local decision logic, Wi-Fi, and telemetry |
| SHT30 | Measures temperature and relative humidity |
| GPS module, such as NEO-6M, if retained in the build | Provides location context when a valid fix is available |
| Buzzer or supported alert output | Local warning |
| Power supply and wiring | Powers and connects the prototype |

Confirm the exact board, pin assignments, GPS module, alert hardware, and sensor wiring against the firmware and the physical build before powering the circuit. Do not assume every optional component is installed.

## 6. Software and Technologies

- **Embedded platform:** ESP32
- **Firmware runtime:** Arduino-compatible ESP32 environment and FreeRTOS facilities
- **Sensor interface:** SHT30
- **On-device ML:** embedded decision-tree implementation in `PRODUCTION/edge_ml_tree_v7.h`
- **Offline queue:** LittleFS
- **Telemetry:** HTTPS and ThingSpeak
- **Language:** C/C++ for firmware and tests
- **Validation:** standalone cadence tests, firmware stub-based syntax/static checks, and historical-dataset evaluation

The repository includes a Velxio demo separately from the production firmware. The demo is not evidence that the production firmware has been uploaded to or validated on physical ESP32 hardware.

## 7. Model Evaluation

The documented evaluation uses a frozen historical Nepal vaccine-carrier thermal dataset. It includes 48 development files and 12 held-out source files, with **6,110 held-out rows**.

| Metric | Held-out result |
|---|---:|
| Precision | 66.3% |
| Recall | 68.1% |
| F1 score | 67.2% |
| ROC-AUC | 0.907 |

Confusion counts: **175 true positives, 89 false positives, 82 false negatives, and 5,764 true negatives**.

### How to interpret these results

These figures describe evaluation on the documented historical dataset. They are **not** Bengaluru field results, transport-trial results, or proof of accuracy on the physical ESP32 device. They should not be interpreted as clinical, regulatory, or vaccine-safety certification. The dataset's conditions may differ from a real shipment.

## 8. Current Verification Status

### Completed and documented

- Randomized testing of 250,000 decision-tree input vectors, with no invalid, non-finite, or out-of-range probabilities reported.
- Cadence edge-case tests.
- Clang syntax checking through an Arduino/ESP32/FreeRTOS stub layer.
- Clang static analysis with no diagnostics reported.
- Compilation checks for the separate Velxio demo through stubs.
- Historical-dataset holdout evaluation and supporting reports.

### Still required

- Compile with the native ESP32 toolchain and upload to the target board.
- Verify a real HTTPS/ThingSpeak publish using the intended device and account configuration.
- Run a controlled physical temperature trajectory through the actual ESP32 inference path.
- Confirm GPS operation if GPS is included in the final build.
- Measure power consumption and battery runtime if battery operation is claimed.

Until these checks are performed, describe the system as a prototype with software-level verification—not as a fully field-validated product.

## 9. Repository Structure

```text
.
├── README.md
├── PRODUCTION/
│   ├── cold_chain_monitor_sih_ml_v8_3_2_secure_thingspeak.ino
│   ├── edge_ml_tree_v7.h
│   ├── secrets.example.h
│   └── GITHUB_SECURITY_NOTE.md
├── PRESENTATION/
│   ├── EDGE-AI-PREDICTIVE-COLD-CHAIN-MONITOR_SIH_UPDATED_V8_3_2.pdf
│   └── EDGE-AI-PREDICTIVE-COLD-CHAIN-MONITOR_SIH_UPDATED_V8_3_2.pptx
├── BASELINE/
│   └── Earlier v8.3.1 reference firmware and presentation
├── VALIDATION/
│   ├── SIH_V8_3_2_AUDIT_REPORT.md
│   ├── SIH_CHECKPOINT_V8_3_2.md
│   ├── SIH_EXECUTION_STATUS_CURRENT_2026_09_28.md
│   ├── MODEL_EXPLANATION_V8_3_2.md
│   ├── MANUAL_VALIDATION_CHECKLIST_V8_3_2.md
│   ├── PHYSICAL_RATE_CALIBRATION_PROTOCOL_V8_3_2.md
│   ├── RATE_THRESHOLD_ANALYSIS_2026_09_28.md
│   └── Additional tests and release notes
└── VELXIO/
    └── Separate demo sketch, instructions, and demo results
```

The `BASELINE/` directory contains earlier reference material. Use the `PRODUCTION/` firmware and model files for the v8.3.2 production checkpoint unless a validation document explicitly instructs otherwise.

## 10. Setup and Build

This repository checkpoint does not provide a verified, universal one-command build for every ESP32 board and development environment. Use the following workflow and consult the included validation documents.

1. Install the Arduino IDE or another ESP32-compatible build environment and the ESP32 board support package.
2. Open `PRODUCTION/cold_chain_monitor_sih_ml_v8_3_2_secure_thingspeak.ino`.
3. Install the libraries required by the sketch and confirm the target ESP32 board.
4. Review the firmware's pin definitions and match them to the actual wiring.
5. Configure Wi-Fi and ThingSpeak credentials using the example configuration and security notes. Keep real credentials out of source control.
6. Compile with the native ESP32 toolchain.
7. Upload to the board and open the serial monitor to inspect startup, sensor readings, inference cadence, alerts, and connectivity.
8. Verify local breach handling before testing predictive warnings.
9. Test offline buffering and recovery by temporarily removing Wi-Fi, then restoring it.
10. Verify an actual ThingSpeak update and record the result.
11. Follow the controlled thermal-trajectory procedure in `VALIDATION/PHYSICAL_RATE_CALIBRATION_PROTOCOL_V8_3_2.md`.

**Important:** stub-based syntax checks are useful but are not a substitute for a native ESP32 compile, upload, and physical test.

## 11. Configuration and Secret Handling

- Start from `PRODUCTION/secrets.example.h` and follow `PRODUCTION/GITHUB_SECURITY_NOTE.md`.
- Never commit Wi-Fi passwords, ThingSpeak write API keys, private tokens, or other credentials.
- Keep local secret files out of Git using `.gitignore`; check `git status` before every commit.
- If a secret has already been committed or pushed, revoke/rotate it. Deleting it from the latest version alone does not remove it from Git history.
- The example file is a template; it is not a working credential configuration.

## 12. ThingSpeak Notes

The firmware uses HTTPS telemetry to ThingSpeak. The free ThingSpeak plan has a minimum 15-second update interval; this project must also respect the configured pacing and service limits. A successful compile does not prove that cloud publishing works. Verify publishing with a real channel and record the test outcome.

## 13. Limitations and Responsible Use

- The model is evaluated on a historical dataset; real-world generalization remains to be tested.
- Temperature measurement depends on sensor placement, calibration, enclosure design, and thermal lag.
- GPS location is available only when the module is installed and has a valid fix.
- Cloud visibility depends on Wi-Fi, credentials, TLS configuration, and service availability.
- Offline buffering reduces dependence on continuous connectivity but does not guarantee delivery under every failure mode.
- The 8°C threshold is the project's configured target, not a universal storage rule for every medicine or perishable product. Follow the requirements for the specific cargo.
- This prototype is not a replacement for validated cold-chain equipment, approved monitoring procedures, or regulatory compliance testing.

## 14. Project Documents

- **SIH presentation (PDF):** `PRESENTATION/EDGE-AI-PREDICTIVE-COLD-CHAIN-MONITOR_SIH_UPDATED_V8_3_2.pdf`
- **Editable presentation:** `PRESENTATION/EDGE-AI-PREDICTIVE-COLD-CHAIN-MONITOR_SIH_UPDATED_V8_3_2.pptx`
- **Checkpoint and audit:** `VALIDATION/SIH_CHECKPOINT_V8_3_2.md`, `VALIDATION/SIH_V8_3_2_AUDIT_REPORT.md`
- **Model explanation:** `VALIDATION/MODEL_EXPLANATION_V8_3_2.md`
- **Manual verification checklist:** `VALIDATION/MANUAL_VALIDATION_CHECKLIST_V8_3_2.md`
- **Physical test procedure:** `VALIDATION/PHYSICAL_RATE_CALIBRATION_PROTOCOL_V8_3_2.md`
- **Demo/production distinction:** `VELXIO/MENTOR_DEMO_VS_PRODUCTION.md`

## 15. License

No license is specified in this checkpoint. Unless a license is added to the repository, do not assume that the code or documents are available for unrestricted reuse.
