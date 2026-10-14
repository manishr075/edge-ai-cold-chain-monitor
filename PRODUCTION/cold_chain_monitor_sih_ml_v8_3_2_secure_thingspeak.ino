/*
  =====================================================================
  Offline-Resilient IoT System for Predictive Cold-Chain Assurance
  in Rural Healthcare Logistics — Secure MVP Firmware (v8.3.2 reliability hardening)
  SIH26215
  =====================================================================
  Hardware: ESP32 + SHT30-DIS (I2C temp/humidity) + NEO-6M GPS only.
  No additional hardware (no GSM/SIM module, no buzzer, no display).

  HONEST LIMITATION — say this to judges rather than let them find it:
  this version is offline-resilient for DATA (nothing is lost during
  a connectivity gap, thanks to the persistent LittleFS log below),
  but NOT for real-time ALERTING — with no local indicator and no
  SMS/GSM fallback, there is no way to notify anyone WHILE the device
  is disconnected. The anomaly is detected and logged the moment it
  happens, but a human only finds out once the dashboard syncs, which
  is exactly when connectivity returns. That is a legitimate, honestly
  scoped hackathon MVP limitation — not a hidden gap.

  WHAT THIS VERSION STILL DOES (all software, no extra hardware)
  ------------------------------------------------------------------
  - Newton's Law of Cooling: predicts minutes until an 8C breach from
    a 3-point exponential fit of the sliding window, with a
    plausibility check on the fitted ambient temperature.
  - Arrhenius cumulative damage: tracks "vaccine life consumed" using
    WHO's VVM7 category as a general reference (7 days at 37C, 45
    days at 25C) — replace with your target vaccine's real VVM
    category if you have it.
  - Freeze detection (< 2C): a one-way latch for the selected demonstration
    target. Freeze sensitivity is product-specific, so this should be validated
    against the actual vaccine/presentation before deployment.
  - Composite Risk Score (0-100): temperature margin + breach urgency
    + cumulative damage + minutes-since-last-successful-sync (the
    rural-relevant "how long has this gone unconfirmed" factor).
  - Persistent offline log (LittleFS): when ThingSpeak is unreachable,
    cloud-rate readings are appended to flash instead of lost, capped in size,
    and drained back to ThingSpeak automatically once reconnected. This
    is the genuine "Offline-Resilient" mechanism in this version —
    it protects the DATA even though it can't protect real-time
    ALERTING without added hardware.

  NAMING NOTE: rule-based anomaly detection remains alongside a separate
  lightweight decision-tree ML layer. The ML layer is trained offline and
  runs on the ESP32 after the cadence adapter has enough valid 10-minute
  buckets.

  LIBRARIES REQUIRED (Arduino Library Manager)
  ---------------------------------------------
  - Adafruit SHT31 Library   (works for SHT30 - same command set)
  - Adafruit BusIO           (dependency of the above)
  - TinyGPSPlus              (by Mikal Hart)
  - HTTPClient               (included with ESP32 Arduino core)
  - LittleFS is built into the ESP32 Arduino core — no install needed
  =====================================================================
*/

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <TinyGPSPlus.h>
#include <HTTPClient.h>
#include "Adafruit_SHT31.h"
#include <math.h>
#include <LittleFS.h>
#include "edge_ml_tree_v7.h"

// ---------------------------------------------------------------
// CONFIG — WiFi / ThingSpeak Cloud
// ---------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ThingSpeak channel details. The Write API Key is enough for REST writes.
// Channel ID is kept here for operator reference / dashboard setup.
const char* THINGSPEAK_CHANNEL_ID     = "YOUR_THINGSPEAK_CHANNEL_ID";
const char* THINGSPEAK_WRITE_API_KEY  = "YOUR_THINGSPEAK_WRITE_API_KEY";
const char* THINGSPEAK_UPDATE_URL     = "https://api.thingspeak.com/update";

// Free ThingSpeak accounts allow channel updates every 15 seconds.
// The sensor still samples every 2 seconds locally; only cloud writes are rate-limited.
#define THINGSPEAK_UPDATE_INTERVAL_MS 15000UL

// Secure ThingSpeak HTTPS configuration. MathWorks currently documents the
// DigiCert Global Root CA as the root certificate to use for api.thingspeak.com.
#define THINGSPEAK_TLS_INSECURE_FOR_HARDWARE_TEST false

static const char* THINGSPEAK_ROOT_CA =
"-----BEGIN CERTIFICATE-----\n"
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
"-----END CERTIFICATE-----\n";

#define EDGE_ML_THRESHOLD 0.85f
#define EDGE_ML_PERSISTENCE 2
#define ML_BUCKET_MS 600000UL
#define ML_MIN_BUCKET_COVERAGE 0.80f
#define GPS_MAX_AGE_MS 120000UL
#define SENSOR_REINIT_AFTER_FAILURES 5
#define MODEL_VERSION "tree120m_v2_dev48_thr085_v8_3_2_secure_ts"

