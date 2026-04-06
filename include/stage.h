#ifndef STAGE_H
#define STAGE_H

#include <stdbool.h>

#include "game.h"

#define MAX_WAVE_SPAWNS 8

typedef enum AttackEmitterType
{
    ATTACK_EMITTER_NONE = 0,
    ATTACK_EMITTER_FAN,
    ATTACK_EMITTER_RING,
    ATTACK_EMITTER_CURTAIN,
    ATTACK_EMITTER_SPIRAL_BURST
} AttackEmitterType;

typedef struct AttackEmitterDef
{
    AttackEmitterType type;
    BulletKind bullet_kind;
    float speed;
    int count;
    float spread_or_width;
    float angle_scale;
    float radius;
} AttackEmitterDef;

typedef struct AttackPatternDef
{
    AttackPatternId id;
    AttackEmitterDef primary;
    float cooldown;
    AttackEmitterDef secondary;
    float secondary_cooldown;
    bool secondary_uses_aux_timer;
} AttackPatternDef;

typedef enum EnemyArchetypeId
{
    ENEMY_ARCHETYPE_SWEEP = 0,
    ENEMY_ARCHETYPE_AIMER,
    ENEMY_ARCHETYPE_AIMER_HEAVY,
    ENEMY_ARCHETYPE_ORBITER,
    ENEMY_ARCHETYPE_BOSS_CORE
} EnemyArchetypeId;

typedef struct EnemyArchetypeDef
{
    EnemyArchetypeId id;
    EnemyKind kind;
    EnemyMovementModel movement;
    float radius;
    int hp;
    float base_cooldown;
    float base_aux_cooldown;
    float collision_scale;
    unsigned int reward;
    float lifetime;
    float attack_start_y;
    float move_param_a;
    float move_param_b;
    float move_param_c;
    AttackPatternId phase_patterns[3];
    float phase_ratios[2];
} EnemyArchetypeDef;

typedef struct WaveSpawnDef
{
    EnemyArchetypeId archetype;
    float x_factor;
    float x_offset;
    float y_offset;
    float velocity_x;
    float velocity_y;
    float cooldown_offset;
    float aux_cooldown_offset;
    float movement_phase_offset;
} WaveSpawnDef;

typedef enum WaveId
{
    WAVE_OPEN_SWEEP_ROW = 0,
    WAVE_AIMER_PAIR,
    WAVE_ORBIT_PAIR,
    WAVE_MID_SWEEP_PRESSURE,
    WAVE_LATE_PINCH,
    WAVE_BOSS_ENTRY
} WaveId;

typedef struct WaveDef
{
    WaveId id;
    bool clear_non_boss_on_start;
    int spawn_count;
    WaveSpawnDef spawns[MAX_WAVE_SPAWNS];
} WaveDef;

typedef struct StageCue
{
    float trigger_time;
    WaveId wave;
} StageCue;

typedef struct StageDef
{
    const char *name;
    int cue_count;
    const StageCue *cues;
} StageDef;

const StageDef *StageGetDefault(void);
const WaveDef *StageGetWaveDefinition(WaveId id);
const EnemyArchetypeDef *StageGetEnemyArchetype(EnemyArchetypeId id);
const AttackPatternDef *StageGetAttackPattern(AttackPatternId id);

#endif
