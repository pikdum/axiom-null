#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include "raylib.h"

#define SCREEN_WIDTH 1100
#define SCREEN_HEIGHT 900
#define PLAYFIELD_WIDTH 560
#define PLAYFIELD_HEIGHT 820
#define PLAYFIELD_X 270
#define PLAYFIELD_Y 40

#define MAX_ENEMIES 64
#define MAX_PLAYER_BULLETS 256
#define MAX_ENEMY_BULLETS 1536
#define MAX_PARTICLES 512

typedef enum GameMode
{
    GAME_MODE_TITLE = 0,
    GAME_MODE_PLAYING,
    GAME_MODE_GAME_OVER,
    GAME_MODE_CLEAR
} GameMode;

typedef enum EnemyKind
{
    ENEMY_SWEEP = 0,
    ENEMY_AIMER,
    ENEMY_ORBITER,
    ENEMY_BOSS
} EnemyKind;

typedef enum BulletKind
{
    BULLET_PLAYER = 0,
    BULLET_MISSILE,
    BULLET_RING,
    BULLET_AIMED,
    BULLET_WALL,
    BULLET_SPIRAL
} BulletKind;

typedef enum Difficulty
{
    DIFFICULTY_CASUAL = 0,
    DIFFICULTY_STANDARD,
    DIFFICULTY_EXPERT
} Difficulty;

typedef struct Player
{
    Vector2 position;
    float shot_timer;
    float missile_timer;
    float respawn_timer;
    float invulnerable_timer;
    float bomb_cooldown;
    int lives;
    int bombs;
    bool alive;
} Player;

typedef struct Enemy
{
    bool active;
    EnemyKind kind;
    Vector2 position;
    Vector2 velocity;
    float radius;
    int hp;
    int max_hp;
    float cooldown;
    float aux_cooldown;
    float age;
    float phase_clock;
    float anchor_x;
} Enemy;

typedef struct Bullet
{
    bool active;
    BulletKind kind;
    Vector2 position;
    Vector2 velocity;
    float radius;
    float age;
    bool grazed;
} Bullet;

typedef struct Particle
{
    bool active;
    Vector2 position;
    Vector2 velocity;
    float radius;
    float age;
    float lifetime;
    Color color;
} Particle;

typedef struct Game
{
    GameMode mode;
    Difficulty difficulty;
    Rectangle playfield;
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Bullet player_bullets[MAX_PLAYER_BULLETS];
    Bullet enemy_bullets[MAX_ENEMY_BULLETS];
    Particle particles[MAX_PARTICLES];
    bool script_flags[16];
    bool boss_spawned;
    bool boss_defeated;
    bool debug_invulnerable;
    bool debug_infinite_lives;
    bool debug_start_at_boss;
    unsigned int score;
    float stage_time;
    float state_time;
    float clear_timer;
    float status_timer;
    char status_message[64];
    char cheat_buffer[32];
    int cheat_length;
} Game;

void GameInit(Game *game);
void GameUpdate(Game *game, float dt);
void GameDraw(const Game *game);

#endif
