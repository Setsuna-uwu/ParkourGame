#include "game.h"
#include <string.h>

/* 一种障碍组合的摆法。types[i] 指定第 i 个障碍长什么样，offsets[i] 指定它离路段起点多远。 */
typedef struct PatternDef {
    float width;
    int count;
    int types[SP_TERRAIN_MAX_OBSTACLES];
    float offsets[SP_TERRAIN_MAX_OBSTACLES];
} PatternDef;

/* 模板宽度包含最后一块障碍之后的少量余量。 */
/* 每行是一种固定摆法。例如第三行放两个冰刺，分别从这一段的第 0 和第 38 像素开始。count 决定读几个元素。 */
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

/* 整个数组的字节数除以一行的字节数，就得到模板行数。新增一行时数量会跟着变。 */
#define SP_PATTERN_COUNT ((int)(sizeof(PATTERNS) / sizeof(PATTERNS[0])))

/* 按种类填宽、高两个结果。调用方传来 &block.w 和 &block.h；这里对 *width、*height 赋值，就能改到那两个成员。 */
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

/* 按各模板的权重抽出一个数组下标。权重越大，被抽中的机会越多；hard_weight 随游戏难度升高。 */
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

    /* 把每行的权重想成首尾相接的线段，在总长度内随机选一点。每跨过一段就减去它的长度，落在哪一段就选那一行。 */
    roll = rng_float01(rng) * total;
    for (i = 0; i < SP_PATTERN_COUNT; ++i) {
        if (roll < weights[i]) {
            return i;
        }
        roll -= weights[i];
    }
    return 1;
}

/* 抽一个模板摆好障碍，把生成的路段追加到地形数组，并预留下一段的起点。 */
static void terrain_spawn_segment(Terrain *terrain, const DifficultyParams *difficulty)
{
    /* 先在局部变量里拼好一段路，再放到路段数组的末尾。 */
    /* {0} 把这一段的所有成员都置零，接下来只填写抽中模板实际用到的部分。 */
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
    /* 数组下标从 0 开始。已有 3 段时占用 0、1、2，所以下标 3 正好是空位。 */
    terrain->segments[terrain->segment_count] = segment;
    terrain->segment_count += 1;
    terrain->generated += 1;

    /* 在基础间距上随机加一个 -18 到 37 的整数，再用下限保证相邻两段之间有空隙。 */
    jitter = rng_range(&terrain->rng, -18, 38);
    gap = difficulty->gap + (float)jitter;
    if (gap < 260.0f) {
        gap = 260.0f;
    }
    terrain->next_x = segment.x + segment.width + gap;
}

/* 循环补路段，直到铺到屏幕右方的预留距离，或数组已满。这样障碍会从屏幕外逐渐滑入。 */
static void terrain_fill_ahead(Terrain *terrain, const DifficultyParams *difficulty)
{
    while (terrain->segment_count < SP_TERRAIN_MAX_SEGMENTS &&
           terrain->next_x < (float)SP_SCREEN_WIDTH + SP_TERRAIN_AHEAD) {
        terrain_spawn_segment(terrain, difficulty);
    }
}

/* 新局清空旧路段，把第一段起点放在角色前方，再用这一局的种子生成初始障碍。 */
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

/* 每一步把路段和障碍左移 dx 像素，回收离屏的路段，再到右侧补足新的路段。 */
void terrain_scroll(Terrain *terrain, float dx, const DifficultyParams *difficulty)
{
    int i;
    int j;

    if (terrain == 0 || difficulty == 0 || dx <= 0.0f) {
        return;
    }

    for (i = 0; i < terrain->segment_count; ++i) {
        /* &terrain->segments[i] 取第 i 段路的地址。下面通过 segment 改坐标，数组里那段路就会一起变。 */
        TerrainSegment *segment = &terrain->segments[i];
        segment->x -= dx;
        /* 障碍坐标是世界坐标的一部分，必须与所属模板同步滚动。 */
        for (j = 0; j < segment->obstacle_count; ++j) {
            segment->obstacles[j].x -= dx;
        }
    }
    terrain->next_x -= dx;

    /* && 从左往右判断。先确认还有路段，才去读第 0 段；它的右边缘也越过回收线时，就能腾出这个位置。 */
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

/* 把每个已生成路段的障碍数累加，返回当前地形数组中的障碍总数。 */
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

/* 给伪随机数生成器设起点。相同种子经过同样次数的抽取会得到相同结果，方便复现一局。 */
void rng_seed(Rng *rng, uint32_t seed)
{
    if (rng == 0) {
        return;
    }
    /* 0 是 xorshift 的固定点，替换为非零常量。 */
    /* 条件 ? A : B 表示条件成立取 A，否则取 B。这里传入 0 时改用一个固定的非零种子。 */
    rng->state = (seed != 0u) ? seed : 0x9E3779B9u;
}

/* 根据上次的状态算出下一项，同时保存它供下次抽取。uint32_t 表示 32 位无符号整数。 */
uint32_t rng_next_u32(Rng *rng)
{
    uint32_t x;

    if (rng == 0) {
        return 0u;
    }
    x = rng->state;
    /* <<、>> 将二进制位左移或右移。^ 是按位异或，对应位不同就得 1；x ^= y 等价于 x = x ^ y。这三步把各位混合。 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (x == 0u) {
        x = 0x6D2B79F5u;
    }
    rng->state = x;
    return x;
}

/* 生成下限到上限之间的整数，包含下限、不包含上限。例如 rng_range(rng, 2, 5) 可得到 2、3、4。 */
int rng_range(Rng *rng, int min_inclusive, int max_exclusive)
{
    uint32_t span;
    uint32_t value;

    if (max_exclusive <= min_inclusive) {
        return min_inclusive;
    }
    span = (uint32_t)(max_exclusive - min_inclusive);
    /* % 是求余数，结果落在 0 到 span - 1；加上下限，就移到想要的区间。 */
    value = rng_next_u32(rng) % span;
    return min_inclusive + (int)value;
}

/* 把一次抽取变成小数。右移 8 位留下高 24 位，再除以 2 的 24 次方，结果能取到 0，但始终小于 1。 */
float rng_float01(Rng *rng)
{
    /* 使用高 24 位，结果严格位于 [0, 1)。 */
    return (float)(rng_next_u32(rng) >> 8) * (1.0f / 16777216.0f);
}
