#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "edge_ml_tree_v7.h"
#define ABS_MAX_TEMP_C 8.0
#define ML_BUCKET_MS 600000UL
#define SAMPLE_INTERVAL_MS 2000
#define ML_MIN_BUCKET_COVERAGE 0.80f
#define EDGE_ML_THRESHOLD 0.85f
#define EDGE_ML_PERSISTENCE 2
struct SerialStub{template<typename... Args> void printf(const char* fmt, Args... args){(void)fmt;(void)sizeof...(args);}} Serial;
float mlTempSum=0; int mlTempCount=0; unsigned long mlBucketStart=0; bool mlBucketInitialized=false;
float mlLastTemps[5]; unsigned long mlLastBucketTs[5]; int mlLastCount=0; int mlPositiveStreak=0; float mlLastProbability=0; uint8_t mlLastPrediction=0; uint8_t mlLastQuality=0;
void resetMlHistory(){mlTempSum=0;mlTempCount=0;mlBucketStart=0;mlBucketInitialized=false;mlLastCount=0;mlPositiveStreak=0;mlLastProbability=0;mlLastPrediction=0;mlLastQuality=0;}
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
static void sample(unsigned long t,float v){ updateEdgeML(v,t); }
int main(){
resetMlHistory(); for(unsigned long t=0;t<4200000UL;t+=2000) sample(t,5.0f); std::cout<<"stable q="<<(int)mlLastQuality<<" p="<<mlLastProbability<<" pred="<<(int)mlLastPrediction<<"\n"; if(mlLastQuality!=1||mlLastPrediction!=0) return 1;
resetMlHistory(); for(int i=0;i<2100;i++){float v=5.0f+2.7f*i/2099.0f; sample((unsigned long)i*2000,v);} std::cout<<"warm q="<<(int)mlLastQuality<<" p="<<mlLastProbability<<" pred="<<(int)mlLastPrediction<<" streak="<<mlPositiveStreak<<"\n"; if(mlLastQuality!=1) return 2;
resetMlHistory(); for(int b=0;b<5;b++){ for(int i=0;i<240;i++) sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,5.0f);} sample(3000000UL,5.0f); std::cout<<"80pct q="<<(int)mlLastQuality<<" count="<<mlLastCount<<"\n"; if(mlLastQuality!=1||mlLastCount!=5)return 3;
resetMlHistory(); for(int b=0;b<5;b++){ for(int i=0;i<239;i++) sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,5.0f);} sample(3000000UL,5.0f); if(mlLastQuality!=0||mlLastCount!=0)return 4;
resetMlHistory(); for(int b=0;b<5;b++){for(int i=0;i<300;i++)sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,7.5f);} for(int i=0;i<299;i++)sample(3000000UL+(unsigned long)i*2000UL,8.0f); sample(3600000UL,8.0f); if(mlLastQuality!=0||mlLastPrediction!=0)return 5;
resetMlHistory();
float traj[8]={5.0f,5.4f,5.8f,6.2f,6.6f,7.0f,7.4f,7.7f};
for(int b=0;b<8;b++){for(int i=0;i<300;i++)sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,traj[b]);}
std::cout<<"persistence_test p="<<mlLastProbability<<" pred="<<(int)mlLastPrediction<<" streak="<<mlPositiveStreak<<"\n";
if(mlLastPrediction!=1 || mlPositiveStreak<2) return 6;
resetMlHistory();
for(int b=0;b<5;b++){for(int i=0;i<300;i++)sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,7.5f);}
for(int i=0;i<300;i++)sample(3000000UL+(unsigned long)i*2000UL,8.2f);
sample(3600000UL,8.2f);
std::cout<<"domain_guard q="<<(int)mlLastQuality<<" pred="<<(int)mlLastPrediction<<" count="<<mlLastCount<<"\n";
if(mlLastQuality!=0 || mlLastPrediction!=0 || mlLastCount!=0) return 7;
resetMlHistory();
for(int b=0;b<5;b++){for(int i=0;i<300;i++)sample((unsigned long)b*600000UL+(unsigned long)i*2000UL,5.0f);}
sample(4200000UL,5.0f);
if(mlLastQuality!=0 || mlLastCount!=0) return 8;
std::cout<<"ALL_V8_3_2_CADENCE_TESTS_PASS\n"; return 0;}
