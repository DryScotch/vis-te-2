#include "audio_analysis.h"
#include <math.h>
#include <string.h>
#define SZ 2048
#define PI 3.14159265358979323846f
static float history[SZ], re[SZ], im[SZ];
static unsigned write_pos=0;
static int sample_rate=16000, filled=0;
static AudioLevels state;
static float prev_power=0;
static float clip(float x){return x<0?0:(x>1?1:x);}
void audio_reset(void){memset(history,0,sizeof(history));memset(&state,0,sizeof(state));write_pos=0;filled=0;prev_power=0;}
void audio_push(const int16_t *mono,int count,int rate){
 if(!mono||count<=0)return;
 if(rate!=sample_rate){sample_rate=rate;write_pos=0;filled=0;memset(history,0,sizeof(history));}
 for(int i=0;i<count;i++){history[write_pos++ & (SZ-1)]=mono[i]/32768.0f;if(filled<SZ)filled++;}
}
static void fft(int n){
 for(int i=1,j=0;i<n;i++){
  int bit=n>>1;
  for(;j&bit;bit>>=1)j^=bit;
  j^=bit;
  if(i<j){float t=re[i];re[i]=re[j];re[j]=t;t=im[i];im[i]=im[j];im[j]=t;}
 }
 for(int len=2;len<=n;len<<=1){
  float angle=-2*PI/len, wr_step=cosf(angle),wi_step=sinf(angle);
  for(int i=0;i<n;i+=len){float wr=1,wi=0;
   for(int j=0;j<len/2;j++){
    int a=i+j,b=a+len/2;
    float tr=wr*re[b]-wi*im[b],ti=wr*im[b]+wi*re[b];
    re[b]=re[a]-tr;im[b]=im[a]-ti;re[a]+=tr;im[a]+=ti;
    float nr=wr*wr_step-wi*wi_step;wi=wr*wi_step+wi*wr_step;wr=nr;
   }
  }
 }
}
static float envelope(float cur,float old,float atk,float release){return old+(cur-old)*(cur>old?atk:release);}
AudioLevels audio_analyze(float gain){
 int n=sample_rate==48000?2048:1024;
 if(filled<n) return state;
 float rms2=0;
 for(int i=0;i<n;i++){
  float x=history[(write_pos-n+i)&(SZ-1)];rms2+=x*x;
  float window=.5f-.5f*cosf(2*PI*i/(n-1));re[i]=x*window;im[i]=0;
 }
 fft(n);
 float power[3]={0}, bins[3]={0};
 for(int i=2;i<n/2;i++){
  float hz=(float)i*sample_rate/n;
  int band=(hz<180)?0:(hz<(sample_rate==16000?2000:3000)?1:2);
  if(hz<40||hz>(sample_rate==16000?7000:14000))continue;
  float mag=sqrtf(re[i]*re[i]+im[i]*im[i])/(n*.5f);
  power[band]+=mag*mag;bins[band]++;
 }
 float bass=clip(sqrtf(power[0]/(bins[0]+1))*gain*18);
 float mids=clip(sqrtf(power[1]/(bins[1]+1))*gain*24);
 float highs=clip(sqrtf(power[2]/(bins[2]+1))*gain*26);
 float rms=clip(sqrtf(rms2/n)*gain*3.0f);
 state.bass=envelope(bass,state.bass,.48f,.07f);
 state.mids=envelope(mids,state.mids,.36f,.12f);
 state.highs=envelope(highs,state.highs,.4f,.16f);
 state.rms=envelope(rms,state.rms,.45f,.1f);
 float onset=clip((rms-prev_power)*6.f);prev_power=prev_power*.72f+rms*.28f;
 state.transient=envelope(onset,state.transient,.85f,.18f);
 if(rms<.008f){state.transient*=.55f;}
 return state;
}
