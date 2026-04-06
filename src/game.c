#include "audio.h"
#include "game.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "logic.h"
#include "stage.h"

static const float TAU = 6.28318530717958647692f;
static const float PLAYER_RADIUS = 10.0f;
static const float PLAYER_HITBOX_RADIUS = 3.5f;
static const float PLAYER_GRAZE_RADIUS = 22.0f;

static const Color COLOR_BG = {10, 12, 18, 255};
static const Color COLOR_PANEL = {17, 20, 29, 255};
static const Color COLOR_PANEL_LINE = {50, 60, 92, 255};
static const Color COLOR_PLAYFIELD = {6, 8, 13, 255};
static const Color COLOR_GRID = {28, 35, 52, 255};
static const Color COLOR_PLAYER = {236, 242, 248, 255};
static const Color COLOR_PLAYER_ACCENT = {80, 224, 255, 255};
static const Color COLOR_MISSILE = {255, 184, 96, 255};
static const Color COLOR_RING = {255, 96, 132, 255};
static const Color COLOR_AIMED = {97, 244, 226, 255};
static const Color COLOR_WALL = {255, 228, 153, 255};
static const Color COLOR_SPIRAL = {187, 120, 255, 255};
static const Color COLOR_SWEEP = {255, 122, 122, 255};
static const Color COLOR_AIMER = {105, 232, 215, 255};
static const Color COLOR_ORBIT = {250, 227, 143, 255};
static const Color COLOR_SENTINEL = {255, 168, 110, 255};
static const Color COLOR_BOSS = {232, 237, 244, 255};
static const Color COLOR_LASER_LIVE = {255, 72, 72, 255};

static const char *DifficultyLabel(Difficulty difficulty)
{
    switch (difficulty)
    {
    case DIFFICULTY_CASUAL:
        return "CASUAL";
    case DIFFICULTY_STANDARD:
        return "STANDARD";
    case DIFFICULTY_EXPERT:
        return "EXPERT";
    }

    return "UNKNOWN";
}

static Color DifficultyColor(Difficulty difficulty)
{
    switch (difficulty)
    {
    case DIFFICULTY_CASUAL:
        return COLOR_AIMED;
    case DIFFICULTY_STANDARD:
        return COLOR_PLAYER;
    case DIFFICULTY_EXPERT:
        return COLOR_RING;
    }

    return COLOR_PLAYER;
}

static float DifficultyBulletSpeedScale(const Game *game)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return 0.84f;
    case DIFFICULTY_STANDARD:
        return 1.0f;
    case DIFFICULTY_EXPERT:
        return 1.16f;
    }

    return 1.0f;
}

static float DifficultyCooldownScale(const Game *game)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return 1.18f;
    case DIFFICULTY_STANDARD:
        return 1.0f;
    case DIFFICULTY_EXPERT:
        return 0.88f;
    }

    return 1.0f;
}

static int DifficultyPatternCount(const Game *game, int base_count)
{
    int adjusted = base_count;

    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        adjusted = base_count > 3 ? base_count - 2 : base_count - 1;
        break;
    case DIFFICULTY_STANDARD:
        adjusted = base_count;
        break;
    case DIFFICULTY_EXPERT:
        adjusted = base_count + 2;
        break;
    }

    if (adjusted < 1)
    {
        adjusted = 1;
    }

    return adjusted;
}

static int DifficultyBossHp(const Game *game, int base_hp)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return (base_hp * 3) / 4;
    case DIFFICULTY_STANDARD:
        return base_hp;
    case DIFFICULTY_EXPERT:
        return (base_hp * 5) / 4;
    }

    return base_hp;
}

static int DifficultyStartingLives(const Game *game)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return 4;
    case DIFFICULTY_STANDARD:
        return 3;
    case DIFFICULTY_EXPERT:
        return 2;
    }

    return 3;
}

static int DifficultyStartingBombs(const Game *game)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return 3;
    case DIFFICULTY_STANDARD:
        return 2;
    case DIFFICULTY_EXPERT:
        return 1;
    }

    return 2;
}

static float DifficultyMissileInterval(const Game *game)
{
    switch (game->difficulty)
    {
    case DIFFICULTY_CASUAL:
        return 1.2f;
    case DIFFICULTY_STANDARD:
        return 1.45f;
    case DIFFICULTY_EXPERT:
        return 1.7f;
    }

    return 1.45f;
}

static void SetStatusMessage(Game *game, const char *message)
{
    strncpy(game->status_message, message, sizeof(game->status_message) - 1);
    game->status_message[sizeof(game->status_message) - 1] = '\0';
    game->status_timer = 2.5f;
}

static float ScaleEnemyCooldown(const Game *game, float base_cooldown);
static void DamagePlayer(Game *game);
static void DestroyEnemy(Game *game, Enemy *enemy);

static const char *StageHudLabel(const Game *game)
{
    if (game->stage != NULL && game->stage->hud_label != NULL)
    {
        return game->stage->hud_label;
    }

    return "ONE";
}

static const char *DebugStageOverrideLabel(const Game *game)
{
    const StageDef *stage = StageGetById((StageId)game->debug_stage_override);

    if (stage != NULL && stage->hud_label != NULL)
    {
        return stage->hud_label;
    }

    return "ONE";
}

static LogicVec2 ToLogic(Vector2 value)
{
    return (LogicVec2){
        .x = value.x,
        .y = value.y,
    };
}

static Vector2 ToVector(LogicVec2 value)
{
    return (Vector2){
        .x = value.x,
        .y = value.y,
    };
}

static float RandomUnit(void) { return (float)GetRandomValue(-1000, 1000) / 1000.0f; }

static float ClampToPlayfieldX(float value)
{
    return LogicClamp(value, (float)PLAYFIELD_X + 18.0f,
                      (float)(PLAYFIELD_X + PLAYFIELD_WIDTH) - 18.0f);
}

static float ClampToPlayfieldY(float value)
{
    return LogicClamp(value, (float)PLAYFIELD_Y + 18.0f,
                      (float)(PLAYFIELD_Y + PLAYFIELD_HEIGHT) - 18.0f);
}

static bool IsInsideExtendedPlayfield(Vector2 position, float padding)
{
    return position.x >= (float)PLAYFIELD_X - padding &&
           position.x <= (float)(PLAYFIELD_X + PLAYFIELD_WIDTH) + padding &&
           position.y >= (float)PLAYFIELD_Y - padding &&
           position.y <= (float)(PLAYFIELD_Y + PLAYFIELD_HEIGHT) + padding;
}

static void SpawnParticle(Game *game, Vector2 position, Vector2 velocity, float radius,
                          float lifetime, Color color)
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle *particle = &game->particles[i];

        if (!particle->active)
        {
            particle->active = true;
            particle->position = position;
            particle->velocity = velocity;
            particle->radius = radius;
            particle->age = 0.0f;
            particle->lifetime = lifetime;
            particle->color = color;
            return;
        }
    }
}

static void SpawnBurst(Game *game, Vector2 position, Color color, int count, float speed)
{
    for (int i = 0; i < count; ++i)
    {
        float angle = ((float)i / (float)count) * TAU + RandomUnit() * 0.3f;
        float magnitude = speed * (0.45f + 0.55f * (0.5f + RandomUnit() * 0.5f));
        Vector2 velocity = ToVector(LogicPolar(angle, magnitude));

        SpawnParticle(game, position, velocity, 2.0f + (float)(i % 3),
                      0.28f + 0.18f * (float)(i % 4), color);
    }
}

static Bullet *ReserveBullet(Bullet *bullets, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (!bullets[i].active)
        {
            return &bullets[i];
        }
    }

    return NULL;
}

static Beam *ReserveBeam(Game *game)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!game->beams[i].active)
        {
            return &game->beams[i];
        }
    }

    return NULL;
}

static Enemy *ReserveEnemy(Game *game)
{
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        if (!game->enemies[i].active)
        {
            return &game->enemies[i];
        }
    }

    return NULL;
}

static void SpawnPlayerBullet(Game *game, Vector2 position, Vector2 velocity)
{
    Bullet *bullet = ReserveBullet(game->player_bullets, MAX_PLAYER_BULLETS);

    if (bullet == NULL)
    {
        return;
    }

    bullet->active = true;
    bullet->kind = BULLET_PLAYER;
    bullet->position = position;
    bullet->velocity = velocity;
    bullet->radius = 4.0f;
    bullet->age = 0.0f;
    bullet->grazed = false;
}

