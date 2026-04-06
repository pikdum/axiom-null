#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef enum AudioSfx
{
    AUDIO_SFX_MENU_MOVE = 0,
    AUDIO_SFX_MENU_SELECT,
    AUDIO_SFX_PLAYER_SHOT,
    AUDIO_SFX_PLAYER_MISSILE,
    AUDIO_SFX_BOMB,
    AUDIO_SFX_ENEMY_DESTROY,
    AUDIO_SFX_PLAYER_DEATH,
    AUDIO_SFX_BOSS_ALERT,
    AUDIO_SFX_STAGE_CLEAR,
    AUDIO_SFX_COUNT
} AudioSfx;

bool AudioInit(void);
void AudioShutdown(void);
void AudioPlaySfx(AudioSfx sfx);

#endif
