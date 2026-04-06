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
    AUDIO_SFX_LASER_CHARGE,
    AUDIO_SFX_LASER_FIRE,
    AUDIO_SFX_COUNT
} AudioSfx;

typedef enum AudioMusic
{
    AUDIO_MUSIC_TITLE = 0,
    AUDIO_MUSIC_STAGE,
    AUDIO_MUSIC_BOSS,
    AUDIO_MUSIC_COUNT
} AudioMusic;

bool AudioInit(void);
void AudioShutdown(void);
void AudioPlaySfx(AudioSfx sfx);
void AudioUpdateMusic(AudioMusic music);

#endif