static void SpawnEnemyBullet(Game *game, BulletKind kind, Vector2 position, Vector2 velocity,
                             float radius)
{
    Bullet *bullet = ReserveBullet(game->enemy_bullets, MAX_ENEMY_BULLETS);

    if (bullet == NULL)
    {
        return;
    }

    bullet->active = true;
    bullet->kind = kind;
    bullet->position = position;
    bullet->velocity = velocity;
    bullet->radius = radius;
    bullet->age = 0.0f;
    bullet->grazed = false;
}

static Enemy *SpawnEnemy(Game *game, EnemyKind kind, Vector2 position, Vector2 velocity,
                         float radius, int hp, float cooldown, float aux_cooldown)
{
    Enemy *enemy = ReserveEnemy(game);

    if (enemy == NULL)
    {
        return NULL;
    }

    *enemy = (Enemy){0};
    enemy->active = true;
    enemy->kind = kind;
    enemy->position = position;
    enemy->velocity = velocity;
    enemy->radius = radius;
    enemy->hp = hp;
    enemy->max_hp = hp;
    enemy->cooldown = ScaleEnemyCooldown(game, cooldown);
    enemy->aux_cooldown = ScaleEnemyCooldown(game, aux_cooldown);
    enemy->age = 0.0f;
    enemy->phase_clock = 0.0f;
    enemy->anchor_x = position.x;

    return enemy;
}

static Enemy *FindBoss(Game *game)
{
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        if (game->enemies[i].active && game->enemies[i].kind == ENEMY_BOSS)
        {
            return &game->enemies[i];
        }
    }

    return NULL;
}

static const Enemy *FindBossConst(const Game *game)
{
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        if (game->enemies[i].active && game->enemies[i].kind == ENEMY_BOSS)
        {
            return &game->enemies[i];
        }
    }

    return NULL;
}

static void ClearEnemyBullets(Game *game)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        if (game->enemy_bullets[i].active)
        {
            SpawnParticle(game, game->enemy_bullets[i].position, (Vector2){0.0f, -30.0f}, 2.0f,
                          0.15f, ColorAlpha(COLOR_PANEL_LINE, 0.6f));
        }

        game->enemy_bullets[i].active = false;
    }

    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        game->beams[i].active = false;
    }
}

static void JumpToBoss(Game *game);
static void ProcessStageCues(Game *game);

static void AdvanceToStage(Game *game, const StageDef *stage)
{
    if (stage == NULL)
    {
        return;
    }

    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->player_bullets, 0, sizeof(game->player_bullets));
    memset(game->enemy_bullets, 0, sizeof(game->enemy_bullets));
    memset(game->beams, 0, sizeof(game->beams));
    memset(game->particles, 0, sizeof(game->particles));

    game->stage = stage;
    game->stage_time = 0.0f;
    game->next_stage_cue = 0;
    game->boss_spawned = false;
    game->boss_defeated = false;
    game->clear_timer = 0.0f;
    game->player.alive = true;
    game->player.position = (Vector2){
        .x = game->playfield.x + game->playfield.width * 0.5f,
        .y = game->playfield.y + game->playfield.height - 88.0f,
    };
    game->player.shot_timer = 0.0f;
    game->player.missile_timer = 0.35f;
    game->player.respawn_timer = 0.0f;
    game->player.invulnerable_timer = 2.0f;
    game->player.bomb_cooldown = 0.0f;
    SetStatusMessage(game, TextFormat("STAGE %s", StageHudLabel(game)));
}

static void ResetRun(Game *game)
{
    Rectangle playfield = game->playfield;
    Difficulty difficulty = game->difficulty;
    bool debug_invulnerable = game->debug_invulnerable;
    bool debug_infinite_lives = game->debug_infinite_lives;
    bool debug_start_at_boss = game->debug_start_at_boss;
    int debug_stage_override = game->debug_stage_override;
    const StageDef *start_stage = StageGetById((StageId)debug_stage_override);

    if (start_stage == NULL)
    {
        start_stage = StageGetDefault();
    }

    memset(game, 0, sizeof(*game));
    game->playfield = playfield;
    game->stage = start_stage;
    game->difficulty = difficulty;
    game->debug_invulnerable = debug_invulnerable;
    game->debug_infinite_lives = debug_infinite_lives;
    game->debug_start_at_boss = debug_start_at_boss;
    game->debug_stage_override = debug_stage_override;
    game->mode = GAME_MODE_PLAYING;
    game->player.position = (Vector2){
        .x = playfield.x + playfield.width * 0.5f,
        .y = playfield.y + playfield.height - 88.0f,
    };
    game->player.alive = true;
    game->player.invulnerable_timer = 1.75f;
    game->player.lives = DifficultyStartingLives(game);
    game->player.bombs = DifficultyStartingBombs(game);
    game->player.missile_timer = 0.35f;

    if (game->debug_start_at_boss)
    {
        JumpToBoss(game);
    }
}

void GameInit(Game *game)
{
    memset(game, 0, sizeof(*game));
    game->stage = StageGetDefault();
    game->difficulty = DIFFICULTY_CASUAL;
    game->playfield = (Rectangle){
        .x = (float)PLAYFIELD_X,
        .y = (float)PLAYFIELD_Y,
        .width = (float)PLAYFIELD_WIDTH,
        .height = (float)PLAYFIELD_HEIGHT,
    };
    game->mode = GAME_MODE_TITLE;
    game->player.position = (Vector2){
        .x = game->playfield.x + game->playfield.width * 0.5f,
        .y = game->playfield.y + game->playfield.height - 88.0f,
    };
    game->player.alive = true;
    game->player.lives = DifficultyStartingLives(game);
    game->player.bombs = DifficultyStartingBombs(game);
}

static void EmitRing(Game *game, Vector2 position, float speed, int count, float start_angle,
                     BulletKind kind, float radius)
{
    LogicVec2 velocities[32];
    int scaled_count = DifficultyPatternCount(game, count);
    float scaled_speed = speed * DifficultyBulletSpeedScale(game);
    int emitted = LogicBuildRing(velocities, 32, scaled_count, start_angle, scaled_speed);

    for (int i = 0; i < emitted; ++i)
    {
        SpawnEnemyBullet(game, kind, position, ToVector(velocities[i]), radius);
    }
}

static void EmitFan(Game *game, Vector2 position, Vector2 target, float speed, int count,
                    float spread, BulletKind kind, float radius)
{
    LogicVec2 velocities[24];
    int scaled_count = DifficultyPatternCount(game, count);
    float scaled_speed = speed * DifficultyBulletSpeedScale(game);
    int emitted = LogicBuildFanToward(velocities, 24, scaled_count, ToLogic(position),
                                      ToLogic(target), spread, scaled_speed);

    for (int i = 0; i < emitted; ++i)
    {
        SpawnEnemyBullet(game, kind, position, ToVector(velocities[i]), radius);
    }
}

static void EmitCurtain(Game *game, Vector2 origin, float center_x, float width, float speed,
                        int count)
{
    int scaled_count = DifficultyPatternCount(game, count);
    float scaled_speed = speed * DifficultyBulletSpeedScale(game);

    for (int i = 0; i < scaled_count; ++i)
    {
        float t = scaled_count == 1 ? 0.5f : (float)i / (float)(scaled_count - 1);
        float angle_offset = (t - 0.5f) * 0.55f;
        float x = center_x - width * 0.5f + width * t;
        Vector2 position = {x, origin.y};
        Vector2 velocity = ToVector(LogicPolar((float)PI / 2.0f + angle_offset, scaled_speed));
        SpawnEnemyBullet(game, BULLET_WALL, position, velocity, 8.5f);
    }
}

static void EmitSpiralBurst(Game *game, Vector2 position, float speed, int count, float angle_scale,
                            float radius, BulletKind kind, float phase_clock)
{
    for (int i = 0; i < count; ++i)
    {
        float angle = phase_clock * angle_scale + ((float)i * TAU / (float)count);

        SpawnEnemyBullet(game, kind, position, ToVector(LogicPolar(angle, speed)), radius);
        SpawnEnemyBullet(game, kind, position, ToVector(LogicPolar(-angle, speed)), radius);
    }
}