// ---------------------------------------------------------------
// CONFIG — Pins (sensors only)
// ---------------------------------------------------------------
#define SHT30_SDA_PIN   21
#define SHT30_SCL_PIN   22
#define GPS_RX_PIN      16
#define GPS_TX_PIN      17

// ---------------------------------------------------------------
// CONFIG — Demonstration cold-chain band (2-8C). Actual vaccine/product
// requirements are product-specific and must be configured per target.
// ---------------------------------------------------------------
// ---------------------------------------------------------------
#define ABS_MAX_TEMP_C          8.0
#define ABS_MIN_TEMP_C           2.0
#define ABS_BREACH_PERSISTENCE  2
#define FREEZE_BREACH_PERSISTENCE 2

#define RATE_THRESHOLD_C_PER_MIN   6.0
// Deliberately high fast-window spike threshold. It is NOT a calibrated
// general warming-rate threshold; select that later from physical SHT30 data.
#define RATE_BREACH_PERSISTENCE    3

#define WINDOW_SIZE            5
#define SAMPLE_INTERVAL_MS     2000

#define GPS_READ_EVERY_N_SAMPLES  15

#define MAX_PREDICT_HORIZON_MIN   120.0

#define AMBIENT_PLAUSIBLE_MIN_C   15.0
#define AMBIENT_PLAUSIBLE_MAX_C   50.0

// ---------------------------------------------------------------
// CONFIG — Arrhenius damage-budget anchor points (WHO VVM7 category)
// Source: WHO, "Getting Started with Vaccine Vial Monitors" —
// VVM7 = 7 days at 37C, 45 days at 25C. Replace with your target
// vaccine's real VVM category if you have it.
// ---------------------------------------------------------------
#define ARRHENIUS_T_A_C     37.0
#define ARRHENIUS_T_A_MIN   (7.0  * 24.0 * 60.0)
#define ARRHENIUS_T_B_C     25.0
#define ARRHENIUS_T_B_MIN   (45.0 * 24.0 * 60.0)
#define KELVIN_OFFSET        273.15

// ---------------------------------------------------------------
// CONFIG — Offline-duration risk factor. Not a cited statistic —
// tune to your target region if you have field data.
// ---------------------------------------------------------------
#define OFFLINE_RISK_TRIGGER_MIN   30.0

// ---------------------------------------------------------------
// CONFIG — Persistent offline log (LittleFS)
// ---------------------------------------------------------------
#define OFFLINE_LOG_PATH        "/thingspeak_offline.log"
#define OFFLINE_LOG_MAX_BYTES   200000UL
#define OFFLINE_LOG_TRIM_EVERY_N_WRITES  50

// ---------------------------------------------------------------
// Telemetry struct — passed sensorTask -> cloudTask via queue
// ---------------------------------------------------------------
typedef struct {
  float temperature;
  float humidity;
  double latitude;
  double longitude;
  bool gpsValid;
  uint8_t anomalyType;              // 0=normal, 1=heat, 2=freeze, 3=both
  float predictedMinutesToBreach;
  float vaccineLifeConsumedPct;
  float offlineMinutes;
  float riskScore;
  unsigned long timestampMs;
  uint32_t sequence;
  uint8_t mlPrediction;
  float mlProbability;
  uint8_t mlDataQuality;
} TelemetryReading;

QueueHandle_t telemetryQueue;
#define TELEMETRY_QUEUE_LENGTH 150

SemaphoreHandle_t syncTimeMutex;   // protects lastSuccessfulSyncMs across tasks

// ---------------------------------------------------------------
// Globals used only inside the Sensor task (Core 0)
// ---------------------------------------------------------------
Adafruit_SHT31 sht30 = Adafruit_SHT31();
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

unsigned long lastGpsUpdateMs = 0;
uint32_t sequenceCounter = 0;

float tempWindow[WINDOW_SIZE];
unsigned long windowTimestamps[WINDOW_SIZE];
int windowIndex = 0;
int windowFilled = 0;

int absBreachCount = 0;
int rateBreachCount = 0;
bool heatAnomalyLatched = false;

int freezeBreachCount = 0;
bool freezeLatched = false;

double lastKnownLat = 0.0;
double lastKnownLng = 0.0;
bool lastGpsValid = false;

double cumulativeDamageFraction = 0.0;
double arrheniusA, arrheniusB;

// ---------------------------------------------------------------
// Globals used only inside the cloud task (Core 1)
// ---------------------------------------------------------------
WiFiClientSecure thingSpeakClient;
volatile unsigned long lastSuccessfulSyncMs = 0;   // guarded by syncTimeMutex

// =================================================================
// Arrhenius fit
// =================================================================
void computeArrheniusFit() {
  double T_A_K = ARRHENIUS_T_A_C + KELVIN_OFFSET;
  double T_B_K = ARRHENIUS_T_B_C + KELVIN_OFFSET;
  arrheniusB = (log((double)ARRHENIUS_T_A_MIN) - log((double)ARRHENIUS_T_B_MIN)) /
               (1.0 / T_A_K - 1.0 / T_B_K);
  arrheniusA = log((double)ARRHENIUS_T_A_MIN) - arrheniusB * (1.0 / T_A_K);
}

