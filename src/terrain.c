#include "game.h"
#include <string.h>

typedef struct PatternDef {
    float width;
    int count;
    int types[SP_TERRAIN_MAX_OBSTACLES];
    float offsets[SP_TERRAIN_MAX_OBSTACLES];
} PatternDef;

/* 模板宽度包含最后一块障碍之后的少量余量。 */
static const PatternDef PATTERNS[] = {
    {44.0f, 1, {OBSTACLE_CRATE, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}},
    {34.0f, 1, {OBSTACLE_SPIKE, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}},
    {72.0f, 2, {OBSTACLE_SPIKE, OBSTACLE_SPIKE, 0, 0}, {0.0f, 38.0f, 0.0f, 0.0f}},
    {40.0f, 1, {OBSTACLE_TALL, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}},
    {80.0f, 1, {OBSTACLE_WIDE, 0, 0, 0}, {0.0f, 0.0f, 0.0f, 0.0f}},
    {120.0f, 3, {OBSTACLE_SPIKE, OBSTACLE_SPIKE, OBSTACLE_SPIKE, 0}, {0.0f, 43.0f, 86.0f, 0.0f}},
    {92.0f, 2, {OBSTACLE_TALL, OBSTACLE_SPIKE, 0, 0}, {0.0f, 52.0f, 0.0f, 0.0f}},
    {114.0f, 2, {OBSTACLE_CRATE, OBSTACLE_CRATE, 0, 0}, {0.0f, 70.0f, 0.0f, 0.0f}}
};

#define SP_PATTERN_COUNT ((int)(sizeof(PATTERNS) / sizeof(PATTERNS[0])))

static void obstacle_dimensions(int type, float *width, float *height)
{
    switch (type) {
    case OBSTACLE_SPIKE:
        *width = 34.0f;
        *height = 42.0f;
        break;
    case OBSTACLE_CRATE:
        *width = 44.0f;
        *height = 48.0f;
        break;
    case OBSTACLE_TALL:
        *width = 40.0f;
        *height = 82.0f;
        break;
    case OBSTACLE_WIDE:
    default:
        *width = 80.0f;
        *height = 34.0f;
        break;
    }
}

static int choose_pattern(Rng *rng, float hard_weight)
{
    float weights[SP_PATTERN_COUNT];
    float total = 0.0f;
    float roll;
    int i;

    weights[0] = 1.40f;
    weights[1] = 1.80f;
    weights[2] = 0.30f + 1.40f * hard_weight;
    weights[3] = 0.15f + 1.00f * hard_weight;
    weights[4] = 0.10f + 1.00f * hard_weight;
    weights[5] = 0.00f + 0.90f * hard_weight;
    weights[6] = 0.00f + 0.80f * hard_weight;
    weights[7] = 0.10f + 0.70f * hard_weight;

    for (i = 0; i < SP_PATTERN_COUNT; ++i) {
        total += weights[i];
    }
    if (total <= 0.0f) {
        return 1;
    }

    roll = rng_float01(rng) * total;
    for (i = 0; i < SP_PATTERN_COUNT; ++i) {
        if (roll < weights[i]) {
            return i;
        }
        roll -= weights[i];
    }
    return 1;
}

static void terrain_spawn_segment(Terrain *terrain, const DifficultyParams *difficulty)
{
    /* 先在局部变量里拼好一段路，再放到路段数组的末尾。 */
    TerrainSegment segment = {0};
    int index = choose_pattern(&terrain->rng, difficulty->hard_weight);
    PatternDef pattern = PATTERNS[index];
    float gap;
    int jitter;
    int i;

    segment.x = terrain->next_x;
    segment.width = pattern.width;
    segment.pattern = index;
    segment.obstacle_count = pattern.count;
    for (i = 0; i < pattern.count; ++i) {
        Obstacle block;
        block.type = pattern.types[i];
        obstacle_dimensions(block.type, &block.w, &block.h);
        block.x = segment.x + pattern.offsets[i];
        block.y = terrain->ground_y - block.h;
        segment.obstacles[i] = block;
    }
    terrain->segments[terrain->segment_count] = segment;
    terrain->segment_count += 1;
    terrain->generated += 1;

    jitter = rng_range(&terrain->rng, -18, 38);
    gap = difficulty->gap + (float)jitter;
    if (gap < 260.0f) {
        gap = 260.0f;
    }
    terrain->next_x = segment.x + segment.width + gap;
}

