#include "audio.h"
#include "game.h"

int main(void)
{
    Game game;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "AXIOM NULL");
    SetTargetFPS(120);
    AudioInit();

    GameInit(&game);

    while (!WindowShouldClose())
    {
        GameUpdate(&game, GetFrameTime());
        if (game.mode == GAME_MODE_TITLE)
        {
            AudioUpdateMusic(AUDIO_MUSIC_TITLE);
        }
        else if (game.mode == GAME_MODE_PLAYING && game.boss_spawned && !game.boss_defeated)
        {
            AudioUpdateMusic(AUDIO_MUSIC_BOSS);
        }
        else
        {
            AudioUpdateMusic(AUDIO_MUSIC_STAGE);
        }

        BeginDrawing();
        GameDraw(&game);
        EndDrawing();
    }

    AudioShutdown();
    CloseWindow();
    return 0;
}
