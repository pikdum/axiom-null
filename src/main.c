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

        BeginDrawing();
        GameDraw(&game);
        EndDrawing();
    }

    AudioShutdown();
    CloseWindow();
    return 0;
}