double allowableMinutesAtTemp(double tempC) {
  double T_K = tempC + KELVIN_OFFSET;
  return exp(arrheniusA + arrheniusB / T_K);
}

// =================================================================
// Newton's Law of Cooling — 3-point fit, with plausibility check
// =================================================================
float predictMinutesToBreachLinearFallback(float currentTemp, float rateCPerMin) {
  if (rateCPerMin <= 0.01) return MAX_PREDICT_HORIZON_MIN;
  float minutesToBreach = (ABS_MAX_TEMP_C - currentTemp) / rateCPerMin;
  if (minutesToBreach < 0) minutesToBreach = 0;
  if (minutesToBreach > MAX_PREDICT_HORIZON_MIN) minutesToBreach = MAX_PREDICT_HORIZON_MIN;
  return minutesToBreach;
}

float predictMinutesToBreachNewton(float currentTemp, float fallbackRateCPerMin) {
  // Outside the safe range, "minutes until 8C" is already zero.
  if (currentTemp >= ABS_MAX_TEMP_C) return 0.0;
  if (windowFilled < WINDOW_SIZE) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  int oldestIdx = windowIndex;
  int middleIdx = (oldestIdx + 2) % WINDOW_SIZE;
  int newestIdx = (oldestIdx + WINDOW_SIZE - 1) % WINDOW_SIZE;

  double T0 = tempWindow[oldestIdx];
  double T1 = tempWindow[middleIdx];
  double T2 = tempWindow[newestIdx];
  unsigned long t0_time = windowTimestamps[oldestIdx];
  unsigned long t2_time = windowTimestamps[newestIdx];

  double denomR = (T1 - T0);
  if (fabs(denomR) < 0.001) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  double r = (T2 - T1) / denomR;
  if (r <= 0.0 || r >= 1.0) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  double elapsedMin = ((double)(t2_time - t0_time) / 2.0) / 60000.0;
  if (elapsedMin < 0.001) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  double k = -log(r) / elapsedMin;

  double ambDenom = (T0 + T2 - 2.0 * T1);
  if (fabs(ambDenom) < 0.001 || k <= 0.0) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  double T_amb = (T0 * T2 - T1 * T1) / ambDenom;

  if (T_amb < AMBIENT_PLAUSIBLE_MIN_C || T_amb > AMBIENT_PLAUSIBLE_MAX_C) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }
  if (T_amb <= ABS_MAX_TEMP_C) {
    return MAX_PREDICT_HORIZON_MIN;
  }
  if (currentTemp >= ABS_MAX_TEMP_C) {
    return 0.0;
  }

  double ratio = (ABS_MAX_TEMP_C - T_amb) / (currentTemp - T_amb);
  if (ratio <= 0.0 || ratio >= 1.0) {
    return predictMinutesToBreachLinearFallback(currentTemp, fallbackRateCPerMin);
  }

  double tMinutes = -log(ratio) / k;
  if (tMinutes < 0) tMinutes = 0;
  if (tMinutes > MAX_PREDICT_HORIZON_MIN) tMinutes = MAX_PREDICT_HORIZON_MIN;

  return (float)tMinutes;
}

// =================================================================
// Composite Risk Score (0-100)
// =================================================================
float computeRiskScore(float currentTemp, float predictedMinutes,
                        double damageFractionPct, double offlineMin) {
  float tempComponent;
  if (currentTemp >= ABS_MAX_TEMP_C) {
    tempComponent = 100.0;
  } else {
    float margin = ABS_MAX_TEMP_C - currentTemp;
    tempComponent = 100.0 * exp(-margin / 3.0);
  }

  float breachComponent = 100.0 * exp(-predictedMinutes / 20.0);
  float damageComponent = (float)fmin(damageFractionPct, 100.0);

  float offlineComponent = 0.0;
  if (offlineMin >= OFFLINE_RISK_TRIGGER_MIN) {
    offlineComponent = (float)fmin(100.0, (offlineMin - OFFLINE_RISK_TRIGGER_MIN) * 5.0);
  }

  float score = 0.35 * tempComponent + 0.30 * breachComponent +
                0.20 * damageComponent + 0.15 * offlineComponent;
  if (score > 100.0) score = 100.0;
  if (score < 0.0) score = 0.0;
  return score;
}