static void StartLaserVolley(Game *game, const Enemy *source, Vector2 origin, int count, float span,
                             float width, float charge_time, float active_time)
{
    bool any_spawned = false;
    int owner_index = source != NULL ? (int)(source - game->enemies) : -1;

    for (int i = 0; i < count; ++i)
    {
        Beam *beam = ReserveBeam(game);
        float t = count == 1 ? 0.5f : (float)i / (float)(count - 1);
        float x = count == 1 ? origin.x : origin.x - span * 0.5f + span * t;

        if (beam == NULL)
        {
            continue;
        }

        beam->active = true;
        beam->harmful = false;
        beam->play_fire_sfx = !any_spawned;
        beam->track_source = source != NULL;
        beam->owner_index = owner_index;
        beam->x = LogicClamp(x, game->playfield.x + 24.0f,
                             game->playfield.x + game->playfield.width - 24.0f);
        beam->x_offset = x - origin.x;
        beam->origin_y = LogicClamp(origin.y, game->playfield.y + 12.0f,
                                    game->playfield.y + game->playfield.height - 12.0f);
        beam->telegraph_width = LogicClamp(width * 0.28f, 2.5f, 5.0f);
        beam->width = width;
        beam->charge_timer = charge_time;
        beam->active_timer = active_time;
        any_spawned = true;
    }

    if (any_spawned)
    {
        AudioPlaySfx(AUDIO_SFX_LASER_CHARGE);
    }
}

static void EmitAttackEmitter(Game *game, Enemy *enemy, const AttackEmitterDef *emitter)
{
    switch (emitter->type)
    {
    case ATTACK_EMITTER_NONE:
        break;
    case ATTACK_EMITTER_FAN:
        EmitFan(game, enemy->position, game->player.position, emitter->speed, emitter->count,
                emitter->spread_or_width, emitter->bullet_kind, emitter->radius);
        break;
    case ATTACK_EMITTER_RING:
        EmitRing(game, enemy->position, emitter->speed, emitter->count,
                 enemy->phase_clock * emitter->angle_scale, emitter->bullet_kind, emitter->radius);
        break;
    case ATTACK_EMITTER_CURTAIN:
        EmitCurtain(game, enemy->position, game->playfield.x + game->playfield.width * 0.5f,
                    emitter->spread_or_width, emitter->speed, emitter->count);
        break;
    case ATTACK_EMITTER_SPIRAL_BURST:
        EmitSpiralBurst(game, enemy->position, emitter->speed, emitter->count, emitter->angle_scale,
                        emitter->radius, emitter->bullet_kind, enemy->phase_clock);
        break;
    case ATTACK_EMITTER_VERTICAL_LASER:
        StartLaserVolley(game, enemy, enemy->position, DifficultyPatternCount(game, emitter->count),
                         emitter->spread_or_width, emitter->radius,
                         emitter->charge_time * DifficultyCooldownScale(game),
                         emitter->active_time);
        break;
    }
}

static AttackPatternId GetEnemyActivePattern(const Enemy *enemy)
{
    AttackPatternId pattern = enemy->phase_patterns[0];

    if (enemy->phase_patterns[1] != ATTACK_PATTERN_NONE && enemy->hp <= enemy->phase_thresholds[0])
    {
        pattern = enemy->phase_patterns[1];
    }

    if (enemy->phase_patterns[2] != ATTACK_PATTERN_NONE && enemy->hp <= enemy->phase_thresholds[1])
    {
        pattern = enemy->phase_patterns[2];
    }

    return pattern;
}

static void SpawnEnemyFromArchetype(Game *game, const WaveSpawnDef *spawn)
{
    const EnemyArchetypeDef *archetype = StageGetEnemyArchetype(spawn->archetype);
    Vector2 position;
    Vector2 velocity;
    int hp;
    Enemy *enemy;

    if (archetype == NULL)
    {
        return;
    }

    position =
        (Vector2){game->playfield.x + game->playfield.width * spawn->x_factor + spawn->x_offset,
                  game->playfield.y + spawn->y_offset};
    velocity = (Vector2){spawn->velocity_x, spawn->velocity_y};
    hp = archetype->kind == ENEMY_BOSS ? DifficultyBossHp(game, archetype->hp) : archetype->hp;

    enemy = SpawnEnemy(game, archetype->kind, position, velocity, archetype->radius, hp,
                       archetype->base_cooldown + spawn->cooldown_offset,
                       archetype->base_aux_cooldown + spawn->aux_cooldown_offset);
    if (enemy == NULL)
    {
        return;
    }

    enemy->movement = archetype->movement;
    enemy->collision_scale = archetype->collision_scale;
    enemy->reward = archetype->reward;
    enemy->lifetime = archetype->lifetime;
    enemy->attack_start_y = archetype->attack_start_y;
    enemy->move_param_a = archetype->move_param_a;
    enemy->move_param_b = archetype->move_param_b;
    enemy->move_param_c = spawn->movement_phase_offset;
    if (archetype->movement == ENEMY_MOVE_BOSS_CORE)
    {
        enemy->move_param_c = archetype->move_param_c;
    }
    for (int i = 0; i < 3; ++i)
    {
        enemy->phase_patterns[i] = archetype->phase_patterns[i];
    }
    enemy->phase_thresholds[0] = (int)((float)enemy->max_hp * archetype->phase_ratios[0]);
    enemy->phase_thresholds[1] = (int)((float)enemy->max_hp * archetype->phase_ratios[1]);

    if (enemy->kind == ENEMY_BOSS)
    {
        game->boss_spawned = true;
        AudioPlaySfx(AUDIO_SFX_BOSS_ALERT);
    }
}

static void ExecuteWave(Game *game, WaveId wave_id)
{
    const WaveDef *wave = StageGetWaveDefinition(wave_id);

    if (wave == NULL)
    {
        return;
    }

    if (wave->clear_non_boss_on_start)
    {
        ClearEnemyBullets(game);

        for (int i = 0; i < MAX_ENEMIES; ++i)
        {
            if (game->enemies[i].active && game->enemies[i].kind != ENEMY_BOSS)
            {
                SpawnBurst(game, game->enemies[i].position, COLOR_PANEL_LINE, 6, 120.0f);
                game->enemies[i].active = false;
            }
        }
    }

    for (int i = 0; i < wave->spawn_count; ++i)
    {
        SpawnEnemyFromArchetype(game, &wave->spawns[i]);
    }
}

static void JumpToBoss(Game *game)
{
    game->stage_time = 22.1f;
    game->next_stage_cue = game->stage != NULL ? game->stage->cue_count : 0;
    ExecuteWave(game, game->stage != NULL ? game->stage->boss_wave : WAVE_BOSS_ENTRY);
    SetStatusMessage(game, "PHASE MODE: BOSS ENTRY");
}

static void ProcessStageCues(Game *game)
{
    if (game->stage == NULL)
    {
        return;
    }

    while (game->next_stage_cue < game->stage->cue_count &&
           game->stage->cues[game->next_stage_cue].trigger_time <= game->stage_time)
    {
        ExecuteWave(game, game->stage->cues[game->next_stage_cue].wave);
        game->next_stage_cue++;
    }
}

static float ScaleEnemyCooldown(const Game *game, float base_cooldown)
{
    return base_cooldown * DifficultyCooldownScale(game);
}

static int CountActiveMissiles(const Game *game)
{
    int active = 0;

    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i)
    {
        if (game->player_bullets[i].active && game->player_bullets[i].kind == BULLET_MISSILE)
        {
            active++;
        }
    }

    return active;
}

static Enemy *FindNearestEnemy(Game *game, Vector2 position)
{
    Enemy *nearest = NULL;
    float nearest_distance = 0.0f;

    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        Enemy *enemy = &game->enemies[i];
        float distance;

        if (!enemy->active)
        {
            continue;
        }

        distance = LogicDistanceSquared(ToLogic(position), ToLogic(enemy->position));
        if (nearest == NULL || distance < nearest_distance)
        {
            nearest = enemy;
            nearest_distance = distance;
        }
    }

    return nearest;
}

static void SpawnMissile(Game *game)
{
    Bullet *bullet;
    Enemy *target;
    Vector2 origin;
    Vector2 initial_velocity;

    if (CountActiveMissiles(game) >= 2)
    {
        return;
    }

    target = FindNearestEnemy(game, game->player.position);
    if (target == NULL)
    {
        return;
    }

    bullet = ReserveBullet(game->player_bullets, MAX_PLAYER_BULLETS);
    if (bullet == NULL)
    {
        return;
    }

    origin = (Vector2){game->player.position.x, game->player.position.y - 22.0f};
    initial_velocity = ToVector(LogicPolar(-1.57079632679f, 430.0f));

    bullet->active = true;
    bullet->kind = BULLET_MISSILE;
    bullet->position = origin;
    bullet->velocity = initial_velocity;
    bullet->radius = 7.0f;
    bullet->age = 0.0f;
    bullet->grazed = false;

    SpawnParticle(game, origin, (Vector2){0.0f, 60.0f}, 4.0f, 0.22f, COLOR_MISSILE);
    AudioPlaySfx(AUDIO_SFX_PLAYER_MISSILE);
}

