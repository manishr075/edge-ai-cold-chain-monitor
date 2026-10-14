# Embedded Decision Tree — Mentor Explanation (v8.3.2)

## Example: why 6.6°C in the Velxio trajectory produces ~0.996

The actual five-bucket temperature window is:

`[5.0, 5.4, 5.8, 6.2, 6.6] °C`

The firmware generates:

| Feature | Value |
|---|---:|
| Current temperature | 6.60 |
| 5-bucket mean | 5.80 |
| 5-bucket std | 0.5657 |
| 5-bucket slope | 0.0400 °C/min |
| 10-min delta | 0.40°C |
| 20-min delta | 0.80°C |
| Rising fraction | 1.00 |
| Distance to 8°C | 1.40°C |
| Count above 6°C | 2 |
| Slope acceleration | 0.00 |

The tree is a sequence of learned threshold tests. For this feature vector, the path reaches:

`f[7] <= 2.75`

then

`f[5] > 0.15`

then

`f[7] <= 2.05`

then

`f[6] > 0.875`

then

`f[3] > 0.01125`

then

`f[9] <= 0.00625`

which reaches the leaf:

`0.99585306`

The deployment rule then applies the operating threshold:

`0.99585306 >= 0.85` → positive ML window.

If the immediately following ML window is also >=0.85, the persistence rule makes:

`ml_prediction = 1`

## Important interpretation

The returned number is the tree's leaf probability estimate used by the deployment model. It is a model score, not independently calibrated real-world probability. The project has not performed a separate probability-calibration study.

## Why this is Edge AI

Training occurs offline. The learned tree is exported to a 6.1 KB C/C++ header and executed locally on the ESP32 without a cloud ML call or an ML runtime. The cloud path only transports telemetry.