// =================================================================
// Anomaly detection
// =================================================================
uint8_t evaluateAnomaly(float latestTemp, unsigned long now, float *predictedMinutesOut) {
  tempWindow[windowIndex] = latestTemp;
  windowTimestamps[windowIndex] = now;
  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  if (windowFilled < WINDOW_SIZE) windowFilled++;

  if (latestTemp > ABS_MAX_TEMP_C) {
    absBreachCount++;
  } else {
    absBreachCount = 0;
  }

  float rateCPerMin = 0.0;
  if (windowFilled == WINDOW_SIZE) {
    int oldestIdx = windowIndex;
    float oldestTemp = tempWindow[oldestIdx];
    unsigned long oldestTime = windowTimestamps[oldestIdx];
    float elapsedMin = (now - oldestTime) / 60000.0;
    if (elapsedMin > 0.001) {
      rateCPerMin = (latestTemp - oldestTemp) / elapsedMin;
    }
  }

  if (rateCPerMin > RATE_THRESHOLD_C_PER_MIN) {
    rateBreachCount++;
  } else {
    rateBreachCount = 0;
  }

  if (!heatAnomalyLatched) {
    if (absBreachCount >= ABS_BREACH_PERSISTENCE ||
        rateBreachCount >= RATE_BREACH_PERSISTENCE) {
      heatAnomalyLatched = true;
    }
  } else {
    if (absBreachCount == 0 && rateBreachCount == 0) {
      heatAnomalyLatched = false;
    }
  }

  if (latestTemp < ABS_MIN_TEMP_C) {
    freezeBreachCount++;
  } else {
    freezeBreachCount = 0;
  }
  if (!freezeLatched && freezeBreachCount >= FREEZE_BREACH_PERSISTENCE) {
    freezeLatched = true;
    cumulativeDamageFraction = fmax(cumulativeDamageFraction, 1.0);
  }

  if (latestTemp > ABS_MAX_TEMP_C) {
    double lifeAtTemp = allowableMinutesAtTemp((double)latestTemp);
    double sampleMinutes = SAMPLE_INTERVAL_MS / 60000.0;
    if (lifeAtTemp > 0.0001) {
      cumulativeDamageFraction += sampleMinutes / lifeAtTemp;
    }
  }
  if (cumulativeDamageFraction > 2.0) cumulativeDamageFraction = 2.0;

  *predictedMinutesOut = predictMinutesToBreachNewton(latestTemp, rateCPerMin);

  uint8_t anomalyType = (heatAnomalyLatched ? 1 : 0) | (freezeLatched ? 2 : 0);
  return anomalyType;
}

// =================================================================
// Persistent offline log — ThingSpeak form-encoded records
// One record is written/sent at the same 15-second cadence used for
// cloud updates, so the flash backlog represents cloud-rate telemetry.
// =================================================================
// Recover LittleFS transaction state after an unexpected power loss.
// Main log wins when present; a backup is restored when the main file vanished.
void recoverOfflineLogState() {
  const bool mainExists = LittleFS.exists(OFFLINE_LOG_PATH);
  const bool backupExists = LittleFS.exists("/thingspeak_offline.bak");
  const bool tempExists = LittleFS.exists("/thingspeak_offline.tmp");

  if (!mainExists && backupExists) {
    if (LittleFS.rename("/thingspeak_offline.bak", OFFLINE_LOG_PATH)) {
      Serial.println("[offline-log] Recovered original backlog after interrupted file transaction.");
    } else {
      Serial.println("[offline-log] Recovery rename failed; backup retained.");
    }
  } else if (mainExists && backupExists) {
    LittleFS.remove("/thingspeak_offline.bak");
  }

  // A temp file is never authoritative unless the main and backup are both absent.
  if (tempExists && !LittleFS.exists(OFFLINE_LOG_PATH) && !LittleFS.exists("/thingspeak_offline.bak")) {
    if (LittleFS.rename("/thingspeak_offline.tmp", OFFLINE_LOG_PATH)) {
      Serial.println("[offline-log] Recovered temporary backlog after interrupted install.");
    }
  } else if (tempExists) {
    LittleFS.remove("/thingspeak_offline.tmp");
  }

  // If both main and backup somehow disappeared, do not fabricate data.
  if (!LittleFS.exists(OFFLINE_LOG_PATH) && LittleFS.exists("/thingspeak_offline.bak")) {
    LittleFS.rename("/thingspeak_offline.bak", OFFLINE_LOG_PATH);
  }
}