static void UseBomb(Game *game)
{
    if (!game->player.alive || game->player.bombs <= 0 || game->player.bomb_cooldown > 0.0f)
    {
        return;
    }

    game->player.bombs--;
    game->player.bomb_cooldown = 0.35f;
    if (game->player.invulnerable_timer < 1.4f)
    {
        game->player.invulnerable_timer = 1.4f;
    }
    game->score += 250;

    AudioPlaySfx(AUDIO_SFX_BOMB);
    ClearEnemyBullets(game);
    SpawnBurst(game, game->player.position, COLOR_MISSILE, 28, 210.0f);

    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        Enemy *enemy = &game->enemies[i];

        if (!enemy->active)
        {
            continue;
        }

        if (enemy->kind == ENEMY_BOSS)
        {
            int bomb_damage = (int)LogicClamp((float)enemy->max_hp * 0.12f, 24.0f, 64.0f);

            enemy->hp -= bomb_damage;
            SpawnParticle(game, enemy->position, (Vector2){0.0f, -30.0f}, 7.0f, 0.35f,
                          COLOR_MISSILE);
            if (enemy->hp <= 0)
            {
                DestroyEnemy(game, enemy);
            }
        }
        else
        {
            DestroyEnemy(game, enemy);
        }
    }
}

static void StartRespawn(Game *game)
{
    game->player.alive = false;
    game->player.respawn_timer = 1.0f;
    game->player.invulnerable_timer = 2.2f;
    game->player.position = (Vector2){
        .x = game->playfield.x + game->playfield.width * 0.5f,
        .y = game->playfield.y + game->playfield.height - 88.0f,
    };
    game->player.shot_timer = 0.0f;
    game->player.missile_timer = 0.35f;
    game->player.bomb_cooldown = 0.0f;
    ClearEnemyBullets(game);
}

static void DamagePlayer(Game *game)
{
    if (game->debug_invulnerable)
    {
        return;
    }

    AudioPlaySfx(AUDIO_SFX_PLAYER_DEATH);
    SpawnBurst(game, game->player.position, COLOR_PLAYER_ACCENT, 18, 200.0f);

    if (!game->debug_infinite_lives)
    {
        game->player.lives--;
    }

    if (game->player.lives <= 0)
    {
        game->mode = GAME_MODE_GAME_OVER;
        game->state_time = 0.0f;
        game->player.alive = false;
        return;
    }

    StartRespawn(game);
}

static void UpdatePlayer(Game *game, float dt)
{
    Vector2 move = {0.0f, 0.0f};
    bool focus = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float speed = focus ? 220.0f : 360.0f;

    if (!game->player.alive)
    {
        game->player.respawn_timer -= dt;

        if (game->player.respawn_timer <= 0.0f)
        {
            game->player.alive = true;
        }
    }

    if (game->player.invulnerable_timer > 0.0f)
    {
        game->player.invulnerable_timer -= dt;
    }

    if (game->player.bomb_cooldown > 0.0f)
    {
        game->player.bomb_cooldown -= dt;
    }

    if (!game->player.alive)
    {
        return;
    }

    if (game->player.bombs > 0 && (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_C)))
    {
        UseBomb(game);
    }

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    {
        move.y -= 1.0f;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    {
        move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    {
        move.x -= 1.0f;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    {
        move.x += 1.0f;
    }

    if (move.x != 0.0f || move.y != 0.0f)
    {
        LogicVec2 normalized = LogicNormalize(ToLogic(move));
        game->player.position.x += normalized.x * speed * dt;
        game->player.position.y += normalized.y * speed * dt;
    }

    game->player.position.x = ClampToPlayfieldX(game->player.position.x);
    game->player.position.y = ClampToPlayfieldY(game->player.position.y);

    game->player.shot_timer -= dt;
    game->player.missile_timer -= dt;
    if (game->player.shot_timer <= 0.0f)
    {
        game->player.shot_timer += focus ? 0.08f : 0.09f;
        AudioPlaySfx(AUDIO_SFX_PLAYER_SHOT);

        if (focus)
        {
            SpawnPlayerBullet(
                game, (Vector2){game->player.position.x - 8.0f, game->player.position.y - 14.0f},
                (Vector2){0.0f, -760.0f});
            SpawnPlayerBullet(game,
                              (Vector2){game->player.position.x, game->player.position.y - 18.0f},
                              (Vector2){0.0f, -800.0f});
            SpawnPlayerBullet(
                game, (Vector2){game->player.position.x + 8.0f, game->player.position.y - 14.0f},
                (Vector2){0.0f, -760.0f});
        }
        else
        {
            SpawnPlayerBullet(
                game, (Vector2){game->player.position.x - 12.0f, game->player.position.y - 14.0f},
                (Vector2){-70.0f, -720.0f});
            SpawnPlayerBullet(game,
                              (Vector2){game->player.position.x, game->player.position.y - 18.0f},
                              (Vector2){0.0f, -760.0f});
            SpawnPlayerBullet(
                game, (Vector2){game->player.position.x + 12.0f, game->player.position.y - 14.0f},
                (Vector2){70.0f, -720.0f});
        }
    }

    if (game->player.missile_timer <= 0.0f)
    {
        game->player.missile_timer += DifficultyMissileInterval(game);
        SpawnMissile(game);
    }
}

static void UpdateParticles(Game *game, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        Particle *particle = &game->particles[i];

        if (!particle->active)
        {
            continue;
        }

        particle->age += dt;
        if (particle->age >= particle->lifetime)
        {
            particle->active = false;
            continue;
        }

        particle->position.x += particle->velocity.x * dt;
        particle->position.y += particle->velocity.y * dt;
        particle->velocity.x *= 0.985f;
        particle->velocity.y *= 0.985f;
    }
}

static void DestroyEnemy(Game *game, Enemy *enemy)
{
    Color color = COLOR_SWEEP;

    switch (enemy->kind)
    {
    case ENEMY_SWEEP:
        color = COLOR_SWEEP;
        break;
    case ENEMY_AIMER:
        color = COLOR_AIMER;
        break;
    case ENEMY_ORBITER:
        color = COLOR_ORBIT;
        break;
    case ENEMY_SENTINEL:
        color = COLOR_SENTINEL;
        break;
    case ENEMY_BOSS:
        color = COLOR_BOSS;
        game->boss_defeated = true;
        game->clear_timer = 2.0f;
        ClearEnemyBullets(game);
        break;
    }

    game->score += enemy->reward;
    AudioPlaySfx(enemy->kind == ENEMY_BOSS ? AUDIO_SFX_STAGE_CLEAR : AUDIO_SFX_ENEMY_DESTROY);
    SpawnBurst(game, enemy->position, color, enemy->kind == ENEMY_BOSS ? 42 : 16,
               enemy->kind == ENEMY_BOSS ? 240.0f : 160.0f);
    enemy->active = false;
}

static void UpdatePlayerBullets(Game *game, float dt)
{
    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i)
    {
        Bullet *bullet = &game->player_bullets[i];

        if (!bullet->active)
        {
            continue;
        }

        bullet->age += dt;

        if (bullet->kind == BULLET_MISSILE)
        {
            Enemy *target = FindNearestEnemy(game, bullet->position);

            if (target != NULL)
            {
                LogicVec2 desired =
                    LogicNormalize(LogicSub(ToLogic(target->position), ToLogic(bullet->position)));
                LogicVec2 current = LogicNormalize(ToLogic(bullet->velocity));
                LogicVec2 steered = LogicNormalize(
                    LogicAdd(LogicScale(current, 1.0f), LogicScale(desired, 6.0f * dt)));
                bullet->velocity = ToVector(LogicScale(steered, 430.0f));
            }
        }

        bullet->position.x += bullet->velocity.x * dt;
        bullet->position.y += bullet->velocity.y * dt;

        if (!IsInsideExtendedPlayfield(bullet->position, 32.0f))
        {
            bullet->active = false;
            continue;
        }

        for (int j = 0; j < MAX_ENEMIES; ++j)
        {
            Enemy *enemy = &game->enemies[j];

            if (!enemy->active)
            {
                continue;
            }

            if (LogicCircleOverlap(ToLogic(bullet->position), bullet->radius,
                                   ToLogic(enemy->position), enemy->radius))
            {
                int damage;

                switch (bullet->kind)
                {
                case BULLET_PLAYER:
                    damage = enemy->kind == ENEMY_BOSS ? 1 : 2;
                    break;
                case BULLET_MISSILE:
                    damage = enemy->kind == ENEMY_BOSS ? 6 : 12;
                    break;
                default:
                    damage = 1;
                    break;
                }

                enemy->hp -= damage;
                bullet->active = false;
                SpawnParticle(game, bullet->position, (Vector2){0.0f, -45.0f},
                              bullet->kind == BULLET_MISSILE ? 4.0f : 2.0f, 0.12f,
                              bullet->kind == BULLET_MISSILE ? COLOR_MISSILE : COLOR_PLAYER);

                if (enemy->hp <= 0)
                {
                    DestroyEnemy(game, enemy);
                }
                break;
            }
        }
    }
}

