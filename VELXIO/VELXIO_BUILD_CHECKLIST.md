# Velxio Build Checklist

1. Create ESP32 DevKit V1 / ESP32 Dev Module project.
2. Add:
   - `sketch_ml_velxio.ino`
   - `edge_ml_tree_v7.h`
   - `libraries.json`
3. Build.
4. Run at 115200 baud.
5. Wait for five simulated 10-minute buckets.
6. Look for:
   - `ML probability`
   - `Positive streak`
   - `ML prediction : PREDICTED BREACH`
7. When temperature reaches 8.1°C, the tree stops inferring and the domain guard is shown.

Expected model behavior from host replay:
- 5.00°C -> 0.046775
- 5.40°C -> 0.977590 (first positive)
- 5.80°C -> 0.838533 (streak reset)
- 6.20°C -> 0.937800 (first positive)
- 6.60°C -> 0.995853 (second positive -> prediction)
- 7.00°C -> 0.995853
- 7.40°C -> 0.995853
- 7.70°C -> 0.995853
- 7.90°C -> 0.939685
- 8.10°C -> domain guard
