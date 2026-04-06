#include "stage.h"

#include <stddef.h>

static const AttackPatternDef g_patterns[] = {
    {
        .id = ATTACK_PATTERN_SWEEP_FAN_RING,
        .primary = {.type = ATTACK_EMITTER_FAN,
                    .bullet_kind = BULLET_RING,
                    .speed = 235.0f,
                    .count = 5,
                    .spread_or_width = 1.0f,
                    .radius = 7.0f},
        .cooldown = 1.15f,
    },
    {
        .id = ATTACK_PATTERN_AIMER_FAN_AIMED,
        .primary = {.type = ATTACK_EMITTER_FAN,
                    .bullet_kind = BULLET_AIMED,
                    .speed = 275.0f,
                    .count = 4,
                    .spread_or_width = 0.72f,
                    .radius = 6.5f},
        .cooldown = 1.0f,
    },
    {
        .id = ATTACK_PATTERN_ORBITER_RING_WALL,
        .primary = {.type = ATTACK_EMITTER_RING,
                    .bullet_kind = BULLET_WALL,
                    .speed = 180.0f,
                    .count = 12,
                    .angle_scale = 0.8f,
                    .radius = 7.5f},
        .cooldown = 1.05f,
    },
    {
        .id = ATTACK_PATTERN_BOSS_PHASE_ONE,
        .primary = {.type = ATTACK_EMITTER_RING,
                    .bullet_kind = BULLET_RING,
                    .speed = 185.0f,
                    .count = 18,
                    .angle_scale = 0.6f,
                    .radius = 7.0f},
        .cooldown = 0.72f,
        .secondary = {.type = ATTACK_EMITTER_RING,
                      .bullet_kind = BULLET_AIMED,
                      .speed = 270.0f,
                      .count = 9,
                      .angle_scale = -0.85f,
                      .radius = 5.5f},
    },
    {
        .id = ATTACK_PATTERN_BOSS_PHASE_TWO,
        .primary = {.type = ATTACK_EMITTER_FAN,
                    .bullet_kind = BULLET_AIMED,
                    .speed = 310.0f,
                    .count = 9,
                    .spread_or_width = 1.35f,
                    .radius = 6.0f},
        .cooldown = 0.56f,
        .secondary = {.type = ATTACK_EMITTER_CURTAIN,
                      .bullet_kind = BULLET_WALL,
                      .speed = 220.0f,
                      .count = 10,
                      .spread_or_width = 476.0f,
                      .radius = 8.5f},
        .secondary_cooldown = 1.6f,
        .secondary_uses_aux_timer = true,
    },
    {
        .id = ATTACK_PATTERN_BOSS_PHASE_THREE,
        .primary = {.type = ATTACK_EMITTER_SPIRAL_BURST,
                    .bullet_kind = BULLET_SPIRAL,
                    .speed = 240.0f,
                    .count = 4,
                    .angle_scale = 3.8f,
                    .radius = 6.5f},
        .cooldown = 0.09f,
        .secondary = {.type = ATTACK_EMITTER_FAN,
                      .bullet_kind = BULLET_WALL,
                      .speed = 340.0f,
                      .count = 11,
                      .spread_or_width = 1.7f,
                      .radius = 7.0f},
        .secondary_cooldown = 1.05f,
        .secondary_uses_aux_timer = true,
    },
};

