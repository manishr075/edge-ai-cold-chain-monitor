# SIH26215 — v8.3.2 Engineering Audit

Date: 2026-09-28

## Scope

Continued from checkpoint v8.3.1. Reviewed the production ESP32 firmware, embedded Decision Tree, ThingSpeak integration, ML cadence adapter, offline logging, presentation and demo path. No retraining or replacement of the validated tree was performed.

## v8.3.2 fixes

### 1. LittleFS transaction recovery
Added startup recovery for interrupted trim/drain operations:
- if the main backlog vanished but the backup exists, restore the backup;
- if only the temporary file remains, recover it as a best-effort backlog;
- stale backup/temp files are cleaned up when the main backlog is authoritative.

### 2. Wi-Fi NVS wear reduction
The firmware now uses:
```cpp
WiFi.persistent(false);
WiFi.setAutoReconnect(true);
```
so repeated reconnect attempts do not unnecessarily rewrite Wi-Fi credentials to persistent NVS storage.

### 3. Freeze-detection wording corrected
The code no longer describes freezing as universally irreversible for “most vaccines”. The one-way latch is explicitly scoped to the selected demonstration target; real product-specific freeze sensitivity must be validated before deployment.

### 4. Version consistency
Production boot string and model version are now aligned to v8.3.2:
`tree120m_v2_dev48_thr085_v8_3_2_secure_ts`

## Full source/API syntax audit

The complete firmware was checked with Clang using a purpose-built Arduino/ESP32/FreeRTOS API stub layer. Result:

**Syntax check: PASS — zero compiler errors.**

This is not a native ESP32 toolchain build; Arduino CLI/PlatformIO are not installed in the execution environment. The check catches C/C++ syntax, missing symbols and API-shape errors in the source-level integration.

## Security verification

The embedded ThingSpeak root certificate was extracted and parsed with OpenSSL:
- Subject: DigiCert Global Root CA
- Issuer: DigiCert Global Root CA
- SHA-256 fingerprint: `43:48:A0:E9:44:4C:78:CB:26:5E:05:8D:5E:89:44:B4:D8:4F:96:62:BD:26:DB:25:7F:89:34:A4:43:C7:01:61`
- Validity: 2006-11-10 through 2031-11-10

MathWorks currently documents the DigiCert Global Root CA for ThingSpeak certificate use, and the REST endpoint is `https://api.thingspeak.com/update`. citeturn966690search0turn966690search1

## ThingSpeak verification

Current MathWorks documentation states:
- secure REST base: `https://api.thingspeak.com`
- POST/GET write endpoint: `/update`
- channel Write API Key authorizes writes
- free accounts have a 15-second channel update interval

The firmware uses a 15-second pacing window and eight fields. citeturn966690search1turn966690search2

## ML verification

The v8.3.2 cadence harness passed:
- stable 5°C -> valid ML state with low probability and no prediction
- monotonic 5–7.7°C warming -> positive predictive state
- exactly 80% bucket coverage -> accepted
- below 80% coverage -> reset/reject
- current bucket >=8°C -> domain guard suppresses inference
- cadence gap -> history reset
- persistence logic exercised

The actual embedded `edge_ml_tree_v7.h` was used; no substitute classifier was used.

## Important evidence boundary

Primary model validation remains the frozen historical Nepal source-file holdout:
- 6,110 rows
- 12 held-out source files
- Precision 66.3%
- Recall 68.1%
- F1 67.2%
- ROC-AUC 0.907

These are historical-data validation metrics, not Bengaluru field accuracy and not physical v8.3.2 runtime measurements.

## Remaining manual/physical work

1. Native ESP32 compile/upload of v8.3.2.
2. Secure ThingSpeak publish on the physical board.
3. Controlled 2–8°C physical ML runtime.
4. Optional GPS satellite-fix recovery.
5. Battery/power characterization for any deployment claim.

## Engineering assessment

Software risk is now concentrated in physical/toolchain validation rather than known C/C++ syntax, ML cadence, TLS configuration, queue handoff, or offline-log transaction handling.