static void UpdateEnemyBulletGrazeAndHit(Game *game)
{
    if (!game->player.alive)
    {
        return;
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        Bullet *bullet = &game->enemy_bullets[i];

        if (!bullet->active)
        {
            continue;
        }

        if (!bullet->grazed &&
            LogicCircleOverlap(ToLogic(bullet->position), bullet->radius + PLAYER_GRAZE_RADIUS,
                               ToLogic(game->player.position), PLAYER_RADIUS))
        {
            bullet->grazed = true;
            game->score += 5;
            SpawnParticle(game, bullet->position, (Vector2){0.0f, -20.0f}, 2.0f, 0.18f,
                          COLOR_PLAYER_ACCENT);
        }

        if (game->player.invulnerable_timer <= 0.0f &&
            LogicCircleOverlap(ToLogic(bullet->position), bullet->radius,
                               ToLogic(game->player.position), PLAYER_HITBOX_RADIUS))
        {
            DamagePlayer(game);
            return;
        }
    }

    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        Enemy *enemy = &game->enemies[i];

        if (!enemy->active || enemy->kind == ENEMY_BOSS)
        {
            continue;
        }

        if (game->player.invulnerable_timer <= 0.0f &&
            LogicCircleOverlap(ToLogic(enemy->position), enemy->radius * enemy->collision_scale,
                               ToLogic(game->player.position), PLAYER_HITBOX_RADIUS))
        {
            DamagePlayer(game);
            return;
        }
    }
}

static void UpdateEnemyBullets(Game *game, float dt)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        Bullet *bullet = &game->enemy_bullets[i];

        if (!bullet->active)
        {
            continue;
        }

        bullet->age += dt;
        bullet->position.x += bullet->velocity.x * dt;
        bullet->position.y += bullet->velocity.y * dt;

        if (bullet->kind == BULLET_SPIRAL)
        {
            float angle = atan2f(bullet->velocity.y, bullet->velocity.x) + 0.7f * dt;
            float speed = LogicLength(ToLogic(bullet->velocity));
            bullet->velocity = ToVector(LogicPolar(angle, speed));
        }

        if (!IsInsideExtendedPlayfield(bullet->position, 40.0f))
        {
            bullet->active = false;
        }
    }

    UpdateEnemyBulletGrazeAndHit(game);
}

static void UpdateBeams(Game *game, float dt)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        Beam *beam = &game->beams[i];

        if (!beam->active)
        {
            continue;
        }

        if (!beam->harmful)
        {
            if (beam->track_source && beam->owner_index >= 0 && beam->owner_index < MAX_ENEMIES &&
                game->enemies[beam->owner_index].active)
            {
                const Enemy *source = &game->enemies[beam->owner_index];

                beam->x = LogicClamp(source->position.x + beam->x_offset, game->playfield.x + 24.0f,
                                     game->playfield.x + game->playfield.width - 24.0f);
                beam->origin_y = LogicClamp(source->position.y, game->playfield.y + 12.0f,
                                            game->playfield.y + game->playfield.height - 12.0f);
            }

            beam->charge_timer -= dt;
            if (beam->charge_timer <= 0.0f)
            {
                beam->harmful = true;
                beam->track_source = false;
                if (beam->play_fire_sfx)
                {
                    AudioPlaySfx(AUDIO_SFX_LASER_FIRE);
                }
            }
        }
        else
        {
            beam->active_timer -= dt;
            if (beam->active_timer <= 0.0f)
            {
                beam->active = false;
                continue;
            }

            if (game->player.alive && game->player.invulnerable_timer <= 0.0f &&
                game->player.position.y >= beam->origin_y - PLAYER_HITBOX_RADIUS &&
                fabsf(game->player.position.x - beam->x) <= beam->width + PLAYER_HITBOX_RADIUS)
            {
                DamagePlayer(game);
                return;
            }
        }
    }
}

static bool HasChargingBeams(const Game *game)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (game->beams[i].active && !game->beams[i].harmful)
        {
            return true;
        }
    }

    return false;
}

static void UpdateEnemy(Game *game, Enemy *enemy, float dt)
{
    AttackPatternId active_pattern_id;
    const AttackPatternDef *pattern;

    enemy->age += dt;
    enemy->phase_clock += dt;
    enemy->cooldown -= dt;
    enemy->aux_cooldown -= dt;

    switch (enemy->movement)
    {
    case ENEMY_MOVE_SWEEP_BOUNCE:
        enemy->position.x += enemy->velocity.x * dt;
        enemy->position.y += enemy->velocity.y * dt;

        if (enemy->position.x < game->playfield.x + enemy->move_param_a ||
            enemy->position.x > game->playfield.x + game->playfield.width - enemy->move_param_a)
        {
            enemy->velocity.x *= -1.0f;
        }
        break;

    case ENEMY_MOVE_AIMER_DRIFT:
        enemy->position.x += enemy->velocity.x * dt;
        enemy->position.y += enemy->velocity.y * dt;
        enemy->velocity.y =
            LogicApproach(enemy->velocity.y, enemy->move_param_a, enemy->move_param_b * dt);
        break;

    case ENEMY_MOVE_ORBIT_SINE:
        enemy->position.y += enemy->velocity.y * dt;
        enemy->position.x =
            enemy->anchor_x +
            sinf(enemy->age * enemy->move_param_b + enemy->move_param_c) * enemy->move_param_a;
        break;

    case ENEMY_MOVE_SENTINEL_HOLD:
        enemy->position.x += enemy->velocity.x * dt;
        enemy->position.y += enemy->velocity.y * dt;
        if (enemy->position.y >= game->playfield.y + enemy->move_param_a)
        {
            enemy->position.y = LogicApproach(enemy->position.y,
                                              game->playfield.y + enemy->move_param_a, 140.0f * dt);
            enemy->velocity.y = LogicApproach(enemy->velocity.y, 0.0f, enemy->move_param_b * dt);
        }
        break;

    case ENEMY_MOVE_BOSS_CORE:
        enemy->position.y =
            LogicApproach(enemy->position.y, game->playfield.y + enemy->move_param_a, 120.0f * dt);
        enemy->position.x = game->playfield.x + game->playfield.width * 0.5f +
                            sinf(enemy->age * enemy->move_param_c) * enemy->move_param_b;
        break;
    }

    active_pattern_id = GetEnemyActivePattern(enemy);
    pattern = StageGetAttackPattern(active_pattern_id);

    if (pattern != NULL)
    {
        if (pattern->primary.type != ATTACK_EMITTER_NONE && enemy->cooldown <= 0.0f &&
            enemy->position.y > game->playfield.y + enemy->attack_start_y)
        {
            EmitAttackEmitter(game, enemy, &pattern->primary);

            if (pattern->secondary.type != ATTACK_EMITTER_NONE &&
                !pattern->secondary_uses_aux_timer)
            {
                EmitAttackEmitter(game, enemy, &pattern->secondary);
            }

            enemy->cooldown = ScaleEnemyCooldown(game, pattern->cooldown);
        }

        if (pattern->secondary.type != ATTACK_EMITTER_NONE && pattern->secondary_uses_aux_timer &&
            enemy->aux_cooldown <= 0.0f &&
            enemy->position.y > game->playfield.y + enemy->attack_start_y)
        {
            EmitAttackEmitter(game, enemy, &pattern->secondary);
            enemy->aux_cooldown = ScaleEnemyCooldown(game, pattern->secondary_cooldown);
        }
    }

    if (enemy->lifetime > 0.0f && enemy->age > enemy->lifetime)
    {
        enemy->active = false;
    }

    if (!IsInsideExtendedPlayfield(enemy->position, 120.0f) && enemy->kind != ENEMY_BOSS)
    {
        enemy->active = false;
    }
}

static void UpdateEnemies(Game *game, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        if (!game->enemies[i].active)
        {
            continue;
        }

        UpdateEnemy(game, &game->enemies[i], dt);
    }
}

