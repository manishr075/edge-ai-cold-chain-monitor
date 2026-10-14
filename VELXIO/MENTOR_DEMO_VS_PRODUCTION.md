# SIH26215 — Mentor Demo vs Production Build

## Production build
`cold_chain_monitor_sih_ml_v8_3_1_secure_thingspeak.ino`

- ESP32 + SHT30 + NEO-6M
- 2-second physical sensing
- 10-minute ML cadence
- actual embedded Decision Tree
- ThingSpeak HTTPS/TLS
- LittleFS offline telemetry
- physical validation still pending for final ML + ThingSpeak path

## Velxio demo build
`sketch_ml_velxio.ino`

- ESP32 + DHT22 simulation
- same embedded Decision Tree (`edge_ml_tree_v7.h`)
- synthetic temperature trajectory
- one simulated 10-minute bucket every 2 seconds
- same 10 features and 0.85 / 2-window logic
- no cloud/GPS

## What the Velxio demo proves
It proves that the **actual embedded tree can be executed in the simulated ESP32 environment** and demonstrates the inference path quickly.

## What it does NOT prove
It does not prove physical sensor accuracy, physical thermal response, real ThingSpeak connectivity, or field ML accuracy.

## Exact judge-safe sentence
> "The Velxio demonstration uses the same embedded Decision Tree and feature logic as the deployment build, but accelerates the thermal clock with synthetic data so the inference path can be shown during the presentation. Our actual model validation and physical runtime claims are kept separate."
