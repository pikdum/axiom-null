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
#define MAX_BEAMS 64
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
    ENEMY_SENTINEL,
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

typedef enum EnemyMovementModel
{
    ENEMY_MOVE_SWEEP_BOUNCE = 0,
    ENEMY_MOVE_AIMER_DRIFT,
    ENEMY_MOVE_ORBIT_SINE,
    ENEMY_MOVE_SENTINEL_HOLD,
    ENEMY_MOVE_BOSS_CORE
} EnemyMovementModel;

typedef enum AttackPatternId
{
    ATTACK_PATTERN_NONE = 0,
    ATTACK_PATTERN_SWEEP_FAN_RING,
    ATTACK_PATTERN_AIMER_FAN_AIMED,
    ATTACK_PATTERN_ORBITER_RING_WALL,
    ATTACK_PATTERN_SENTINEL_LASER,
    ATTACK_PATTERN_BOSS_PHASE_ONE,
    ATTACK_PATTERN_BOSS_PHASE_TWO,
    ATTACK_PATTERN_BOSS_PHASE_THREE,
    ATTACK_PATTERN_ARRAY_BOSS_PHASE_ONE,
    ATTACK_PATTERN_ARRAY_BOSS_PHASE_TWO,
    ATTACK_PATTERN_ARRAY_BOSS_PHASE_THREE
} AttackPatternId;

struct StageDef;

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
    EnemyMovementModel movement;
    Vector2 position;
    Vector2 velocity;
    float radius;
    int hp;
    int max_hp;
    int phase_thresholds[2];
    float cooldown;
    float aux_cooldown;
    float age;
    float phase_clock;
    float anchor_x;
    float collision_scale;
    float lifetime;
    float attack_start_y;
    float move_param_a;
    float move_param_b;
    float move_param_c;
    unsigned int reward;
    AttackPatternId phase_patterns[3];
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

typedef struct Beam
{
    bool active;
    bool harmful;
    bool play_fire_sfx;
    float x;
    float origin_y;
    float telegraph_width;
    float width;
    float charge_timer;
    float active_timer;
} Beam;

typedef struct Game
{
    GameMode mode;
    Difficulty difficulty;
    const struct StageDef *stage;
    Rectangle playfield;
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Bullet player_bullets[MAX_PLAYER_BULLETS];
    Bullet enemy_bullets[MAX_ENEMY_BULLETS];
    Beam beams[MAX_BEAMS];
    Particle particles[MAX_PARTICLES];
    bool boss_spawned;
    bool boss_defeated;
    bool debug_invulnerable;
    bool debug_infinite_lives;
    bool debug_start_at_boss;
    int debug_stage_override;
    unsigned int score;
    float stage_time;
    float state_time;
    float clear_timer;
    int next_stage_cue;
    float status_timer;
    char status_message[64];
    char cheat_buffer[32];
    int cheat_length;
} Game;

void GameInit(Game *game);
void GameUpdate(Game *game, float dt);
void GameDraw(const Game *game);

#endif
