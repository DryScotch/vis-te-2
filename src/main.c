#include <psp2/audioin.h>
#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "music.h"
#include "audio_analysis.h"
#include "scene_renderer.h"
#define SCREEN_W 960
#define SCREEN_H 544
#define MAX_TRACKS 80
#define C(r,g,b) RGBA8(r,g,b,255)
typedef enum {MAIN,SOURCES,SCENES,BROWSER,PALETTES,SETTINGS,ABOUT,PLAYING} Page;
typedef enum {MIC,FILE_SOURCE,LOOP} Source;
typedef struct {const char *name;uint32_t primary,light;} Palette;
static const Palette palettes[12]={
 {"ELECTRIC BLUE",C(38,136,233),C(198,242,255)},
 {"CYAN ICE",C(39,213,244),C(228,255,255)},
 {"BLUE GOLD",C(246,189,79),C(255,243,188)},
 {"PURPLE NEON",C(171,105,250),C(235,221,255)},
 {"MAGENTA VIOLET",C(244,91,215),C(255,230,253)},
 {"TOXIC GREEN",C(118,237,80),C(219,255,181)},
 {"RED EMBER",C(251,82,88),C(255,240,223)},
 {"AMBER CHROME",C(255,182,76),C(255,242,208)},
 {"WHITE ICE",C(211,235,248),C(255,255,255)},
 {"TEAL AQUA",C(41,221,203),C(221,255,247)},
 {"PINK CYBER",C(255,92,190),C(255,227,250)},
 {"MONO BLUE-BLACK",C(64,128,213),C(175,216,248)}
};
static const char *items[]={"START VISUALIZER","AUDIO SOURCE","VISUALIZERS","MUSIC BROWSER","COLOR SCHEMES","SETTINGS","ABOUT"};
static const char *sources[]={"MICROPHONE","MUSIC PLAYER","ANIMATION LOOP"};
static const char *options[]={"MIC GAIN","ANIMATION SPEED","OVERLAY","DEMO AUTOPLAY","CRT SCANLINES","PERFORMANCE","RESET DEFAULTS"};
static const char *speed_names[]={"SLOW 0.5X","NORMAL 1X","FAST 1.5X"};
static const float speeds[]={.5f,1.f,1.5f};
static const char *autoplay_names[]={"OFF","30 SECONDS","60 SECONDS","120 SECONDS"};
static const float autoplay_sec[]={0,30,60,120};
static const char *perf_names[]={"QUALITY","BALANCED","FAST"};
static Page page=MAIN;
static Source source=LOOP;
static int sel[8]={0},scene=0,palette=0,gain=5,overlay=1,scanlines=0,speed=1,autoplay=0,perf=1;
static int overlay_pin=0,paused=0,micport=-1,mic_error=0,music_error=0,track_count=0,track_sel=0,track_active=-1;
static int16_t samples[2048],mic_chunk[256];
static char tracks[MAX_TRACKS][256];
static vita2d_pgf *font=NULL;
static float scene_clock=0.f,scene_age=0.f,ui_age=0.f;
static AudioLevels sound;
static void tx(int x,int y,float size,uint32_t color,const char *s){if(font)vita2d_pgf_draw_text(font,x,y,color,size,s);}
static uint32_t pri(void){return palettes[palette].primary;}
static uint32_t hi(void){return palettes[palette].light;}
static void rect(int x,int y,int w,int h,uint32_t c){vita2d_draw_rectangle(x,y,w,h,c);}
static void title(const char *s,const char *hint){tx(52,72,1.52f,hi(),s);tx(56,102,.81f,C(138,172,204),hint);rect(51,115,856,2,RGBA8(70,119,158,150));}
static void backdrop(void){rect(0,0,960,544,C(1,3,8));for(int y=0;y<544;y+=24)rect(0,y,960,1,RGBA8(29,76,120,18));}
static void footer(const char *s){rect(44,487,872,1,RGBA8(80,145,203,130));tx(51,519,.82f,C(147,186,208),s);}
static int cmp_tracks(const void *a,const void *b){return strcmp((const char*)a,(const char*)b);}
static int ext_ok(const char *filename){const char *p=strrchr(filename,'.');if(!p)return 0;char e[8]={0};snprintf(e,sizeof(e),"%s",p);for(int i=0;e[i];i++)if(e[i]>='A'&&e[i]<='Z')e[i]+='a'-'A';return !strcmp(e,".mp3")||!strcmp(e,".wav")||!strcmp(e,".flac");}
static void scan_music(void){
 track_count=0;track_sel=0;
 int fd=sceIoDopen("ux0:data/CabinMusic");if(fd<0)return;
 SceIoDirent ent;memset(&ent,0,sizeof(ent));
 while(track_count<MAX_TRACKS&&sceIoDread(fd,&ent)>0){
  if(ext_ok(ent.d_name)){snprintf(tracks[track_count],256,"%s",ent.d_name);track_count++;}
  memset(&ent,0,sizeof(ent));
 }
 sceIoDclose(fd);
 qsort(tracks,track_count,sizeof(tracks[0]),cmp_tracks);
}
static void defaults(void){source=LOOP;scene=0;palette=0;gain=5;overlay=1;scanlines=0;speed=1;autoplay=0;perf=1;}
static void save_settings(void){
 sceIoMkdir("ux0:data/VitaMotionLab",0777);
 int fd=sceIoOpen("ux0:data/VitaMotionLab/settings.tmp",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0666);
 if(fd<0)return;
 char s[256];int n=snprintf(s,sizeof(s),"1 %d %d %d %d %d %d %d %d %d\n",(int)source,scene,palette,gain,overlay,scanlines,speed,autoplay,perf);
 sceIoWrite(fd,s,n);sceIoClose(fd);
 sceIoRemove("ux0:data/VitaMotionLab/settings.ini");
 sceIoRename("ux0:data/VitaMotionLab/settings.tmp","ux0:data/VitaMotionLab/settings.ini");
}
static void load_settings(void){
 defaults();int fd=sceIoOpen("ux0:data/VitaMotionLab/settings.ini",SCE_O_RDONLY,0);if(fd<0)return;
 char s[256]={0};int n=sceIoRead(fd,s,sizeof(s)-1);sceIoClose(fd);if(n<=0)return;
 int ver,so,sc,pa,g,ov,cr,sp,au,pe;
 if(sscanf(s,"%d %d %d %d %d %d %d %d %d %d",&ver,&so,&sc,&pa,&g,&ov,&cr,&sp,&au,&pe)!=10||ver!=1)return;
 if(so>=0&&so<3)source=(Source)so;
 if(sc>=0&&sc<HERO_SCENES)scene=sc;
 if(pa>=0&&pa<12)palette=pa;
 if(g>=1&&g<=10)gain=g;
 if(ov>=0&&ov<=1)overlay=ov;
 if(cr>=0&&cr<=1)scanlines=cr;
 if(sp>=0&&sp<=2)speed=sp;
 if(au>=0&&au<=3)autoplay=au;
 if(pe>=0&&pe<=2)perf=pe;
}
static void stop_audio(void){
 music_stop();if(micport>=0){sceAudioInReleasePort(micport);micport=-1;}audio_reset();memset(&sound,0,sizeof(sound));
}
static void leave_play(void){stop_audio();save_settings();page=MAIN;paused=0;}
static void start_play(void){
 if(source==FILE_SOURCE && (track_active<0 || track_active>=track_count)){scan_music();page=BROWSER;return;}
 stop_audio();mic_error=0;music_error=0;paused=0;scene_clock=0;scene_age=0;ui_age=0;overlay_pin=0;
 if(source==MIC){micport=sceAudioInOpenPort(SCE_AUDIO_IN_PORT_TYPE_VOICE,256,16000,SCE_AUDIO_IN_PARAM_FORMAT_S16_MONO);if(micport<0)mic_error=1;}
 if(source==FILE_SOURCE){
  if(track_active<0||track_active>=track_count){music_error=1;return;}
  char path[400];snprintf(path,sizeof(path),"ux0:data/CabinMusic/%s",tracks[track_active]);
  music_error=music_start(path);
 }
 scene_activate(scene);page=PLAYING;
}
static void change_scene(int next){
 scene=(next+HERO_SCENES)%HERO_SCENES;scene_clock=0;scene_age=0;ui_age=0;scene_activate(scene);save_settings();
}
static void draw_select_list(const char *const *texts,int count,int selected,int start_y,int stride){
 for(int i=0;i<count;i++){
  int y=start_y+i*stride;
  if(i==selected){rect(63,y-30,820,stride-4,RGBA8(15,50,88,240));rect(63,y-30,4,stride-4,pri());}
  tx(87,y,1.02f,i==selected?hi():C(143,175,209),texts[i]);
 }
}
static void draw_main(void){
 title("VITA MOTION LAB", "V1.0 ALPHA  /  OLED  /  FIVE HERO SCENES");
 draw_select_list(items,7,sel[MAIN],170,43);
 char detail[180];snprintf(detail,sizeof(detail),"SOURCE: %s  |  SCENE: %s  |  PALETTE: %s",sources[source],scene_title(scene),palettes[palette].name);
 tx(55,472,.73f,pri(),detail);
 footer("DPAD SELECT  X OPEN  START LAUNCH  TRIANGLE PALETTE  CIRCLE EXIT");
}
static void draw_sources(void){title("AUDIO SOURCE","CHOOSE LIVE MIC, LOCAL AUDIO, OR OFFLINE ANIMATION");draw_select_list(sources,3,sel[SOURCES],209,73);
 footer("X CHOOSE   CIRCLE BACK");}
