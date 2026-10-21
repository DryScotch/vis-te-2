#include "scene_renderer.h"
#include <psp2/gxm.h>
#include <vita2d.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define SCALE 3.0f
#define PI 3.14159265358979323846f
#define CLIPS 3
/* All clips keep at most 2 decoded textures. No 96-frame startup allocation. */
typedef struct {const char *pattern;int count;float fps;vita2d_texture *tex[2];int idx[2];int next;} Clip;
static Clip clip[CLIPS];
static vita2d_texture *base=NULL;
static int current=-1;
static const char *names[]={"BLUE HALO","DUAL SUB REACTOR","RIBBON WAVE","LIQUID PIXEL","PULSE HEART"};
static const char *descs[]={"Orbiting reference footage","Angled two-sub enclosure","Three-dimensional ribbon layers","Animated organic pixel fields","Reference anatomical pulse"};
const char *scene_title(int i){return i>=0&&i<HERO_SCENES?names[i]:"UNKNOWN";}
const char *scene_description(int i){return i>=0&&i<HERO_SCENES?descs[i]:"";}
static void reset_clip(Clip *c){for(int j=0;j<2;j++){if(c->tex[j])vita2d_free_texture(c->tex[j]);c->tex[j]=NULL;c->idx[j]=-1;}c->next=0;}
static void configure(Clip *c,const char *pat,int count,float fps){reset_clip(c);c->pattern=pat;c->count=count;c->fps=fps;}
static vita2d_texture *load(const char *path){vita2d_texture *t=vita2d_load_PNG_file(path);if(t)vita2d_texture_set_filters(t,SCE_GXM_TEXTURE_FILTER_POINT,SCE_GXM_TEXTURE_FILTER_POINT);return t;}
static vita2d_texture *get_clip_sheet(Clip *c,int idx){
 if(!c->pattern||c->count<=0)return NULL;
 idx=((idx%c->count)+c->count)%c->count;
 idx/=8; /* 8 frames per GPU atlas sheet; far fewer blocking PNG decodes */
 for(int k=0;k<2;k++)if(c->tex[k]&&c->idx[k]==idx)return c->tex[k];
 char path[160];snprintf(path,sizeof(path),c->pattern,idx);
 int slot=c->next;c->next=1-slot;
 if(c->tex[slot])vita2d_free_texture(c->tex[slot]);
 c->tex[slot]=load(path);c->idx[slot]=idx;
 return c->tex[slot];
}
static void draw(vita2d_texture *t){if(t)vita2d_draw_texture_scale(t,0,2,SCALE,SCALE);}
static void draw_clip(Clip *c,int frame){
 if(c->count<=0)return;frame=((frame%c->count)+c->count)%c->count;
 vita2d_texture *t=get_clip_sheet(c,frame);
 if(t)vita2d_draw_texture_part_scale(t,0,2,(frame%4)*320,(frame%8/4)*180,320,180,SCALE,SCALE);
}
void scene_activate(int id){
 if(id==current)return;
 for(int i=0;i<CLIPS;i++)reset_clip(&clip[i]);
 if(base)vita2d_free_texture(base);base=NULL;
 current=id;
 switch(id){
 case 0: configure(&clip[0],"app0:assets/scenes/blue_halo/base/sheet_%03d.png",96,15);break;
 case 1:
  base=load("app0:assets/scenes/dual_sub/cabinet.png");
  configure(&clip[0],"app0:assets/scenes/dual_sub/near/sheet_%03d.png",32,24);
  configure(&clip[1],"app0:assets/scenes/dual_sub/far/sheet_%03d.png",32,24);
  configure(&clip[2],"app0:assets/scenes/dual_sub/glow/sheet_%03d.png",32,24);break;
 case 2:
  base=load("app0:assets/scenes/ribbon_wave/background.png");
  configure(&clip[0],"app0:assets/scenes/ribbon_wave/back/sheet_%03d.png",64,24);
  configure(&clip[1],"app0:assets/scenes/ribbon_wave/front/sheet_%03d.png",64,24);break;
 case 3:
  configure(&clip[0],"app0:assets/scenes/liquid_pixel/field_back/sheet_%03d.png",64,24);
  configure(&clip[1],"app0:assets/scenes/liquid_pixel/field_front/sheet_%03d.png",64,24);break;
 case 4:
  configure(&clip[0],"app0:assets/scenes/pulse_heart/body/sheet_%03d.png",48,12);
  configure(&clip[1],"app0:assets/scenes/pulse_heart/aura/sheet_%03d.png",48,12);break;
 }
}
void scene_shutdown(void){for(int i=0;i<CLIPS;i++)reset_clip(&clip[i]);if(base)vita2d_free_texture(base);base=NULL;current=-1;}
void scene_draw(int id,float seconds,AudioLevels a,uint32_t accent){
 scene_activate(id);
 int f0=(int)floorf(seconds*clip[0].fps);
 int f1=(int)floorf(seconds*clip[1].fps);
 float bass=a.bass;
 if(id==0){draw_clip(&clip[0],f0);
  for(int i=0;i<9;i++){
   float x=480+170*sinf(seconds*.5f+i*4.1f),y=272+130*cosf(seconds*.57f+i*3.17f);
   if(a.highs>.2f)vita2d_draw_fill_circle(x,y,1.5f+2*a.highs,accent);
  }
 }else if(id==1){
  draw(base);
  float pulse=sinf(seconds*(11+9*bass));
  int near=(int)(15.5f+15.4f*bass*pulse);
  int far=(int)(15.5f+13.8f*bass*sinf(seconds*(11+9*bass)-.55f));
  draw_clip(&clip[0],near);draw_clip(&clip[1],far);draw_clip(&clip[2],(int)(seconds*24));
 }else if(id==2){
  draw(base);draw_clip(&clip[0],f0);draw_clip(&clip[1],f1);
  if(a.mids>.12f)for(int i=0;i<7;i++){
   float x=75+i*124.0f;float y=268+72*sinf(seconds*1.7f+i*.8f);
   vita2d_draw_fill_circle(x,y,1.0f+a.highs*3.0f,accent);
  }
 }else if(id==3){
  draw_clip(&clip[0],f0);draw_clip(&clip[1],f1+13);
  for(int i=0;i<16;i++)if(a.highs>.08f){
   float x=fmodf((i*89+seconds*41),940)+10,y=15+fmodf((i*51+seconds*19),510);
   vita2d_draw_rectangle(x,y,3,3,accent);
  }
 }else if(id==4){
  draw_clip(&clip[0],f0);draw_clip(&clip[1],f1);
  if(a.transient>.05f)vita2d_draw_fill_circle(480,263,4+a.transient*9,accent);
 }
}
