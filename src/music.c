#include "music.h"
#include "miniaudio.h"
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <string.h>
#define BLOCK 1024
#define WAVE_CAP 2048
static ma_decoder decoder;
static int decoder_open=0,port=-1,thread_id=-1;
static volatile int running=0,paused=0,finished=0;
static volatile int front=0;
static int16_t wave[2][WAVE_CAP];
static unsigned pos=0;
static volatile unsigned published_pos=0;
static int worker(unsigned int argc, void *arg){
 (void)argc;(void)arg;
 int16_t out[BLOCK*2];
 while(running){
  if(paused){sceKernelDelayThread(15000);continue;}
  ma_uint64 n=0;
  ma_result r=ma_decoder_read_pcm_frames(&decoder,out,BLOCK,&n);
  if(n==0){finished=1;break;}
  for(int i=(int)n;i<BLOCK;i++){out[i*2]=0;out[i*2+1]=0;}
  /* Publish stable 2048-sample snapshots without exposing writes to the renderer. */
  int back=1-front;
  memcpy(wave[back],wave[front],sizeof(wave[front]));
  for(int i=0;i<(int)n;i++)wave[back][pos++&(WAVE_CAP-1)]=(int16_t)(((int)out[i*2]+(int)out[i*2+1])/2);
  published_pos=pos;__sync_synchronize();front=back;
  if(sceAudioOutOutput(port,out)<0){finished=1;break;}
  if(r!=MA_SUCCESS&&n<BLOCK){finished=1;break;}
 }
 return 0;
}
int music_start(const char *path){
 music_stop();
 ma_decoder_config cfg=ma_decoder_config_init(ma_format_s16,2,48000);
 if(ma_decoder_init_file(path,&cfg,&decoder)!=MA_SUCCESS)return -1;
 decoder_open=1;port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,BLOCK,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(port<0){music_stop();return -2;}
 memset(wave,0,sizeof(wave));pos=0;published_pos=0;front=0;running=1;paused=0;finished=0;
 thread_id=sceKernelCreateThread("vml_music",worker,0x10000100,256*1024,0,0,NULL);
 if(thread_id<0){music_stop();return -3;}
 if(sceKernelStartThread(thread_id,0,NULL)<0){music_stop();return -4;}
 return 0;
}
void music_stop(void){
 running=0;
 if(thread_id>=0){sceKernelWaitThreadEnd(thread_id,NULL,NULL);sceKernelDeleteThread(thread_id);thread_id=-1;}
 if(port>=0){sceAudioOutReleasePort(port);port=-1;}
 if(decoder_open){ma_decoder_uninit(&decoder);decoder_open=0;}
 memset(wave,0,sizeof(wave));pos=0;published_pos=0;front=0;paused=0;finished=0;
}
void music_set_paused(int v){paused=v;}
int music_is_active(void){return running&&!finished;}
int music_is_finished(void){return finished;}
void music_get_wave(int16_t *dest,int count){
 if(count<=0)return;if(count>WAVE_CAP)count=WAVE_CAP;
 int id=front;__sync_synchronize();
 unsigned p=published_pos;/* latest completed snapshot, not a partially written block */
 for(int i=0;i<count;i++)dest[i]=wave[id][(p-count+i)&(WAVE_CAP-1)];
}