static void draw_scenes(void){
 title("VISUALIZERS","ART-DIRECTED LOOPING SCENES  /  3D-STYLE OFFLINE RENDERING");
 for(int i=0;i<HERO_SCENES;i++){
  int y=180+i*61;
  if(sel[SCENES]==i){rect(65,y-30,825,53,RGBA8(20,55,95,240));rect(65,y-30,4,53,pri());}
  char count[8];snprintf(count,sizeof(count),"%02d",i+1);
  tx(88,y,1.0f,i==sel[SCENES]?hi():C(148,178,206),count);
  tx(140,y,1.06f,i==sel[SCENES]?hi():C(170,190,209),scene_title(i));
  tx(524,y,.79f,C(108,159,202),scene_description(i));
 }
 footer("X CHOOSE SCENE   START PLAY   CIRCLE BACK");
}
static void draw_browser(void){
 title("MUSIC BROWSER","ux0:data/CabinMusic/  -  MP3  WAV  FLAC");
 if(!track_count){tx(80,247,1.0f,hi(),"NO TRACKS FOUND");tx(80,288,.9f,C(161,194,225),"COPY AUDIO FILES INTO ux0:data/CabinMusic/");}
 int first=(track_sel>7)?track_sel-7:0;
 for(int i=first;i<track_count&&i<first+9;i++){
  int y=158+(i-first)*39;
  if(i==track_sel)rect(61,y-25,833,34,RGBA8(16,53,92,255));
  char name[65];snprintf(name,sizeof(name),"%.59s",tracks[i]);
  tx(80,y,.90f,(i==track_sel)?hi():C(175,201,224),name);
 }
 footer("X PLAY SELECTED FILE   CIRCLE BACK");
}
static void draw_palettes(void){
 title("COLOR SCHEMES","12 UI AND REACTIVE LIGHT COLORWAYS  /  HERO ART REMAINS BLUE");
 for(int i=0;i<12;i++){
  int col=i%4,row=i/4,x=60+col*222,y=160+row*98;
  rect(x,y,202,80,i==palette?RGBA8(20,60,92,235):RGBA8(7,21,37,255));
  rect(x+8,y+8,185,18,palettes[i].primary);
  rect(x+8,y+27,185,8,palettes[i].light);
  char label[34];snprintf(label,sizeof(label),"%02d %.17s",i+1,palettes[i].name);
  tx(x+9,y+62,.73f,i==palette?hi():C(147,174,201),label);
 }
 footer("DPAD MOVE   X SELECT   CIRCLE BACK");
}
static void draw_settings(void){
 title("SETTINGS","SOURCE GAIN, LOOP PREFERENCES AND OLED DISPLAY OPTIONS");
 for(int i=0;i<7;i++){
  int y=158+i*47;
  if(sel[SETTINGS]==i)rect(69,y-28,822,41,RGBA8(21,57,97,255));
  tx(85,y,.95f,sel[SETTINGS]==i?hi():C(158,185,215),options[i]);
  char val[80]="";
  if(i==0)snprintf(val,sizeof(val),"%d / 10",gain);
  if(i==1)snprintf(val,sizeof(val),"%s",speed_names[speed]);
  if(i==2)snprintf(val,sizeof(val),"%s",overlay?"ON":"OFF");
  if(i==3)snprintf(val,sizeof(val),"%s",autoplay_names[autoplay]);
  if(i==4)snprintf(val,sizeof(val),"%s",scanlines?"ON":"OFF");
  if(i==5)snprintf(val,sizeof(val),"%s",perf_names[perf]);
  if(i==6)snprintf(val,sizeof(val),"X TO RESET");
  tx(644,y,.82f,pri(),val);
 }
 footer("UP/DOWN ITEM   LEFT/RIGHT CHANGE   CIRCLE SAVE + BACK");
}
static void draw_about(void){
 title("ABOUT VITA MOTION LAB","VERSION 1.0 ALPHA - TEST BUILD");
 tx(75,176,.99f,hi(),"LIVE MIC  /  LOCAL MUSIC  /  OFFLINE LOOPS");
 tx(75,215,.88f,C(158,192,225),"FIVE ART-DIRECTED HERO SCENES.  12 ACCENT PALETTES.");
 tx(75,254,.88f,C(158,192,225),"REFERENCE VIDEO FRAMES + SEPARATE ANIMATED SPRITES.");
 tx(75,293,.88f,C(158,192,225),"VITA OUTPUT: 960x544  |  PIXEL ART: 320x180.");
 tx(75,332,.88f,C(158,192,225),"NO INTERNET OR MICROPHONE RECORDING.");
 tx(75,371,.88f,C(158,192,225),"NOT HARDWARE-VALIDATED. BACK UP YOUR PRIOR VPK.");
 footer("CIRCLE BACK");
}
static void draw_playback(void){
 scene_draw(scene,scene_clock,sound,pri());
 if(scanlines){for(int y=2;y<544;y+=6)rect(0,y,960,1,RGBA8(0,0,0,38));}
 if((overlay&&(ui_age<4.0f||overlay_pin))||mic_error||music_error){
  rect(0,0,960,48,RGBA8(0,5,13,207));
  tx(24,32,1.02f,hi(),scene_title(scene));
  char right[128];snprintf(right,sizeof(right),"%s / %s",sources[source],palettes[palette].name);tx(575,32,.76f,pri(),right);
  rect(0,498,960,46,RGBA8(0,5,13,195));
  char label[110];snprintf(label,sizeof(label),"BASS %02d  MID %02d  HIGH %02d  %s",(int)(sound.bass*99),(int)(sound.mids*99),(int)(sound.highs*99),paused?"PAUSED":"LIVE");
  tx(24,528,.87f,hi(),label);
  if(source==FILE_SOURCE&&track_active>=0){char shortname[49];snprintf(shortname,sizeof(shortname),"%.43s",tracks[track_active]);tx(556,528,.78f,pri(),shortname);}
  else tx(592,528,.73f,pri(),"L/R STYLE  TRIANGLE COLOR");
 }
 if(mic_error||music_error){rect(93,221,775,105,RGBA8(7,15,30,244));
  tx(119,271,1.04f,C(255,140,120),mic_error?"MIC UNAVAILABLE - CIRCLE TO RETURN":"TRACK FAILED TO OPEN - CIRCLE TO RETURN");}
 if(source==FILE_SOURCE&&music_is_finished()){
  rect(138,220,686,70,RGBA8(7,15,30,225));tx(170,263,.98f,hi(),"TRACK FINISHED  -  CIRCLE FOR MENU");
 }
 if(paused){tx(418,282,1.5f,hi(),"PAUSED");}
}
static void change_option(int dir){
 switch(sel[SETTINGS]){
 case 0: gain=(gain+dir<1)?1:((gain+dir>10)?10:gain+dir);break;
 case 1: speed=(speed+3+dir)%3;break;
 case 2: overlay=!overlay;break;
 case 3: autoplay=(autoplay+4+dir)%4;break;
 case 4: scanlines=!scanlines;break;
 case 5: perf=(perf+3+dir)%3;break;
 case 6:if(dir)defaults();break;
 }save_settings();
}
static void input_menu(unsigned pressed){
 if(pressed&SCE_CTRL_TRIANGLE){palette=(palette+1)%12;save_settings();}
 if(page==MAIN){
  if(pressed&SCE_CTRL_UP)sel[MAIN]=(sel[MAIN]+6)%7;
  if(pressed&SCE_CTRL_DOWN)sel[MAIN]=(sel[MAIN]+1)%7;
  if(pressed&SCE_CTRL_START)start_play();
  if(pressed&SCE_CTRL_CROSS){switch(sel[MAIN]){
   case 0:start_play();break;
   case 1:page=SOURCES;break;case 2:page=SCENES;sel[SCENES]=scene;break;
   case 3:scan_music();page=BROWSER;break;
   case 4:page=PALETTES;sel[PALETTES]=palette;break;
   case 5:page=SETTINGS;break;case 6:page=ABOUT;break;
  }}
 }else if(page==SOURCES){
  if(pressed&SCE_CTRL_UP)sel[SOURCES]=(sel[SOURCES]+2)%3;
  if(pressed&SCE_CTRL_DOWN)sel[SOURCES]=(sel[SOURCES]+1)%3;
  if(pressed&SCE_CTRL_CROSS){source=(Source)sel[SOURCES];save_settings();page=MAIN;}
  if(pressed&SCE_CTRL_CIRCLE)page=MAIN;
 }else if(page==SCENES){
  if(pressed&SCE_CTRL_UP)sel[SCENES]=(sel[SCENES]+HERO_SCENES-1)%HERO_SCENES;
  if(pressed&SCE_CTRL_DOWN)sel[SCENES]=(sel[SCENES]+1)%HERO_SCENES;
  if(pressed&SCE_CTRL_CROSS){scene=sel[SCENES];save_settings();page=MAIN;}
  if(pressed&SCE_CTRL_START){scene=sel[SCENES];start_play();}
  if(pressed&SCE_CTRL_CIRCLE)page=MAIN;
 }else if(page==BROWSER){
  if((pressed&SCE_CTRL_UP)&&track_sel>0)track_sel--;
  if((pressed&SCE_CTRL_DOWN)&&track_sel<track_count-1)track_sel++;
  if((pressed&SCE_CTRL_CROSS)&&track_count){track_active=track_sel;source=FILE_SOURCE;start_play();}
  if(pressed&SCE_CTRL_CIRCLE)page=MAIN;
 }else if(page==PALETTES){
  if(pressed&SCE_CTRL_UP)sel[PALETTES]=(sel[PALETTES]+8)%12;
  if(pressed&SCE_CTRL_DOWN)sel[PALETTES]=(sel[PALETTES]+4)%12;
  if(pressed&SCE_CTRL_LEFT)sel[PALETTES]=(sel[PALETTES]+11)%12;
  if(pressed&SCE_CTRL_RIGHT)sel[PALETTES]=(sel[PALETTES]+1)%12;
  if(pressed&SCE_CTRL_CROSS){palette=sel[PALETTES];save_settings();}
  if(pressed&SCE_CTRL_CIRCLE)page=MAIN;
 }else if(page==SETTINGS){
  if(pressed&SCE_CTRL_UP)sel[SETTINGS]=(sel[SETTINGS]+6)%7;
  if(pressed&SCE_CTRL_DOWN)sel[SETTINGS]=(sel[SETTINGS]+1)%7;
  if(pressed&SCE_CTRL_LEFT)change_option(-1);
  if(pressed&SCE_CTRL_RIGHT)change_option(1);
  if(pressed&SCE_CTRL_CROSS){if(sel[SETTINGS]==6){defaults();save_settings();}else change_option(1);}
  if(pressed&SCE_CTRL_CIRCLE){save_settings();page=MAIN;}
 }else if(page==ABOUT){if(pressed&SCE_CTRL_CIRCLE)page=MAIN;}
}
static void input_play(unsigned pressed){
 if(pressed&SCE_CTRL_CIRCLE){leave_play();return;}
 if(pressed&(SCE_CTRL_RIGHT|SCE_CTRL_RTRIGGER))change_scene(scene+1);
 if(pressed&(SCE_CTRL_LEFT|SCE_CTRL_LTRIGGER))change_scene(scene-1);
 if(pressed&SCE_CTRL_UP){if(gain<10)gain++;save_settings();}
 if(pressed&SCE_CTRL_DOWN){if(gain>1)gain--;save_settings();}
 if(pressed&SCE_CTRL_TRIANGLE){palette=(palette+1)%12;save_settings();}
 if(pressed&SCE_CTRL_SQUARE){overlay_pin=!overlay_pin;overlay=1;}
 if(pressed&SCE_CTRL_CROSS){paused=!paused;if(source==FILE_SOURCE)music_set_paused(paused);}
 if(pressed&SCE_CTRL_SELECT){autoplay=autoplay?0:2;save_settings();}
 if(pressed&SCE_CTRL_START){overlay_pin=!overlay_pin;overlay=1;}
 if(pressed)ui_age=0;
}
int main(void){
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
 vita2d_init();vita2d_set_vblank_wait(1);vita2d_set_clear_color(C(0,0,0));font=vita2d_load_default_pgf();
 load_settings();scan_music();if(source==FILE_SOURCE&&track_count==0)source=LOOP;
 unsigned prev=0;SceUInt64 previous_us=sceKernelGetProcessTimeWide();
 while(1){
  SceUInt64 now=sceKernelGetProcessTimeWide();float dt=(now-previous_us)/1000000.f;previous_us=now;
  if(dt<0||dt>.15f)dt=.0166667f;
  SceCtrlData pad;memset(&pad,0,sizeof(pad));sceCtrlPeekBufferPositive(0,&pad,1);
  unsigned pressed=pad.buttons & ~prev;prev=pad.buttons;
  if(page==PLAYING){
   input_play(pressed);
   if(page==PLAYING){
    if(!paused){
     if(source==MIC && micport>=0&&!mic_error){
      if(sceAudioInInput(micport,mic_chunk)<0)mic_error=1;
      else audio_push(mic_chunk,256,16000);
     }else if(source==FILE_SOURCE&&music_error==0){
      music_get_wave(samples,2048);audio_push(samples,2048,48000);
     }
     if(source==LOOP){
      float t=scene_clock;
      sound.bass=.17f+.18f*(.5f+.5f*sinf(t*8));
      sound.mids=.1f+.14f*(.5f+.5f*sinf(t*3.5f));
      sound.highs=.06f+.07f*(.5f+.5f*sinf(t*9.2f));
      sound.rms=.3f;sound.transient=.12f*(.5f+.5f*sinf(t*5.5f));
     }else sound=audio_analyze(gain*.20f+.55f);
     scene_clock+=dt*speeds[speed];scene_age+=dt;
     if(autoplay&&scene_age>=autoplay_sec[autoplay])change_scene(scene+1);
    }
    ui_age+=dt;
   }
  }else input_menu(pressed);
  vita2d_start_drawing();vita2d_clear_screen();
  if(page==PLAYING)draw_playback();
  else{backdrop();switch(page){case MAIN:draw_main();break;case SOURCES:draw_sources();break;case SCENES:draw_scenes();break;case BROWSER:draw_browser();break;case PALETTES:draw_palettes();break;case SETTINGS:draw_settings();break;case ABOUT:draw_about();break;default:break;}}
  vita2d_end_drawing();vita2d_swap_buffers();
  if(page==MAIN&&(pressed&SCE_CTRL_CIRCLE))break;
 }
 stop_audio();scene_shutdown();save_settings();if(font)vita2d_free_pgf(font);vita2d_fini();return 0;
}
