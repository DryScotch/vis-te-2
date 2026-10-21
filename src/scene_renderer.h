#ifndef VML_SCENE_RENDERER_H
#define VML_SCENE_RENDERER_H
#include <stdint.h>
#include "audio_analysis.h"
#define HERO_SCENES 5
const char *scene_title(int i);
const char *scene_description(int i);
void scene_activate(int id);
void scene_draw(int id,float seconds,AudioLevels a,uint32_t accent);
void scene_shutdown(void);
#endif
