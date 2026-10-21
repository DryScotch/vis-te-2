#ifndef CABIN_MUSIC_H
#define CABIN_MUSIC_H
#include <stdint.h>
int music_start(const char *path);
void music_stop(void);
void music_set_paused(int paused);
int music_is_active(void);
int music_is_finished(void);
void music_get_wave(int16_t *dest, int count);
#endif
