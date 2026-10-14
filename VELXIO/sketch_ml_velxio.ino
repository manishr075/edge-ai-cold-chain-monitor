#include <Arduino.h>
#include <DHTesp.h>
#include <math.h>
#include "edge_ml_tree_v7.h"

// ================================================================
// SIH26215 — Velxio Edge-AI demonstration build
// Hardware: ESP32 + DHT22 only
//
// IMPORTANT:
// - In DEMO_MODE, the temperature samples are synthetic demonstration data.
// - The Decision Tree is the ACTUAL deployed tree from the project.
// - The demo simulates one 10-minute thermal bucket every 2 real seconds,
//   so judges can see the ML inference without waiting 50–60 minutes.
// - This is a presentation/simulation build, not physical ML validation.
// ================================================================

#define DHT_PIN 4
DHTesp dht;

#define DEMO_MODE true
#define DEMO_LOOP true
#define SAMPLE_INTERVAL_MS 2000UL
#define ML_BUCKET_MINUTES 10.0f
#define ML_BUCKET_MS ((unsigned long)(ML_BUCKET_MINUTES * 60000.0f))
#define ML_WINDOW_SIZE 5
#define ML_THRESHOLD 0.85f
#define ML_PERSISTENCE 2
#define SAFE_MAX_C 8.0f
#define SAFE_MIN_C 2.0f

// Stable -> gradual warming -> pre-breach -> excursion.
const float demoTemps[] = {
  5.0f, 5.0f, 5.0f, 5.0f, 5.0f,
  5.4f, 5.8f, 6.2f, 6.6f, 7.0f,
  7.4f, 7.7f, 7.9f, 8.1f, 9.0f,
  10.0f, 12.0f, 15.0f
};
const size_t DEMO_COUNT = sizeof(demoTemps) / sizeof(demoTemps[0]);
size_t demoIndex = 0;

float mlTemps[ML_WINDOW_SIZE] = {0};
int mlCount = 0;
int positiveStreak = 0;
float mlProbability = 0.0f;
bool mlPrediction = false;

// Real hardware mode aggregates the 2-second DHT22 stream into true 10-minute buckets.
unsigned long realBucketStartMs = 0;
unsigned long realBucketSamples = 0;
double realTempSum = 0.0;

uint8_t heatCount = 0;
bool heatLatched = false;

static void resetML() {
  mlCount = 0;
  positiveStreak = 0;
  mlProbability = 0.0f;
  mlPrediction = false;
  for (float &v : mlTemps) v = 0.0f;
}

static void pushBucket(float t) {
  if (mlCount < ML_WINDOW_SIZE) {
    mlTemps[mlCount++] = t;
  } else {
    for (int i = 0; i < ML_WINDOW_SIZE - 1; ++i) mlTemps[i] = mlTemps[i + 1];
    mlTemps[ML_WINDOW_SIZE - 1] = t;
  }
}

static float treeProbability() {
  if (mlCount < ML_WINDOW_SIZE) return 0.0f;

  float mean = 0.0f;
  for (float t : mlTemps) mean += t;
  mean /= 5.0f;

  float variance = 0.0f;
  for (float t : mlTemps) {
    const float d = t - mean;
    variance += d * d;
  }
  const float stdv = sqrtf(variance / 5.0f);

  const float slope = (mlTemps[4] - mlTemps[0]) / 40.0f;
  const float d10 = mlTemps[4] - mlTemps[3];
  const float d20 = mlTemps[4] - mlTemps[2];

  float risingFraction = 0.0f;
  for (int i = 1; i < 5; ++i) {
    if (mlTemps[i] > mlTemps[i - 1]) risingFraction += 1.0f;
  }
  risingFraction /= 4.0f;

  const float distanceTo8 = 8.0f - mlTemps[4];
  const float countAbove6 =
      (mlTemps[0] > 6.0f) + (mlTemps[1] > 6.0f) +
      (mlTemps[2] > 6.0f) + (mlTemps[3] > 6.0f) +
      (mlTemps[4] > 6.0f);
  const float slopeAcceleration = slope - d20 / 20.0f;

  float features[10] = {
      mlTemps[4], mean, stdv, slope, d10,
      d20, risingFraction, distanceTo8, countAbove6, slopeAcceleration
  };

  return edge_ml_breach_probability_v7(features);
}

static uint8_t evaluateRuleAnomaly(float temperature) {
  if (temperature > SAFE_MAX_C) {
    if (heatCount < 255) ++heatCount;
  } else {
    heatCount = 0;
  }

  if (heatCount >= 2) heatLatched = true;
  if (heatCount == 0) heatLatched = false;
  return heatLatched ? 1 : 0;
}

