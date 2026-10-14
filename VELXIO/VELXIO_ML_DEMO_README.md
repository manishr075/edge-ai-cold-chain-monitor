# Velxio Edge-AI Demo — SIH26215 (v8.3.2 model path)

## What this demo proves
This simulator runs the **actual deployed Decision Tree** (`edge_ml_tree_v7.h`) on an ESP32 simulation.

The temperature profile is synthetic so the demonstration can run in seconds. The ML inference itself is real model inference using the same `edge_ml_tree_v7.h` as production.

## Files
- `sketch_ml_velxio.ino` — simulator sketch
- `edge_ml_tree_v7.h` — actual deployed Decision Tree
- `libraries.json` — DHTesp dependency

## Run
1. Create an ESP32 DevKit V1 project in Velxio.
2. Put the three files in the project.
3. Build/run.
4. Open serial output at 115200 baud.

## Expected progression
After 5 simulated 10-minute buckets, the Decision Tree starts producing probabilities.
The current demo profile should move through stable → warming → predicted breach → actual excursion.

A representative host replay of the exact feature-generation logic and the same tree produced:

| Current T | ML probability | Result |
|---:|---:|---|
| 5.00 C | 0.046775 | no prediction |
| 5.40 C | 0.977590 | first positive window |
| 5.80 C | 0.838533 | persistence reset |
| 6.20 C | 0.937800 | first positive window |
| 6.60 C | 0.995853 | second positive → prediction |
| 7.00 C | 0.995853 | prediction stays active |
| 7.40 C | 0.995853 | prediction stays active |
| 7.70 C | 0.995853 | prediction stays active |
| 7.90 C | 0.939685 | prediction stays active |
| 8.10 C | — | domain guard; actual breach layer takes over |

## Judge-safe statement
Say:
> “This simulator uses the real embedded Decision Tree and the same ten-feature construction as the deployment path, but the temperature trajectory is synthetic and time-accelerated. It demonstrates the firmware inference path; it is not physical ML validation.”

Do not present the simulator probability sequence as model accuracy or field validation.


## Real-hardware mode
Set `DEMO_MODE` to `false` to read the DHT22. The stream is aggregated into 10-minute buckets and a bucket is accepted only when it reaches at least 80% of the expected 2-second samples. A bad/short bucket resets ML history rather than pretending that cadence is valid.

The DHT22 build is a presentation compatibility path for Velxio. The production sensor path remains SHT30.