void appendToOfflineLog(const char* formPayload) {
  File f = LittleFS.open(OFFLINE_LOG_PATH, "a");
  if (!f) {
    Serial.println("[offline-log] Failed to open for append.");
    return;
  }
  f.println(formPayload);
  f.close();

  static int writeCounter = 0;
  writeCounter++;
  if (writeCounter >= OFFLINE_LOG_TRIM_EVERY_N_WRITES) {
    writeCounter = 0;

    File check = LittleFS.open(OFFLINE_LOG_PATH, "r");
    if (check) {
      size_t sz = check.size();
      check.close();
      if (sz > OFFLINE_LOG_MAX_BYTES) {
        const size_t keepBytes = (size_t)(OFFLINE_LOG_MAX_BYTES * 0.7);
        size_t skipBytes = sz - keepBytes;

        File in = LittleFS.open(OFFLINE_LOG_PATH, "r");
        File out = LittleFS.open("/thingspeak_offline.tmp", "w");
        if (in && out) {
          in.seek(skipBytes);
          in.readStringUntil('\n');

          uint8_t copyBuf[512];
          bool copyOk = true;
          while (in.available()) {
            size_t n = in.read(copyBuf, sizeof(copyBuf));
            if (n == 0) break;
            if (out.write(copyBuf, n) != n) {
              copyOk = false;
              break;
            }
          }
          in.close();
          out.close();

          if (!copyOk) {
            LittleFS.remove("/thingspeak_offline.tmp");
            Serial.println("[offline-log] Trim copy failed; original log retained.");
          } else {
          LittleFS.remove("/thingspeak_offline.bak");
          if (!LittleFS.rename(OFFLINE_LOG_PATH, "/thingspeak_offline.bak")) {
            LittleFS.remove("/thingspeak_offline.tmp");
            Serial.println("[offline-log] Trim backup rename failed; original log retained.");
          } else if (!LittleFS.rename("/thingspeak_offline.tmp", OFFLINE_LOG_PATH)) {
            LittleFS.remove(OFFLINE_LOG_PATH);
            LittleFS.rename("/thingspeak_offline.bak", OFFLINE_LOG_PATH);
            Serial.println("[offline-log] Trim install failed; original log restored.");
          } else {
            LittleFS.remove("/thingspeak_offline.bak");
            Serial.println("[offline-log] Trimmed oversized log (kept newest ~70%).");
          }
          }
        } else {
          if (in) in.close();
          if (out) out.close();
          LittleFS.remove("/thingspeak_offline.tmp");
        }
      }
    }
  }
}
// =================================================================
// ThingSpeak REST payload
// Fields (1-8):
// 1 temperature, 2 humidity, 3 risk, 4 predicted min to breach,
// 5 anomaly type, 6 ML prediction, 7 ML probability, 8 ML quality.
// GPS is sent through the channel location parameters when valid.
// Additional audit values are carried in the ThingSpeak status string.
// =================================================================
size_t buildThingSpeakPayload(const TelemetryReading &r, char* buffer, size_t bufSize) {
  int n = snprintf(
    buffer, bufSize,
    "api_key=%s&field1=%.2f&field2=%.2f&field3=%.1f&field4=%.1f&field5=%u&field6=%u&field7=%.5f&field8=%u&status=seq_%lu_off_%0.1f_life_%0.2f_mv_%s",
    THINGSPEAK_WRITE_API_KEY,
    r.temperature,
    r.humidity,
    r.riskScore,
    r.predictedMinutesToBreach,
    (unsigned)r.anomalyType,
    (unsigned)r.mlPrediction,
    r.mlProbability,
    (unsigned)r.mlDataQuality,
    (unsigned long)r.sequence,
    r.offlineMinutes,
    r.vaccineLifeConsumedPct,
    MODEL_VERSION
  );

  if (r.gpsValid && n > 0 && (size_t)n < bufSize) {
    n += snprintf(buffer + n, bufSize - (size_t)n,
                  "&lat=%.6f&long=%.6f", r.latitude, r.longitude);
  }
  if (n < 0) return 0;
  return (size_t)n;
}