static const EnemyArchetypeDef g_archetypes[] = {
    {
        .id = ENEMY_ARCHETYPE_SWEEP,
        .kind = ENEMY_SWEEP,
        .movement = ENEMY_MOVE_SWEEP_BOUNCE,
        .radius = 18.0f,
        .hp = 16,
        .base_cooldown = 0.65f,
        .collision_scale = 0.55f,
        .reward = 100,
        .lifetime = 8.0f,
        .attack_start_y = 30.0f,
        .move_param_a = 26.0f,
        .phase_patterns = {ATTACK_PATTERN_SWEEP_FAN_RING, ATTACK_PATTERN_NONE, ATTACK_PATTERN_NONE},
    },
    {
        .id = ENEMY_ARCHETYPE_AIMER,
        .kind = ENEMY_AIMER,
        .movement = ENEMY_MOVE_AIMER_DRIFT,
        .radius = 20.0f,
        .hp = 26,
        .base_cooldown = 1.0f,
        .collision_scale = 0.55f,
        .reward = 180,
        .lifetime = 7.0f,
        .attack_start_y = 40.0f,
        .move_param_a = 28.0f,
        .move_param_b = 80.0f,
        .phase_patterns = {ATTACK_PATTERN_AIMER_FAN_AIMED, ATTACK_PATTERN_NONE,
                           ATTACK_PATTERN_NONE},
    },
    {
        .id = ENEMY_ARCHETYPE_AIMER_HEAVY,
        .kind = ENEMY_AIMER,
        .movement = ENEMY_MOVE_AIMER_DRIFT,
        .radius = 20.0f,
        .hp = 34,
        .base_cooldown = 1.0f,
        .collision_scale = 0.55f,
        .reward = 180,
        .lifetime = 7.0f,
        .attack_start_y = 40.0f,
        .move_param_a = 28.0f,
        .move_param_b = 80.0f,
        .phase_patterns = {ATTACK_PATTERN_AIMER_FAN_AIMED, ATTACK_PATTERN_NONE,
                           ATTACK_PATTERN_NONE},
    },
    {
        .id = ENEMY_ARCHETYPE_ORBITER,
        .kind = ENEMY_ORBITER,
        .movement = ENEMY_MOVE_ORBIT_SINE,
        .radius = 22.0f,
        .hp = 32,
        .base_cooldown = 0.75f,
        .collision_scale = 0.55f,
        .reward = 240,
        .lifetime = 7.8f,
        .attack_start_y = 45.0f,
        .move_param_a = 86.0f,
        .move_param_b = 1.9f,
        .phase_patterns = {ATTACK_PATTERN_ORBITER_RING_WALL, ATTACK_PATTERN_NONE,
                           ATTACK_PATTERN_NONE},
    },
    {
        .id = ENEMY_ARCHETYPE_BOSS_CORE,
        .kind = ENEMY_BOSS,
        .movement = ENEMY_MOVE_BOSS_CORE,
        .radius = 46.0f,
        .hp = 320,
        .base_cooldown = 0.6f,
        .base_aux_cooldown = 1.25f,
        .reward = 3000,
        .attack_start_y = 30.0f,
        .move_param_a = 165.0f,
        .move_param_b = 158.0f,
        .move_param_c = 0.72f,
        .phase_patterns = {ATTACK_PATTERN_BOSS_PHASE_ONE, ATTACK_PATTERN_BOSS_PHASE_TWO,
                           ATTACK_PATTERN_BOSS_PHASE_THREE},
        .phase_ratios = {0.6875f, 0.375f},
    },
};