static void terrain_fill_ahead(Terrain *terrain, const DifficultyParams *difficulty)
{
    while (terrain->segment_count < SP_TERRAIN_MAX_SEGMENTS &&
           terrain->next_x < (float)SP_SCREEN_WIDTH + SP_TERRAIN_AHEAD) {
        terrain_spawn_segment(terrain, difficulty);
    }
}

void terrain_init(Terrain *terrain, uint32_t seed, float ground_y, const DifficultyParams *difficulty)
{
    if (terrain == 0 || difficulty == 0) {
        return;
    }
    memset(terrain, 0, sizeof(*terrain));
    terrain->ground_y = ground_y;
    terrain->next_x = SP_TERRAIN_SAFE_LEAD;
    rng_seed(&terrain->rng, seed);
    terrain_fill_ahead(terrain, difficulty);
}

void terrain_scroll(Terrain *terrain, float dx, const DifficultyParams *difficulty)
{
    int i;
    int j;

    if (terrain == 0 || difficulty == 0 || dx <= 0.0f) {
        return;
    }

    for (i = 0; i < terrain->segment_count; ++i) {
        TerrainSegment *segment = &terrain->segments[i];
        segment->x -= dx;
        /* 障碍坐标是世界坐标的一部分，必须与所属模板同步滚动。 */
        for (j = 0; j < segment->obstacle_count; ++j) {
            segment->obstacles[j].x -= dx;
        }
    }
    terrain->next_x -= dx;

    while (terrain->segment_count > 0 &&
           terrain->segments[0].x + terrain->segments[0].width < SP_TERRAIN_RECYCLE_X) {
        /* 第 0 段已经滚出画面，后面的路段依次向前挪一个位置。 */
        for (i = 1; i < terrain->segment_count; ++i) {
            terrain->segments[i - 1] = terrain->segments[i];
        }
        terrain->segment_count -= 1;
    }

    terrain_fill_ahead(terrain, difficulty);
}

int terrain_obstacle_count(const Terrain *terrain)
{
    int total = 0;
    int i;

    if (terrain == 0) {
        return 0;
    }
    for (i = 0; i < terrain->segment_count; ++i) {
        total += terrain->segments[i].obstacle_count;
    }
    return total;
}

void rng_seed(Rng *rng, uint32_t seed)
{
    if (rng == 0) {
        return;
    }
    /* 0 是 xorshift 的固定点，替换为非零常量。 */
    rng->state = (seed != 0u) ? seed : 0x9E3779B9u;
}

uint32_t rng_next_u32(Rng *rng)
{
    uint32_t x;

    if (rng == 0) {
        return 0u;
    }
    x = rng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (x == 0u) {
        x = 0x6D2B79F5u;
    }
    rng->state = x;
    return x;
}

int rng_range(Rng *rng, int min_inclusive, int max_exclusive)
{
    uint32_t span;
    uint32_t value;

    if (max_exclusive <= min_inclusive) {
        return min_inclusive;
    }
    span = (uint32_t)(max_exclusive - min_inclusive);
    value = rng_next_u32(rng) % span;
    return min_inclusive + (int)value;
}

float rng_float01(Rng *rng)
{
    /* 使用高 24 位，结果严格位于 [0, 1)。 */
    return (float)(rng_next_u32(rng) >> 8) * (1.0f / 16777216.0f);
}