bool postThingSpeakPayload(const char* formPayload) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setTimeout(12000);
  if (!http.begin(thingSpeakClient, THINGSPEAK_UPDATE_URL)) {
    Serial.println("[cloudTask] ThingSpeak HTTPS begin failed.");
    return false;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST((uint8_t*)formPayload, strlen(formPayload));
  String response = http.getString();
  response.trim();
  http.end();

  // ThingSpeak returns a positive entry ID on a successful channel update.
  long entryId = response.toInt();
  bool ok = (httpCode == HTTP_CODE_OK && entryId > 0);

  if (ok) {
    if (xSemaphoreTake(syncTimeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      lastSuccessfulSyncMs = millis();
      xSemaphoreGive(syncTimeMutex);
    }
    Serial.printf("[cloudTask] ThingSpeak published, entry=%ld\n", entryId);
  } else {
    Serial.printf("[cloudTask] ThingSpeak write failed, HTTP=%d response='%s'\n",
                  httpCode, response.c_str());
  }
  return ok;
}

bool drainOneOfflineRecord() {
  if (!LittleFS.exists(OFFLINE_LOG_PATH)) return true;

  File in = LittleFS.open(OFFLINE_LOG_PATH, "r");
  if (!in || in.size() == 0) {
    if (in) in.close();
    LittleFS.remove(OFFLINE_LOG_PATH);
    return true;
  }

  String firstLine = in.readStringUntil('\n');
  firstLine.trim();

  // Drop a leading blank/corrupt line rather than retrying it forever.
  if (firstLine.length() == 0) {
    File out = LittleFS.open("/thingspeak_offline.tmp", "w");
    if (!out) {
      in.close();
      Serial.println("[offline-log] Failed to create temporary drain file.");
      return false;
    }
    uint8_t copyBuf[512];
    while (in.available()) {
      size_t n = in.read(copyBuf, sizeof(copyBuf));
      if (n == 0) break;
      if (out.write(copyBuf, n) != n) {
        out.close();
        in.close();
        LittleFS.remove("/thingspeak_offline.tmp");
        return false;
      }
    }
    out.close();
    in.close();
    LittleFS.remove("/thingspeak_offline.bak");
    if (!LittleFS.rename(OFFLINE_LOG_PATH, "/thingspeak_offline.bak")) {
      LittleFS.remove("/thingspeak_offline.tmp");
      return false;
    }
    if (!LittleFS.rename("/thingspeak_offline.tmp", OFFLINE_LOG_PATH)) {
      LittleFS.remove(OFFLINE_LOG_PATH);
      LittleFS.rename("/thingspeak_offline.bak", OFFLINE_LOG_PATH);
      return false;
    }
    LittleFS.remove("/thingspeak_offline.bak");
    return true;
  }

  // Read/retry exactly one record. The remainder is streamed to a temp file
  // so a 200 KB backlog never has to fit into one heap-allocated String.
  bool accepted = postThingSpeakPayload(firstLine.c_str());
  if (!accepted) {
    in.close();
    return false;
  }

  File out = LittleFS.open("/thingspeak_offline.tmp", "w");
  if (!out) {
    in.close();
    Serial.println("[offline-log] Failed to create temporary drain file.");
    return false;
  }

  uint8_t copyBuf[512];
  bool copyOk = true;
  while (in.available()) {
    size_t n = in.read(copyBuf, sizeof(copyBuf));
    if (n == 0) break;
    if (out.write(copyBuf, n) != n) {
      copyOk = false;
      break;
    }
  }
  out.close();
  in.close();

  if (!copyOk) {
    LittleFS.remove("/thingspeak_offline.tmp");
    Serial.println("[offline-log] Remainder copy failed; original backlog retained.");
    return false;
  }

  LittleFS.remove("/thingspeak_offline.bak");
  if (!LittleFS.rename(OFFLINE_LOG_PATH, "/thingspeak_offline.bak")) {
    LittleFS.remove("/thingspeak_offline.tmp");
    Serial.println("[offline-log] Drain backup rename failed; backlog retained.");
    return false;
  }
  if (!LittleFS.rename("/thingspeak_offline.tmp", OFFLINE_LOG_PATH)) {
    LittleFS.remove(OFFLINE_LOG_PATH);
    LittleFS.rename("/thingspeak_offline.bak", OFFLINE_LOG_PATH);
    Serial.println("[offline-log] Drain install failed; backlog restored.");
    return false;
  }
  LittleFS.remove("/thingspeak_offline.bak");

  File check = LittleFS.open(OFFLINE_LOG_PATH, "r");
  bool remaining = check && check.size() > 0;
  if (check) check.close();
  if (!remaining) {
    LittleFS.remove(OFFLINE_LOG_PATH);
    Serial.println("[cloudTask] ThingSpeak backlog cleared.");
  } else {
    Serial.println("[cloudTask] ThingSpeak backlog: one record drained; remainder retained.");
  }
  return true;
}
// =================================================================
// Edge-ML cadence adapter: 2-second sensing -> 10-minute model cadence
// Training features assume ~10-minute samples. We aggregate each bucket
// and only infer when coverage is adequate; irregular/missing buckets are
// flagged instead of silently pretending sample-count cadence is valid.
// =================================================================
float mlTempSum=0.0f; int mlTempCount=0; unsigned long mlBucketStart=0; bool mlBucketInitialized=false;
float mlLastTemps[5]; unsigned long mlLastBucketTs[5]; int mlLastCount=0;
int mlPositiveStreak=0; float mlLastProbability=0.0f; uint8_t mlLastPrediction=0; uint8_t mlLastQuality=0;

void resetMlHistory(){ mlTempSum=0; mlTempCount=0; mlBucketStart=0; mlBucketInitialized=false; mlLastCount=0; mlPositiveStreak=0; mlLastProbability=0; mlLastPrediction=0; mlLastQuality=0; }

void updateEdgeML(float temperature, unsigned long now){
  const unsigned long bucketId = now / ML_BUCKET_MS;
  if(!mlBucketInitialized){ mlBucketStart=bucketId * ML_BUCKET_MS; mlBucketInitialized=true; }
  unsigned long currentBucketId = mlBucketStart / ML_BUCKET_MS;
  if(bucketId < currentBucketId) { resetMlHistory(); mlBucketStart=bucketId*ML_BUCKET_MS; currentBucketId=bucketId; }
  if(bucketId > currentBucketId){
    float coverage=(float)mlTempCount/(float)(ML_BUCKET_MS/SAMPLE_INTERVAL_MS);
    if(mlTempCount>0 && coverage>=ML_MIN_BUCKET_COVERAGE){
      float bucketTemp=mlTempSum/(float)mlTempCount;
      if(mlLastCount<5){ mlLastTemps[mlLastCount]=bucketTemp; mlLastBucketTs[mlLastCount]=mlBucketStart; mlLastCount++; }
      else { for(int i=0;i<4;i++){mlLastTemps[i]=mlLastTemps[i+1];mlLastBucketTs[i]=mlLastBucketTs[i+1];} mlLastTemps[4]=bucketTemp; mlLastBucketTs[4]=mlBucketStart; }
      if(mlLastCount>=5){
        // The deployment tree was trained/evaluated only for current temperature < 8 C.
        // Never infer from a breached/out-of-domain window.
        if(mlLastTemps[4] >= ABS_MAX_TEMP_C){
          mlLastCount=0; mlPositiveStreak=0; mlLastProbability=0; mlLastPrediction=0; mlLastQuality=0;
          Serial.printf("[ML] skipped: current bucket mean %.2fC is outside model domain (< %.1fC).\n", mlLastTemps[4], ABS_MAX_TEMP_C);
        } else {
          float f[10]; float mean=0,stdv=0; for(int i=0;i<5;i++) mean+=mlLastTemps[i]; mean/=5.0f; for(int i=0;i<5;i++){float d=mlLastTemps[i]-mean;stdv+=d*d;} stdv=sqrtf(stdv/5.0f);
          float slope=(mlLastTemps[4]-mlLastTemps[0])/40.0f; float d10=mlLastTemps[4]-mlLastTemps[3]; float d20=mlLastTemps[4]-mlLastTemps[2]; float rise=0; for(int i=1;i<5;i++) rise += (mlLastTemps[i]>mlLastTemps[i-1])?1.0f:0.0f; rise/=4.0f;
          f[0]=mlLastTemps[4]; f[1]=mean; f[2]=stdv; f[3]=slope; f[4]=d10; f[5]=d20; f[6]=rise; f[7]=8.0f-mlLastTemps[4]; f[8]=(mlLastTemps[0]>6)+(mlLastTemps[1]>6)+(mlLastTemps[2]>6)+(mlLastTemps[3]>6)+(mlLastTemps[4]>6); f[9]=slope-d20/20.0f;
          mlLastProbability=edge_ml_breach_probability_v7(f); if(mlLastProbability>=EDGE_ML_THRESHOLD) mlPositiveStreak++; else mlPositiveStreak=0; mlLastPrediction=(mlPositiveStreak>=EDGE_ML_PERSISTENCE)?1:0; mlLastQuality=1;
          Serial.printf("[ML] inference p=%.3f prediction=%u streak=%d quality=%u\n", mlLastProbability, mlLastPrediction, mlPositiveStreak, mlLastQuality);
        }
      }
    } else {
      // A bad or skipped bucket breaks cadence continuity; do not infer across it.
      mlLastCount=0; mlPositiveStreak=0; mlLastProbability=0; mlLastPrediction=0; mlLastQuality=0;
    }
    mlTempSum=0; mlTempCount=0; mlBucketStart=bucketId*ML_BUCKET_MS;
    // A gap of more than one bucket invalidates the prior feature history even if the last bucket was usable.
    if(bucketId > currentBucketId + 1){ mlLastCount=0; mlPositiveStreak=0; mlLastProbability=0; mlLastPrediction=0; mlLastQuality=0; }
  }
  mlTempSum+=temperature; mlTempCount++;
}

// =================================================================
// TASK 1 (Core 0) — Sensor acquisition + edge analytics + risk score
// =================================================================
void sensorTask(void *param) {
  Wire.begin(SHT30_SDA_PIN, SHT30_SCL_PIN);

  if (!sht30.begin(0x44)) {
    Serial.println("[sensorTask] SHT30 not found — check wiring.");
  }

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  unsigned long sampleCount = 0;
  int sensorFailures = 0;
  resetMlHistory();

  for (;;) {
    float temperature = sht30.readTemperature();
    float humidity = sht30.readHumidity();
    unsigned long now = millis();

    if (isnan(temperature) || isnan(humidity)) {
      sensorFailures++;
      Serial.println("[sensorTask] Sensor read failed, skipping this cycle.");
      if(sensorFailures>=SENSOR_REINIT_AFTER_FAILURES){ sensorFailures=0; sht30.begin(0x44); resetMlHistory(); Serial.println("[sensorTask] Sensor reinitialization attempted."); }
      vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
      continue;
    }
    sensorFailures=0;

    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
    if (sampleCount % GPS_READ_EVERY_N_SAMPLES == 0) {
      if (gps.location.isValid()) {
        lastKnownLat = gps.location.lat();
        lastKnownLng = gps.location.lng();
        lastGpsValid = true;
        lastGpsUpdateMs = now;
      }
    }

    if(lastGpsValid && (now-lastGpsUpdateMs)>GPS_MAX_AGE_MS) lastGpsValid=false;
    updateEdgeML(temperature, now);

    float predictedMinutes = 0.0;
    uint8_t anomalyType = evaluateAnomaly(temperature, now, &predictedMinutes);

    double minutesSinceSync = 0.0;
    if (xSemaphoreTake(syncTimeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      minutesSinceSync = (now - lastSuccessfulSyncMs) / 60000.0;
      xSemaphoreGive(syncTimeMutex);
    }

    float risk = computeRiskScore(temperature, predictedMinutes,
                                   cumulativeDamageFraction * 100.0, minutesSinceSync);

    Serial.printf("[detect] T=%.2fC predictBreach=%.1fmin lifeConsumed=%.1f%% offlineMin=%.1f risk=%.1f type=%d\n",
                  temperature, predictedMinutes, cumulativeDamageFraction * 100.0,
                  minutesSinceSync, risk, anomalyType);

    TelemetryReading reading;
    reading.temperature = temperature;
    reading.humidity = humidity;
    reading.latitude = lastKnownLat;
    reading.longitude = lastKnownLng;
    reading.gpsValid = lastGpsValid;
    reading.anomalyType = anomalyType;
    reading.predictedMinutesToBreach = predictedMinutes;
    reading.vaccineLifeConsumedPct = (float)(cumulativeDamageFraction * 100.0);
    reading.offlineMinutes = (float)minutesSinceSync;
    reading.riskScore = risk;
    reading.timestampMs = now;
    reading.sequence = ++sequenceCounter;
    reading.mlPrediction = mlLastPrediction;
    reading.mlProbability = mlLastProbability;
    reading.mlDataQuality = mlLastQuality;

    if (xQueueSend(telemetryQueue, &reading, 0) != pdTRUE) {
      TelemetryReading discard;
      xQueueReceive(telemetryQueue, &discard, 0);
      xQueueSend(telemetryQueue, &reading, 0);
    }

    sampleCount++;
    vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
  }
}

// =================================================================
// TASK 2 (Core 1) — WiFi + ThingSpeak cloud sync
// =================================================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("[cloudTask] Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[cloudTask] WiFi connected.");
    WiFi.setSleep(true);
    Serial.printf("[cloudTask] ThingSpeak channel=%s update_interval=%lus\n",
                  THINGSPEAK_CHANNEL_ID,
                  THINGSPEAK_UPDATE_INTERVAL_MS / 1000UL);
    Serial.println("[SECURITY] ThingSpeak HTTPS certificate validation ENABLED (DigiCert Global Root CA).");
  } else {
    Serial.println("[cloudTask] WiFi connect timed out, will retry.");
  }
}