static void processMLBucket(float temperature, float humidity, unsigned long simulatedMinutes, uint8_t anomaly) {

  // The deployed tree is only evaluated while the current bucket is inside
  // its training/deployment domain (<8 °C).
  if (temperature >= SAFE_MAX_C) {
    // End this simulated excursion with a clean ML state so a new demo
    // cycle never reuses pre-breach buckets.
    resetML();
  } else {
    pushBucket(temperature);
    if (mlCount >= ML_WINDOW_SIZE) {
      mlProbability = treeProbability();
      if (mlProbability >= ML_THRESHOLD) ++positiveStreak;
      else positiveStreak = 0;
      mlPrediction = (positiveStreak >= ML_PERSISTENCE);
    }
  }

  Serial.println("==================================================");
  Serial.printf("Simulated time        : %lu min\n", simulatedMinutes);
  Serial.printf("Bucket temperature    : %.2f C\n", temperature);
  Serial.printf("Bucket humidity       : %.1f %%RH\n", humidity);
  Serial.printf("Rule anomaly          : %u (%s)\n", anomaly, anomaly ? "HEAT" : "NORMAL");
  Serial.printf("ML buckets available  : %d/%d\n", mlCount, ML_WINDOW_SIZE);
  if (mlCount >= ML_WINDOW_SIZE && temperature < SAFE_MAX_C) {
    Serial.printf("ML probability        : %.4f\n", mlProbability);
    Serial.printf("ML threshold          : %.2f\n", ML_THRESHOLD);
    Serial.printf("Positive streak       : %d/%d\n", positiveStreak, ML_PERSISTENCE);
    Serial.printf("ML prediction         : %s\n", mlPrediction ? "PREDICTED BREACH" : "NO PREDICTIVE BREACH");
  } else if (temperature >= SAFE_MAX_C) {
    Serial.println("ML status             : DOMAIN GUARD — current temperature >= 8 C");
  } else {
    Serial.println("ML status             : WARM-UP — need 5 valid 10-min buckets");
  }
  Serial.println("==================================================\n");
}

void sensorTask(void *pv) {
  unsigned long lastSampleMs = millis();
  realBucketStartMs = lastSampleMs;
  realBucketSamples = 0;
  realTempSum = 0.0;
  float lastHumidity = NAN;
  unsigned long simulatedMinutes = 0;

  for (;;) {
    float temperature = NAN;
    float humidity = NAN;

    if (DEMO_MODE) {
      temperature = demoTemps[demoIndex];
      humidity = 70.0f;
      if (demoIndex + 1 < DEMO_COUNT) {
        ++demoIndex;
      } else if (DEMO_LOOP) {
        demoIndex = 0;
        heatCount = 0;
        heatLatched = false;
        resetML();
      }

      const uint8_t anomaly = evaluateRuleAnomaly(temperature);
      processMLBucket(temperature, humidity, simulatedMinutes, anomaly);
      simulatedMinutes += (unsigned long)ML_BUCKET_MINUTES;
    } else {
      temperature = dht.getTemperature();
      humidity = dht.getHumidity();
      if (isnan(temperature) || isnan(humidity)) {
        Serial.println("DHT22 read failed");
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
        continue;
      }

      // Fast safety layer observes every physical sensor sample.
      const uint8_t anomaly = evaluateRuleAnomaly(temperature);
      realTempSum += temperature;
      realBucketSamples++;
      lastHumidity = humidity;

      const unsigned long now = millis();
      if (now - realBucketStartMs >= ML_BUCKET_MS) {
        const unsigned long expectedSamples = ML_BUCKET_MS / SAMPLE_INTERVAL_MS;
        const float coverage = expectedSamples > 0 ?
            (float)realBucketSamples / (float)expectedSamples : 0.0f;

        if (realBucketSamples > 0 && coverage >= 0.80f) {
          const float bucketTemp = (float)(realTempSum / (double)realBucketSamples);
          processMLBucket(bucketTemp, lastHumidity, (now - realBucketStartMs) / 60000UL, anomaly);
        } else {
          Serial.printf("ML bucket rejected: samples=%lu coverage=%.1f%%\n",
                        realBucketSamples, coverage * 100.0f);
          resetML();
        }

        realTempSum = 0.0;
        realBucketSamples = 0;
        realBucketStartMs = now;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== SIH26215 Velxio Edge-AI Demo ===");
  Serial.println("Hardware: ESP32 + DHT22");
  Serial.println("Model: edge_ml_tree_v7.h");
  Serial.println("Target: >8 C within the next 120 minutes");
  Serial.println("Threshold: 0.85 | Persistence: 2 positive windows");
  Serial.println(DEMO_MODE ? "DEMO_MODE: synthetic thermal trajectory; actual tree inference" : "REAL_MODE: DHT22 -> 10-min buckets -> actual tree inference");

  dht.setup(DHT_PIN, DHTesp::DHT22);
  resetML();

  // The demo uses a single queue/task only for simple presentation output.
  // The production build keeps the full SensorTask/CloudTask architecture.
  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, nullptr, 2, nullptr, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