static const WaveDef g_waves[] = {
    {
        .id = WAVE_OPEN_SWEEP_ROW,
        .spawn_count = 5,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.089286f,
                 .y_offset = -20.0f,
                 .velocity_x = 50.0f,
                 .velocity_y = 82.0f,
                 .cooldown_offset = 0.00f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.294643f,
                 .y_offset = -20.0f,
                 .velocity_x = -50.0f,
                 .velocity_y = 82.0f,
                 .cooldown_offset = 0.08f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.500000f,
                 .y_offset = -20.0f,
                 .velocity_x = 50.0f,
                 .velocity_y = 82.0f,
                 .cooldown_offset = 0.16f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.705357f,
                 .y_offset = -20.0f,
                 .velocity_x = -50.0f,
                 .velocity_y = 82.0f,
                 .cooldown_offset = 0.24f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.910714f,
                 .y_offset = -20.0f,
                 .velocity_x = 50.0f,
                 .velocity_y = 82.0f,
                 .cooldown_offset = 0.32f},
            },
    },
    {
        .id = WAVE_AIMER_PAIR,
        .spawn_count = 2,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_AIMER,
                 .x_factor = 0.214286f,
                 .y_offset = -10.0f,
                 .velocity_x = 42.0f,
                 .velocity_y = 52.0f},
                {.archetype = ENEMY_ARCHETYPE_AIMER,
                 .x_factor = 0.785714f,
                 .y_offset = -40.0f,
                 .velocity_x = -42.0f,
                 .velocity_y = 52.0f,
                 .cooldown_offset = 0.2f},
            },
    },
    {
        .id = WAVE_ORBIT_PAIR,
        .spawn_count = 2,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_ORBITER,
                 .x_factor = 0.267857f,
                 .y_offset = -40.0f,
                 .velocity_y = 42.0f},
                {.archetype = ENEMY_ARCHETYPE_ORBITER,
                 .x_factor = 0.732143f,
                 .y_offset = -85.0f,
                 .velocity_y = 42.0f,
                 .cooldown_offset = 0.35f,
                 .movement_phase_offset = 1.1f},
            },
    },
    {
        .id = WAVE_MID_SWEEP_PRESSURE,
        .spawn_count = 7,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.089286f,
                 .y_offset = -30.0f,
                 .velocity_x = 90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.00f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.253571f,
                 .y_offset = -30.0f,
                 .velocity_x = -90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.08f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.417857f,
                 .y_offset = -30.0f,
                 .velocity_x = 90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.16f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.582143f,
                 .y_offset = -30.0f,
                 .velocity_x = -90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.24f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.746429f,
                 .y_offset = -30.0f,
                 .velocity_x = 90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.32f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.910714f,
                 .y_offset = -30.0f,
                 .velocity_x = -90.0f,
                 .velocity_y = 105.0f,
                 .cooldown_offset = 0.40f},
                {.archetype = ENEMY_ARCHETYPE_AIMER_HEAVY,
                 .x_factor = 0.500000f,
                 .y_offset = -60.0f,
                 .velocity_y = 64.0f},
            },
    },
    {
        .id = WAVE_LATE_PINCH,
        .spawn_count = 6,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_AIMER,
                 .x_factor = 0.214286f,
                 .y_offset = -20.0f,
                 .velocity_x = 58.0f,
                 .velocity_y = 52.0f},
                {.archetype = ENEMY_ARCHETYPE_AIMER,
                 .x_factor = 0.785714f,
                 .y_offset = -50.0f,
                 .velocity_x = -58.0f,
                 .velocity_y = 52.0f,
                 .cooldown_offset = 0.2f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.089286f,
                 .y_offset = -20.0f,
                 .velocity_x = 50.0f,
                 .velocity_y = 128.0f,
                 .cooldown_offset = 0.00f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.363095f,
                 .y_offset = -20.0f,
                 .velocity_x = -50.0f,
                 .velocity_y = 128.0f,
                 .cooldown_offset = 0.08f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.636905f,
                 .y_offset = -20.0f,
                 .velocity_x = 50.0f,
                 .velocity_y = 128.0f,
                 .cooldown_offset = 0.16f},
                {.archetype = ENEMY_ARCHETYPE_SWEEP,
                 .x_factor = 0.910714f,
                 .y_offset = -20.0f,
                 .velocity_x = -50.0f,
                 .velocity_y = 128.0f,
                 .cooldown_offset = 0.24f},
            },
    },
    {
        .id = WAVE_BOSS_ENTRY,
        .clear_non_boss_on_start = true,
        .spawn_count = 1,
        .spawns =
            {
                {.archetype = ENEMY_ARCHETYPE_BOSS_CORE,
                 .x_factor = 0.500000f,
                 .y_offset = -120.0f},
            },
    },
};

static const StageCue g_stage_one_cues[] = {
    {.trigger_time = 0.8f, .wave = WAVE_OPEN_SWEEP_ROW},
    {.trigger_time = 4.6f, .wave = WAVE_AIMER_PAIR},
    {.trigger_time = 8.2f, .wave = WAVE_ORBIT_PAIR},
    {.trigger_time = 12.4f, .wave = WAVE_MID_SWEEP_PRESSURE},
    {.trigger_time = 17.0f, .wave = WAVE_LATE_PINCH},
    {.trigger_time = 22.0f, .wave = WAVE_BOSS_ENTRY},
};

static const StageDef g_stage_one = {
    .name = "stage-one",
    .cue_count = (int)(sizeof(g_stage_one_cues) / sizeof(g_stage_one_cues[0])),
    .cues = g_stage_one_cues,
};

const StageDef *StageGetDefault(void) { return &g_stage_one; }

const WaveDef *StageGetWaveDefinition(WaveId id)
{
    for (int i = 0; i < (int)(sizeof(g_waves) / sizeof(g_waves[0])); ++i)
    {
        if (g_waves[i].id == id)
        {
            return &g_waves[i];
        }
    }

    return NULL;
}

const EnemyArchetypeDef *StageGetEnemyArchetype(EnemyArchetypeId id)
{
    for (int i = 0; i < (int)(sizeof(g_archetypes) / sizeof(g_archetypes[0])); ++i)
    {
        if (g_archetypes[i].id == id)
        {
            return &g_archetypes[i];
        }
    }

    return NULL;
}

const AttackPatternDef *StageGetAttackPattern(AttackPatternId id)
{
    for (int i = 0; i < (int)(sizeof(g_patterns) / sizeof(g_patterns[0])); ++i)
    {
        if (g_patterns[i].id == id)
        {
            return &g_patterns[i];
        }
    }

    return NULL;
}