static bool CheatBufferEndsWith(const Game *game, const char *code)
{
    size_t code_length = strlen(code);

    if ((size_t)game->cheat_length < code_length)
    {
        return false;
    }

    return memcmp(game->cheat_buffer + game->cheat_length - (int)code_length, code, code_length) ==
           0;
}

static void UpdateTitleCheats(Game *game)
{
    int pressed = GetCharPressed();

    while (pressed > 0)
    {
        if (isalnum(pressed))
        {
            if (game->cheat_length >= (int)sizeof(game->cheat_buffer) - 1)
            {
                memmove(game->cheat_buffer, game->cheat_buffer + 1, sizeof(game->cheat_buffer) - 2);
                game->cheat_length = (int)sizeof(game->cheat_buffer) - 2;
            }

            game->cheat_buffer[game->cheat_length++] =
                (char)(isalpha(pressed) ? tolower(pressed) : pressed);
            game->cheat_buffer[game->cheat_length] = '\0';

            if (CheatBufferEndsWith(game, "ghost"))
            {
                game->debug_invulnerable = !game->debug_invulnerable;
                SetStatusMessage(game,
                                 game->debug_invulnerable ? "CHEAT: GHOST ON" : "CHEAT: GHOST OFF");
                game->cheat_length = 0;
                game->cheat_buffer[0] = '\0';
            }
            else if (CheatBufferEndsWith(game, "eternal"))
            {
                game->debug_infinite_lives = !game->debug_infinite_lives;
                SetStatusMessage(game, game->debug_infinite_lives ? "CHEAT: ETERNAL ON"
                                                                  : "CHEAT: ETERNAL OFF");
                game->cheat_length = 0;
                game->cheat_buffer[0] = '\0';
            }
            else if (CheatBufferEndsWith(game, "phase"))
            {
                game->debug_start_at_boss = !game->debug_start_at_boss;
                SetStatusMessage(game, game->debug_start_at_boss ? "CHEAT: PHASE ON"
                                                                 : "CHEAT: PHASE OFF");
                game->cheat_length = 0;
                game->cheat_buffer[0] = '\0';
            }
            else if (CheatBufferEndsWith(game, "stage1"))
            {
                game->debug_stage_override = STAGE_ONE;
                SetStatusMessage(game, "CHEAT: START AT STAGE ONE");
                game->cheat_length = 0;
                game->cheat_buffer[0] = '\0';
            }
            else if (CheatBufferEndsWith(game, "stage2"))
            {
                game->debug_stage_override = STAGE_TWO;
                SetStatusMessage(game, "CHEAT: START AT STAGE TWO");
                game->cheat_length = 0;
                game->cheat_buffer[0] = '\0';
            }
        }

        pressed = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && game->cheat_length > 0)
    {
        game->cheat_length--;
        game->cheat_buffer[game->cheat_length] = '\0';
    }
}

static void UpdateTitle(Game *game)
{
    UpdateTitleCheats(game);

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1))
    {
        game->difficulty = DIFFICULTY_CASUAL;
        AudioPlaySfx(AUDIO_SFX_MENU_MOVE);
        SetStatusMessage(game, "DIFFICULTY: CASUAL");
    }
    else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))
    {
        game->difficulty = DIFFICULTY_STANDARD;
        AudioPlaySfx(AUDIO_SFX_MENU_MOVE);
        SetStatusMessage(game, "DIFFICULTY: STANDARD");
    }
    else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))
    {
        game->difficulty = DIFFICULTY_EXPERT;
        AudioPlaySfx(AUDIO_SFX_MENU_MOVE);
        SetStatusMessage(game, "DIFFICULTY: EXPERT");
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        game->difficulty =
            (Difficulty)((game->difficulty + DIFFICULTY_EXPERT) % (DIFFICULTY_EXPERT + 1));
        AudioPlaySfx(AUDIO_SFX_MENU_MOVE);
        SetStatusMessage(game, TextFormat("DIFFICULTY: %s", DifficultyLabel(game->difficulty)));
    }
    else if (IsKeyPressed(KEY_RIGHT))
    {
        game->difficulty = (Difficulty)((game->difficulty + 1) % (DIFFICULTY_EXPERT + 1));
        AudioPlaySfx(AUDIO_SFX_MENU_MOVE);
        SetStatusMessage(game, TextFormat("DIFFICULTY: %s", DifficultyLabel(game->difficulty)));
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        AudioPlaySfx(AUDIO_SFX_MENU_SELECT);
        ResetRun(game);
    }
}

void GameUpdate(Game *game, float dt)
{
    dt = LogicClamp(dt, 0.0f, 1.0f / 30.0f);
    game->state_time += dt;
    if (game->status_timer > 0.0f)
    {
        game->status_timer -= dt;
    }
    UpdateParticles(game, dt);

    if (game->mode == GAME_MODE_TITLE)
    {
        AudioSetLaserHumActive(false);
        UpdateTitle(game);
        return;
    }

    if (game->mode == GAME_MODE_GAME_OVER || game->mode == GAME_MODE_CLEAR)
    {
        AudioSetLaserHumActive(false);
        if (game->mode == GAME_MODE_CLEAR)
        {
            Enemy *boss = FindBoss(game);
            if (boss != NULL)
            {
                boss->active = false;
            }
        }

        if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER))
        {
            ResetRun(game);
        }
        return;
    }

    game->stage_time += dt;
    ProcessStageCues(game);
    UpdatePlayer(game, dt);
    UpdateEnemies(game, dt);
    UpdatePlayerBullets(game, dt);
    UpdateEnemyBullets(game, dt);
    UpdateBeams(game, dt);
    AudioSetLaserHumActive(HasChargingBeams(game));

    if (game->boss_defeated)
    {
        game->clear_timer -= dt;
        if (game->clear_timer <= 0.0f)
        {
            const StageDef *next_stage = StageGetNext(game->stage);

            if (next_stage != NULL)
            {
                AdvanceToStage(game, next_stage);
            }
            else
            {
                game->mode = GAME_MODE_CLEAR;
                game->state_time = 0.0f;
            }
        }
    }
}

static void DrawBackdrop(const Game *game)
{
    float pulse = 0.5f + 0.5f * sinf(game->state_time * 0.8f);

    ClearBackground(COLOR_BG);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG);
    DrawRectangle(22, 22, PLAYFIELD_X - 42, SCREEN_HEIGHT - 44, COLOR_PANEL);
    DrawRectangle(PLAYFIELD_X + PLAYFIELD_WIDTH + 20, 22,
                  SCREEN_WIDTH - (PLAYFIELD_X + PLAYFIELD_WIDTH + 42), SCREEN_HEIGHT - 44,
                  COLOR_PANEL);
    DrawRectangleRec(game->playfield, COLOR_PLAYFIELD);
    DrawRectangleLinesEx(game->playfield, 2.0f, COLOR_PANEL_LINE);

    for (int i = 0; i < 14; ++i)
    {
        float y = game->playfield.y +
                  fmodf(game->state_time * (20.0f + (float)i * 7.0f) + (float)i * 58.0f,
                        game->playfield.height);
        Color line = ColorAlpha(COLOR_GRID, 0.25f + 0.15f * pulse);
        DrawLineEx((Vector2){game->playfield.x + 12.0f, y},
                   (Vector2){game->playfield.x + game->playfield.width - 12.0f, y},
                   1.0f + (float)(i % 2), line);
    }

    for (int i = 0; i < 9; ++i)
    {
        float t = game->state_time * (14.0f + (float)i * 2.5f) + (float)i * 71.0f;
        float y = game->playfield.y + fmodf(t, game->playfield.height + 120.0f) - 60.0f;
        float x = game->playfield.x + game->playfield.width * (0.12f + 0.08f * (float)i) +
                  sinf(game->state_time * 0.7f + (float)i) * 28.0f;
        DrawPolyLinesEx((Vector2){x, y}, 4, 10.0f + (float)(i % 3) * 6.0f,
                        game->state_time * 18.0f + (float)i * 14.0f, 1.5f,
                        ColorAlpha(COLOR_GRID, 0.5f));
    }
}

