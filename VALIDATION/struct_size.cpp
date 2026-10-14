#include <iostream>
#include <cstdint>
struct TelemetryReading {
  float temperature;
  float humidity;
  double latitude;
  double longitude;
  bool gpsValid;
  uint8_t anomalyType;
  float predictedMinutesToBreach;
  float vaccineLifeConsumedPct;
  float offlineMinutes;
  float riskScore;
  unsigned long timestampMs;
  uint32_t sequence;
  uint8_t mlPrediction;
  float mlProbability;
  uint8_t mlDataQuality;
};
int main(){std::cout<<sizeof(TelemetryReading)<<" bytes; queue= "<<sizeof(TelemetryReading)*150<<" bytes\n";}
