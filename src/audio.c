#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

typedef struct AudioState
{
    bool initialized;
    Sound sounds[AUDIO_SFX_COUNT];
} AudioState;

static AudioState g_audio = {0};

static const char *AudioSfxFile(AudioSfx sfx)
{
    switch (sfx)
    {
    case AUDIO_SFX_MENU_MOVE:
        return "menu_move.wav";
    case AUDIO_SFX_MENU_SELECT:
        return "menu_select.wav";
    case AUDIO_SFX_PLAYER_SHOT:
        return "player_shot.wav";
    case AUDIO_SFX_PLAYER_MISSILE:
        return "player_missile.wav";
    case AUDIO_SFX_BOMB:
        return "bomb.wav";
    case AUDIO_SFX_ENEMY_DESTROY:
        return "enemy_destroy.wav";
    case AUDIO_SFX_PLAYER_DEATH:
        return "player_death.wav";
    case AUDIO_SFX_BOSS_ALERT:
        return "boss_alert.wav";
    case AUDIO_SFX_STAGE_CLEAR:
        return "stage_clear.wav";
    case AUDIO_SFX_COUNT:
        break;
    }

    return "";
}

static float AudioSfxVolume(AudioSfx sfx)
{
    switch (sfx)
    {
    case AUDIO_SFX_MENU_MOVE:
        return 0.45f;
    case AUDIO_SFX_MENU_SELECT:
        return 0.55f;
    case AUDIO_SFX_PLAYER_SHOT:
        return 0.18f;
    case AUDIO_SFX_PLAYER_MISSILE:
        return 0.35f;
    case AUDIO_SFX_BOMB:
        return 0.6f;
    case AUDIO_SFX_ENEMY_DESTROY:
        return 0.3f;
    case AUDIO_SFX_PLAYER_DEATH:
        return 0.6f;
    case AUDIO_SFX_BOSS_ALERT:
        return 0.5f;
    case AUDIO_SFX_STAGE_CLEAR:
        return 0.55f;
    case AUDIO_SFX_COUNT:
        break;
    }

    return 0.5f;
}

static bool AudioFileExists(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return false;
    }

    fclose(file);
    return true;
}

static const char *ResolveAudioAssetRoot(void)
{
    static char asset_root[1024];
    const char *env_root = getenv("AXIOM_NULL_ASSETS");

    if (env_root != NULL && env_root[0] != '\0')
    {
        snprintf(asset_root, sizeof(asset_root), "%s/audio", env_root);
        return asset_root;
    }

    snprintf(asset_root, sizeof(asset_root), "%s/assets/audio", GetWorkingDirectory());
    if (AudioFileExists(TextFormat("%s/sfx/menu_move.wav", asset_root)))
    {
        return asset_root;
    }

    snprintf(asset_root, sizeof(asset_root), "%s../share/axiom-null/assets/audio",
             GetApplicationDirectory());
    return asset_root;
}

bool AudioInit(void)
{
    const char *asset_root;

    if (g_audio.initialized)
    {
        return true;
    }

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        return false;
    }

    asset_root = ResolveAudioAssetRoot();

    for (int i = 0; i < AUDIO_SFX_COUNT; ++i)
    {
        const char *path = TextFormat("%s/sfx/%s", asset_root, AudioSfxFile((AudioSfx)i));

        if (!AudioFileExists(path))
        {
            TraceLog(LOG_WARNING, "AUDIO: missing SFX file: %s", path);
            continue;
        }

        g_audio.sounds[i] = LoadSound(path);
        SetSoundVolume(g_audio.sounds[i], AudioSfxVolume((AudioSfx)i));
    }

    SetMasterVolume(0.85f);
    g_audio.initialized = true;
    return true;
}

void AudioShutdown(void)
{
    if (!g_audio.initialized)
    {
        return;
    }

    for (int i = 0; i < AUDIO_SFX_COUNT; ++i)
    {
        if (IsSoundValid(g_audio.sounds[i]))
        {
            UnloadSound(g_audio.sounds[i]);
        }
    }

    CloseAudioDevice();
    memset(&g_audio, 0, sizeof(g_audio));
}

void AudioPlaySfx(AudioSfx sfx)
{
    if (!g_audio.initialized || sfx < 0 || sfx >= AUDIO_SFX_COUNT)
    {
        return;
    }

    if (IsSoundValid(g_audio.sounds[sfx]))
    {
        PlaySound(g_audio.sounds[sfx]);
    }
}