void cloudTask(void *param) {
  unsigned long lastCloudAttemptMs = 0;
  TelemetryReading latestReading{};
  bool haveLatestReading = false;

  for (;;) {
    connectWiFi();

    // Always consume the queue so the 2-second sensor stream does not back up.
    // We only persist/transmit the latest reading at ThingSpeak's 15-second limit.
    TelemetryReading queued;
    while (xQueueReceive(telemetryQueue, &queued, 0) == pdTRUE) {
      latestReading = queued;
      haveLatestReading = true;
    }

    unsigned long now = millis();
    bool writeSlotDue = (lastCloudAttemptMs == 0) ||
                        ((unsigned long)(now - lastCloudAttemptMs) >= THINGSPEAK_UPDATE_INTERVAL_MS);

    if (writeSlotDue) {
      if (WiFi.status() == WL_CONNECTED) {
        // Recover durable backlog first, one ThingSpeak message per 15-second slot.
        bool backlogExists = LittleFS.exists(OFFLINE_LOG_PATH);
        if (backlogExists) {
          if (!drainOneOfflineRecord()) {
            Serial.println("[cloudTask] Backlog record not accepted; retained for retry.");
          }
        } else if (haveLatestReading) {
          char payload[768];
          size_t len = buildThingSpeakPayload(latestReading, payload, sizeof(payload));
          if (len == 0 || len >= sizeof(payload)) {
            Serial.println("[cloudTask] ThingSpeak payload build failed.");
          } else if (!postThingSpeakPayload(payload)) {
            appendToOfflineLog(payload);
            Serial.println("[cloudTask] Latest reading retained in LittleFS.");
          }
        }
      } else if (haveLatestReading) {
        char payload[768];
        size_t len = buildThingSpeakPayload(latestReading, payload, sizeof(payload));
        if (len > 0 && len < sizeof(payload)) {
          appendToOfflineLog(payload);
          Serial.println("[cloudTask] WiFi unavailable; latest reading saved to LittleFS.");
        }
      }

      // Start the next 15-second window after this slot has actually been
      // processed, so a slow HTTPS request cannot cause two writes to occur
      // less than 15 seconds apart.
      lastCloudAttemptMs = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// =================================================================
// setup() / loop()
// =================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nCold Chain Monitor v8.3.1 (MVP: Edge ML + ThingSpeak + durable offline fallback) booting...");

  if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") == 0 || strcmp(THINGSPEAK_WRITE_API_KEY, "YOUR_THINGSPEAK_WRITE_API_KEY") == 0) {
    Serial.println("WARNING: Update WiFi and ThingSpeak credentials in the CONFIG section before upload.");
  }

  if (!LittleFS.begin(true)) {
    Serial.println("WARNING: LittleFS mount failed — offline logging will not work.");
  } else {
    recoverOfflineLogState();
  }

  // Avoid rewriting Wi-Fi credentials to NVS on every reconnect attempt.
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  computeArrheniusFit();
  thingSpeakClient.setCACert(THINGSPEAK_ROOT_CA);
  lastSuccessfulSyncMs = millis();

  telemetryQueue = xQueueCreate(TELEMETRY_QUEUE_LENGTH, sizeof(TelemetryReading));
  syncTimeMutex = xSemaphoreCreateMutex();

  if (telemetryQueue == NULL || syncTimeMutex == NULL) {
    Serial.println("FATAL: could not create queue/mutex.");
    while (1) delay(1000);
  }

  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(cloudTask, "CloudTask", 8192, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