static void DrawPlayer(const Game *game)
{
    float blink = 0.5f + 0.5f * sinf(game->state_time * 16.0f);
    Color body = COLOR_PLAYER;
    bool focus = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (!game->player.alive)
    {
        return;
    }

    if (game->player.invulnerable_timer > 0.0f)
    {
        body = ColorAlpha(body, 0.35f + 0.45f * blink);
    }

    DrawTriangle((Vector2){game->player.position.x, game->player.position.y - 16.0f},
                 (Vector2){game->player.position.x - 12.0f, game->player.position.y + 10.0f},
                 (Vector2){game->player.position.x + 12.0f, game->player.position.y + 10.0f}, body);
    DrawPoly((Vector2){game->player.position.x, game->player.position.y + 6.0f}, 4, 8.0f, 45.0f,
             COLOR_PLAYER_ACCENT);

    if (focus)
    {
        DrawCircleLines((int)game->player.position.x, (int)game->player.position.y,
                        PLAYER_GRAZE_RADIUS, ColorAlpha(COLOR_PLAYER_ACCENT, 0.24f));
        DrawCircleV(game->player.position, PLAYER_HITBOX_RADIUS, COLOR_PLAYER);
    }
}

static void DrawEnemy(const Enemy *enemy)
{
    switch (enemy->kind)
    {
    case ENEMY_SWEEP:
        DrawPolyLinesEx(enemy->position, 4, enemy->radius, enemy->age * 40.0f, 3.0f, COLOR_SWEEP);
        DrawLineEx((Vector2){enemy->position.x - enemy->radius, enemy->position.y},
                   (Vector2){enemy->position.x + enemy->radius, enemy->position.y}, 2.0f,
                   COLOR_SWEEP);
        break;

    case ENEMY_AIMER:
        DrawPoly(enemy->position, 3, enemy->radius, 180.0f, ColorAlpha(COLOR_AIMER, 0.25f));
        DrawPolyLinesEx(enemy->position, 3, enemy->radius, 180.0f, 3.0f, COLOR_AIMER);
        break;

    case ENEMY_ORBITER:
        DrawCircleLines((int)enemy->position.x, (int)enemy->position.y, enemy->radius, COLOR_ORBIT);
        DrawPolyLinesEx(enemy->position, 6, enemy->radius - 4.0f, enemy->age * 30.0f, 2.5f,
                        COLOR_ORBIT);
        break;

    case ENEMY_SENTINEL:
        DrawRectangleLinesEx((Rectangle){enemy->position.x - enemy->radius,
                                         enemy->position.y - enemy->radius * 0.9f,
                                         enemy->radius * 2.0f, enemy->radius * 1.8f},
                             3.0f, COLOR_SENTINEL);
        DrawLineEx((Vector2){enemy->position.x - enemy->radius + 4.0f, enemy->position.y},
                   (Vector2){enemy->position.x + enemy->radius - 4.0f, enemy->position.y}, 2.5f,
                   COLOR_SENTINEL);
        DrawCircleV(enemy->position, 5.0f, COLOR_PLAYER);
        break;

    case ENEMY_BOSS:
        DrawPolyLinesEx(enemy->position, 4, enemy->radius + 12.0f, enemy->age * 22.0f, 4.0f,
                        COLOR_BOSS);
        DrawCircleLines((int)enemy->position.x, (int)enemy->position.y, enemy->radius + 26.0f,
                        ColorAlpha(COLOR_SPIRAL, 0.5f));
        DrawPoly(enemy->position, 6, enemy->radius - 10.0f, enemy->age * 30.0f,
                 ColorAlpha(COLOR_AIMED, 0.18f));
        DrawCircleV(enemy->position, enemy->radius - 18.0f, COLOR_BOSS);
        DrawCircleV(enemy->position, 7.0f, COLOR_BG);
        break;
    }
}

static void DrawBullet(const Bullet *bullet)
{
    float rotation = atan2f(bullet->velocity.y, bullet->velocity.x) * RAD2DEG;

    switch (bullet->kind)
    {
    case BULLET_PLAYER:
        DrawRectanglePro((Rectangle){bullet->position.x, bullet->position.y, 5.0f, 18.0f},
                         (Vector2){2.5f, 9.0f}, rotation + 90.0f, COLOR_PLAYER);
        break;

    case BULLET_MISSILE:
        DrawPoly(bullet->position, 4, bullet->radius + 2.0f, rotation + 45.0f, COLOR_MISSILE);
        DrawCircleV(bullet->position, 2.5f, COLOR_PLAYER);
        break;

    case BULLET_RING:
        DrawCircleV(bullet->position, bullet->radius, COLOR_RING);
        DrawCircleLines((int)bullet->position.x, (int)bullet->position.y, bullet->radius + 1.0f,
                        ColorAlpha(COLOR_RING, 0.65f));
        break;

    case BULLET_AIMED:
        DrawPoly(bullet->position, 4, bullet->radius + 1.0f, rotation + 45.0f, COLOR_AIMED);
        break;

    case BULLET_WALL:
        DrawRectanglePro((Rectangle){bullet->position.x, bullet->position.y, bullet->radius * 2.7f,
                                     bullet->radius * 1.15f},
                         (Vector2){bullet->radius * 1.35f, bullet->radius * 0.575f}, rotation,
                         COLOR_WALL);
        break;

    case BULLET_SPIRAL:
        DrawPoly(bullet->position, 5, bullet->radius + 1.0f, rotation, COLOR_SPIRAL);
        break;
    }
}

static void DrawParticles(const Game *game)
{
    for (int i = 0; i < MAX_PARTICLES; ++i)
    {
        const Particle *particle = &game->particles[i];
        float alpha;

        if (!particle->active)
        {
            continue;
        }

        alpha = 1.0f - particle->age / particle->lifetime;
        DrawCircleV(particle->position, particle->radius * alpha,
                    ColorAlpha(particle->color, alpha));
    }
}

static void DrawBeams(const Game *game)
{
    float playfield_bottom = game->playfield.y + game->playfield.height;
    float pulse = 0.5f + 0.5f * sinf(game->state_time * 12.0f);

    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        const Beam *beam = &game->beams[i];
        float height;

        if (!beam->active)
        {
            continue;
        }

        height = playfield_bottom - beam->origin_y;
        if (height <= 0.0f)
        {
            continue;
        }

        if (!beam->harmful)
        {
            DrawRectangle((int)(beam->x - beam->telegraph_width), (int)beam->origin_y,
                          (int)(beam->telegraph_width * 2.0f), (int)height,
                          ColorAlpha(COLOR_WALL, 0.14f + 0.1f * pulse));
            DrawLineEx((Vector2){beam->x, beam->origin_y}, (Vector2){beam->x, playfield_bottom},
                       beam->telegraph_width, ColorAlpha(COLOR_PLAYER, 0.8f));
        }
        else
        {
            DrawRectangle((int)(beam->x - beam->width), (int)beam->origin_y,
                          (int)(beam->width * 2.0f), (int)height,
                          ColorAlpha(COLOR_LASER_LIVE, 0.34f));
            DrawRectangle((int)(beam->x - beam->width * 0.45f), (int)beam->origin_y,
                          (int)(beam->width * 0.9f), (int)height, COLOR_PLAYER);
            DrawLineEx((Vector2){beam->x, beam->origin_y}, (Vector2){beam->x, playfield_bottom},
                       beam->width * 1.15f, ColorAlpha(COLOR_LASER_LIVE, 0.85f));
        }
    }
}

