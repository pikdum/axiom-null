#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

typedef struct AudioState
{
    bool initialized;
    Sound sounds[AUDIO_SFX_COUNT];
    bool sounds_loaded[AUDIO_SFX_COUNT];
    Music music[AUDIO_MUSIC_COUNT];
    bool music_loaded[AUDIO_MUSIC_COUNT];
    Music laser_hum;
    bool laser_hum_loaded;
    AudioMusic current_music;
    bool music_playing;
    bool laser_hum_active;
    bool laser_hum_playing;
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
    case AUDIO_SFX_LASER_CHARGE:
        return "laser_charge.wav";
    case AUDIO_SFX_LASER_FIRE:
        return "laser_fire.wav";
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
    case AUDIO_SFX_LASER_CHARGE:
        return 0.42f;
    case AUDIO_SFX_LASER_FIRE:
        return 0.5f;
    case AUDIO_SFX_COUNT:
        break;
    }

    return 0.5f;
}

static const char *AudioMusicFile(AudioMusic music)
{
    switch (music)
    {
    case AUDIO_MUSIC_TITLE:
        return "title.ogg";
    case AUDIO_MUSIC_STAGE:
        return "stage.ogg";
    case AUDIO_MUSIC_BOSS:
        return "boss.ogg";
    case AUDIO_MUSIC_COUNT:
        break;
    }

    return "";
}

static float AudioMusicVolume(AudioMusic music)
{
    switch (music)
    {
    case AUDIO_MUSIC_TITLE:
        return 0.55f;
    case AUDIO_MUSIC_STAGE:
        return 0.65f;
    case AUDIO_MUSIC_BOSS:
        return 0.68f;
    case AUDIO_MUSIC_COUNT:
        break;
    }

    return 0.6f;
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

static const char *AudioLaserHumFile(void) { return "laser_hum.wav"; }

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
        g_audio.sounds_loaded[i] = true;
        SetSoundVolume(g_audio.sounds[i], AudioSfxVolume((AudioSfx)i));
    }

    for (int i = 0; i < AUDIO_MUSIC_COUNT; ++i)
    {
        const char *path = TextFormat("%s/music/%s", asset_root, AudioMusicFile((AudioMusic)i));

        if (!AudioFileExists(path))
        {
            TraceLog(LOG_WARNING, "AUDIO: missing music file: %s", path);
            continue;
        }

        g_audio.music[i] = LoadMusicStream(path);
        g_audio.music_loaded[i] = true;
        SetMusicVolume(g_audio.music[i], AudioMusicVolume((AudioMusic)i));
    }

    {
        const char *path = TextFormat("%s/sfx/%s", asset_root, AudioLaserHumFile());

        if (!AudioFileExists(path))
        {
            TraceLog(LOG_WARNING, "AUDIO: missing laser hum file: %s", path);
        }
        else
        {
            g_audio.laser_hum = LoadMusicStream(path);
            g_audio.laser_hum_loaded = true;
            SetMusicVolume(g_audio.laser_hum, 0.26f);
        }
    }

    SetMasterVolume(0.85f);
    g_audio.current_music = AUDIO_MUSIC_COUNT;
    g_audio.music_playing = false;
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
        if (g_audio.sounds_loaded[i])
        {
            UnloadSound(g_audio.sounds[i]);
        }
    }

    for (int i = 0; i < AUDIO_MUSIC_COUNT; ++i)
    {
        if (g_audio.music_loaded[i])
        {
            UnloadMusicStream(g_audio.music[i]);
        }
    }

    if (g_audio.laser_hum_loaded)
    {
        UnloadMusicStream(g_audio.laser_hum);
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

    if (g_audio.sounds_loaded[sfx])
    {
        PlaySound(g_audio.sounds[sfx]);
    }
}

void AudioSetLaserHumActive(bool active)
{
    if (!g_audio.initialized)
    {
        return;
    }

    g_audio.laser_hum_active = active;
}

void AudioUpdate(AudioMusic music)
{
    if (!g_audio.initialized || music < 0 || music >= AUDIO_MUSIC_COUNT)
    {
        return;
    }

    if (!g_audio.music_loaded[music])
    {
        return;
    }

    if (!g_audio.music_playing || g_audio.current_music != music)
    {
        if (g_audio.music_playing && g_audio.current_music < AUDIO_MUSIC_COUNT &&
            g_audio.music_loaded[g_audio.current_music])
        {
            StopMusicStream(g_audio.music[g_audio.current_music]);
        }

        g_audio.current_music = music;
        g_audio.music_playing = true;
        PlayMusicStream(g_audio.music[g_audio.current_music]);
    }

    UpdateMusicStream(g_audio.music[g_audio.current_music]);

    if (g_audio.laser_hum_loaded)
    {
        if (g_audio.laser_hum_active)
        {
            if (!g_audio.laser_hum_playing)
            {
                PlayMusicStream(g_audio.laser_hum);
                g_audio.laser_hum_playing = true;
            }

            UpdateMusicStream(g_audio.laser_hum);
        }
        else if (g_audio.laser_hum_playing)
        {
            StopMusicStream(g_audio.laser_hum);
            g_audio.laser_hum_playing = false;
        }
    }
}