static void DrawHud(const Game *game)
{
    const Enemy *boss = FindBossConst(game);
    int left_x = 52;
    int right_x = PLAYFIELD_X + PLAYFIELD_WIDTH + 46;
    int right_panel_width = SCREEN_WIDTH - (PLAYFIELD_X + PLAYFIELD_WIDTH + 42);
    float boss_bar_width = (float)(right_panel_width - 40);

    DrawText("AXIOM", left_x, 64, 34, COLOR_PLAYER);
    DrawText("NULL", left_x, 96, 34, COLOR_PLAYER_ACCENT);
    DrawText("WASD / ARROWS", left_x, 170, 20, COLOR_PLAYER);
    DrawText("MOVE", left_x, 194, 18, ColorAlpha(COLOR_PLAYER, 0.6f));
    DrawText("SHIFT", left_x, 242, 20, COLOR_PLAYER);
    DrawText("FOCUS", left_x, 266, 18, ColorAlpha(COLOR_PLAYER, 0.6f));
    DrawText("X / C", left_x, 314, 20, COLOR_PLAYER);
    DrawText("BOMB", left_x, 338, 18, ColorAlpha(COLOR_PLAYER, 0.6f));
    DrawText("ENTER", left_x, 362, 20, COLOR_PLAYER);
    DrawText("START / RESTART", left_x, 386, 18, ColorAlpha(COLOR_PLAYER, 0.6f));

    DrawText("SCORE", right_x, 64, 22, ColorAlpha(COLOR_PLAYER, 0.7f));
    DrawText(TextFormat("%08u", game->score), right_x, 92, 34, COLOR_PLAYER);
    DrawText("MODE", right_x, 146, 22, ColorAlpha(COLOR_PLAYER, 0.7f));
    DrawText(DifficultyLabel(game->difficulty), right_x, 172, 24,
             DifficultyColor(game->difficulty));
    DrawText("LIVES", right_x, 220, 22, ColorAlpha(COLOR_PLAYER, 0.7f));

    for (int i = 0; i < game->player.lives; ++i)
    {
        DrawPoly((Vector2){(float)right_x + 18.0f + (float)i * 28.0f, 264.0f}, 4, 10.0f, 45.0f,
                 COLOR_PLAYER_ACCENT);
    }

    DrawText("BOMBS", right_x, 308, 22, ColorAlpha(COLOR_PLAYER, 0.7f));
    for (int i = 0; i < game->player.bombs; ++i)
    {
        DrawCircleV((Vector2){(float)right_x + 18.0f + (float)i * 28.0f, 350.0f}, 9.0f,
                    COLOR_MISSILE);
    }

    DrawText("STAGE", right_x, 392, 22, ColorAlpha(COLOR_PLAYER, 0.7f));
    if (!game->boss_spawned)
    {
        DrawText(StageHudLabel(game), right_x, 420, 30, COLOR_PLAYER);
        DrawText(TextFormat("%04.1fs", game->stage_time), right_x, 458, 24,
                 ColorAlpha(COLOR_PLAYER, 0.7f));
    }
    else if (!game->boss_defeated)
    {
        DrawText("BOSS", right_x, 420, 30, COLOR_SPIRAL);
    }
    else
    {
        DrawText("CLEAR", right_x, 420, 30, COLOR_AIMED);
    }

    if (boss != NULL)
    {
        float ratio = LogicClamp((float)boss->hp / (float)boss->max_hp, 0.0f, 1.0f);
        Rectangle frame = {(float)right_x, 510.0f, boss_bar_width, 16.0f};
        DrawText("CORE", right_x, 538, 18, ColorAlpha(COLOR_PLAYER, 0.7f));
        DrawRectangleRec(frame, COLOR_GRID);
        DrawRectangle((int)frame.x + 2, (int)frame.y + 2, (int)((frame.width - 4.0f) * ratio),
                      (int)frame.height - 4, COLOR_BOSS);
        DrawRectangleLinesEx(frame, 2.0f, COLOR_PANEL_LINE);
    }

    if (game->debug_invulnerable || game->debug_infinite_lives || game->debug_start_at_boss ||
        game->debug_stage_override != STAGE_ONE)
    {
        int debug_y = 584;
        DrawText("DEBUG", right_x, debug_y, 18, ColorAlpha(COLOR_PLAYER, 0.7f));
        if (game->debug_invulnerable)
        {
            DrawText("GHOST", right_x, debug_y + 28, 18, COLOR_AIMED);
            debug_y += 28;
        }
        if (game->debug_infinite_lives)
        {
            DrawText("ETERNAL", right_x, debug_y + 28, 18, COLOR_PLAYER_ACCENT);
            debug_y += 28;
        }
        if (game->debug_start_at_boss)
        {
            DrawText("PHASE", right_x, debug_y + 28, 18, COLOR_MISSILE);
            debug_y += 28;
        }
        if (game->debug_stage_override != STAGE_ONE)
        {
            DrawText(TextFormat("STAGE %s", DebugStageOverrideLabel(game)), right_x, debug_y + 28,
                     18, COLOR_WALL);
        }
    }
}

static void DrawTitleOverlay(const Game *game)
{
    float pulse = 0.5f + 0.5f * sinf(game->state_time * 2.2f);
    Color text_color = ColorAlpha(COLOR_PLAYER, 0.8f + 0.2f * pulse);
    bool show_debug = game->debug_invulnerable || game->debug_infinite_lives ||
                      game->debug_start_at_boss || game->debug_stage_override != STAGE_ONE;
    int panel_x = (int)game->playfield.x + 48;
    int panel_y = (int)game->playfield.y + 210;
    int panel_height = show_debug ? (game->debug_stage_override != STAGE_ONE ? 414 : 388) : 348;
    int text_x = PLAYFIELD_X + 72;

    DrawRectangle(panel_x, panel_y, PLAYFIELD_WIDTH - 96, panel_height, Fade(COLOR_PANEL, 0.78f));
    DrawText("ABSTRACT MINIMALIST", text_x, PLAYFIELD_Y + 250, 20, COLOR_PLAYER_ACCENT);
    DrawText("VERTICAL BULLET HELL", text_x, PLAYFIELD_Y + 282, 32, text_color);
    DrawText("graze bullets / survive the stage", text_x, PLAYFIELD_Y + 334, 18,
             ColorAlpha(COLOR_PLAYER, 0.65f));
    DrawText("break the core", text_x, PLAYFIELD_Y + 358, 18, ColorAlpha(COLOR_PLAYER, 0.65f));
    DrawText("DIFFICULTY", text_x, PLAYFIELD_Y + 402, 20, ColorAlpha(COLOR_PLAYER, 0.65f));
    DrawText("[1] CASUAL  [2] STANDARD", text_x, PLAYFIELD_Y + 430, 20, COLOR_PLAYER);
    DrawText("[3] EXPERT", text_x, PLAYFIELD_Y + 456, 20, COLOR_PLAYER);
    DrawText(TextFormat("CURRENT: %s", DifficultyLabel(game->difficulty)), text_x,
             PLAYFIELD_Y + 490, 24, DifficultyColor(game->difficulty));
    DrawText("PRESS ENTER", text_x, PLAYFIELD_Y + 528, 28, text_color);

    if (show_debug)
    {
        DrawText("DEBUG MODIFIERS ACTIVE", text_x, PLAYFIELD_Y + 566, 18, COLOR_MISSILE);
        if (game->debug_stage_override != STAGE_ONE)
        {
            DrawText(TextFormat("START STAGE %s", DebugStageOverrideLabel(game)), text_x,
                     PLAYFIELD_Y + 592, 18, COLOR_WALL);
        }
    }
}

static void DrawEndOverlay(const Game *game, const char *headline, Color color)
{
    float pulse = 0.5f + 0.5f * sinf(game->state_time * 3.0f);

    DrawRectangle((int)game->playfield.x + 66, (int)game->playfield.y + 230, PLAYFIELD_WIDTH - 132,
                  180, Fade(COLOR_PANEL, 0.82f));
    DrawText(headline, PLAYFIELD_X + 112, PLAYFIELD_Y + 282, 44,
             ColorAlpha(color, 0.8f + 0.2f * pulse));
    DrawText("PRESS R OR ENTER", PLAYFIELD_X + 112, PLAYFIELD_Y + 342, 24, COLOR_PLAYER);
}

void GameDraw(const Game *game)
{
    DrawBackdrop(game);

    BeginScissorMode((int)game->playfield.x, (int)game->playfield.y, (int)game->playfield.width,
                     (int)game->playfield.height);
    DrawParticles(game);
    DrawBeams(game);

    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i)
    {
        if (game->player_bullets[i].active)
        {
            DrawBullet(&game->player_bullets[i]);
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        if (game->enemy_bullets[i].active)
        {
            DrawBullet(&game->enemy_bullets[i]);
        }
    }

    for (int i = 0; i < MAX_ENEMIES; ++i)
    {
        if (game->enemies[i].active)
        {
            DrawEnemy(&game->enemies[i]);
        }
    }

    DrawPlayer(game);
    EndScissorMode();
    DrawHud(game);

    if (game->mode == GAME_MODE_TITLE)
    {
        DrawTitleOverlay(game);
    }
    else if (game->mode == GAME_MODE_GAME_OVER)
    {
        DrawEndOverlay(game, "SYSTEM BREAK", COLOR_RING);
    }
    else if (game->mode == GAME_MODE_CLEAR)
    {
        DrawEndOverlay(game, "AXIOM SHATTERED", COLOR_AIMED);
    }

    if (game->status_timer > 0.0f && game->status_message[0] != '\0')
    {
        DrawRectangle(PLAYFIELD_X + 90, PLAYFIELD_Y + PLAYFIELD_HEIGHT - 76, PLAYFIELD_WIDTH - 180,
                      38, Fade(COLOR_PANEL, 0.84f));
        DrawText(game->status_message, PLAYFIELD_X + 108, PLAYFIELD_Y + PLAYFIELD_HEIGHT - 68, 20,
                 COLOR_PLAYER);
    }
}
