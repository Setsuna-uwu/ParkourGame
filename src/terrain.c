#include "game.h"

/* #8 地形分段骨架：
   - 种子 0 保留原来的固定试跳坡，方便和随机路线对照。
   - 其他种子生成一条连续、可复现的雪道。
   - ground_at / ramp_at 只查询已经生成好的段，不在绘制时重新随机。
   - 每条沟的宽度按角色最低速度校验；过宽的组合不会进入正式路线。 */

#define MAX_SEGMENTS 96
#define WORLD_END 100000.0f
#define FIRST_RANDOM_START 720.0f
#define MIN_GROUND_Y 220.0f
#define MAX_GROUND_Y 400.0f
#define PLAYER_MIN_SPEED 3.0f
#define SAFE_JUMP_FRAMES 36.0f
#define MAX_SAFE_GAP (PLAYER_MIN_SPEED * SAFE_JUMP_FRAMES)
#define MIN_RUNWAY_AFTER_GAP 360.0f

enum {
    SEG_FLAT,
    SEG_SLOPE,
    SEG_GAP,
    SEG_LANDING
};

typedef struct {
    int type;
    float start;
    float end;
    float start_y;
    float slope;
    int has_ground;
    int has_ramp;
    float ramp_start;
    float ramp_end;
    float ramp_y0;
    float ramp_slope;
} Segment;

static Segment segments[MAX_SEGMENTS];
static int segment_count;
static unsigned current_seed;
static unsigned rng_state;
static int terrain_ready;

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static unsigned next_random(void)
{
    /* xorshift32：同样的种子永远产生同样的路线。 */
    unsigned x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (x == 0) x = 0x6D2B79F5u;
    rng_state = x;
    return x;
}

static float random01(void)
{
    return (float)(next_random() >> 8) * (1.0f / 16777216.0f);
}

static float random_between(float low, float high)
{
    return low + (high - low) * random01();
}

static int add_segment(int type, float start, float end, float start_y,
                       float slope, int has_ground)
{
    Segment *segment;

    if (segment_count >= MAX_SEGMENTS || end <= start) return 0;

    segment = &segments[segment_count];
    segment->type = type;
    segment->start = start;
    segment->end = end;
    segment->start_y = start_y;
    segment->slope = slope;
    segment->has_ground = has_ground;
    segment->has_ramp = 0;
    segment->ramp_start = 0.0f;
    segment->ramp_end = 0.0f;
    segment->ramp_y0 = 0.0f;
    segment->ramp_slope = 0.0f;
    segment_count += 1;
    return 1;
}

static void add_fixed_route(void)
{
    Segment *slope;

    add_segment(SEG_FLAT, 0.0f, 300.0f, 250.0f, 0.0f, 1);
    add_segment(SEG_SLOPE, 300.0f, 700.0f, 250.0f, 0.25f, 1);
    slope = &segments[segment_count - 1];
    slope->has_ramp = 1;
    slope->ramp_start = 560.0f;
    slope->ramp_end = 680.0f;
    slope->ramp_y0 = 315.0f;
    slope->ramp_slope = -0.25f;

    add_segment(SEG_GAP, 700.0f, 750.0f, 350.0f, 0.0f, 0);
    add_segment(SEG_LANDING, 750.0f, 1150.0f, 345.0f, 0.125f, 1);
    add_segment(SEG_FLAT, 1150.0f, WORLD_END, 395.0f, 0.0f, 1);
}

static void add_random_route(unsigned seed)
{
    float distance = 0.0f;
    float y = 250.0f;
    float last_gap_end = -10000.0f;

    rng_state = seed ? seed : 1u;

    /* 开头留一段安全区，第一处随机内容不会贴脸出现。 */
    add_segment(SEG_FLAT, 0.0f, 300.0f, y, 0.0f, 1);
    add_segment(SEG_SLOPE, 300.0f, 650.0f, y, 0.18f, 1);
    y = 250.0f + 350.0f * 0.18f;
    add_segment(SEG_FLAT, 650.0f, FIRST_RANDOM_START, y, 0.0f, 1);
    distance = FIRST_RANDOM_START;

    /* 预留两格：沟 + 落坡；循环后再补最后一段长平地。 */
    while (segment_count < MAX_SEGMENTS - 2 && distance < 15000.0f) {
        int pick = (int)(next_random() % 10u);
        float length;
        float slope;
        float end_y;

        /* 上一跳可能飞出很远才落地。沟后留足恢复跑道，避免第二条沟
           正好落在落点上，形成无法操作的必死组合。 */
        if (pick >= 8 && distance - last_gap_end < MIN_RUNWAY_AFTER_GAP) {
            pick = (int)(next_random() % 8u);
        }

        if (pick < 4) {
            /* 平地：给金币和障碍留出可读的落脚空间。 */
            length = random_between(120.0f, 280.0f);
            if (!add_segment(SEG_FLAT, distance, distance + length, y, 0.0f, 1)) break;
            distance += length;
        } else if (pick < 8) {
            /* 坡度：靠近上下边界时自动改方向，避免雪面钻出画面。 */
            float direction;
            length = random_between(180.0f, 320.0f);
            if (y > 355.0f) direction = -1.0f;
            else if (y < 255.0f) direction = 1.0f;
            else direction = (next_random() & 1u) ? 1.0f : -1.0f;

            slope = direction * random_between(0.10f, 0.22f);
            end_y = y + length * slope;
            if (end_y < MIN_GROUND_Y || end_y > MAX_GROUND_Y) {
                slope = -slope;
                end_y = y + length * slope;
            }
            /* 翻转方向后仍越界时，把坡度压到刚好落在边界上，
               保证下一段从上一段的真实终点接上，不产生裂缝。 */
            if (end_y < MIN_GROUND_Y) {
                slope = (MIN_GROUND_Y - y) / length;
                end_y = MIN_GROUND_Y;
            } else if (end_y > MAX_GROUND_Y) {
                slope = (MAX_GROUND_Y - y) / length;
                end_y = MAX_GROUND_Y;
            }
            if (!add_segment(SEG_SLOPE, distance, distance + length, y, slope, 1)) break;
            y = end_y;
            distance += length;
        } else {
            /* 沟：宽度按最低速度的 36 帧滞空时间限制。 */
            float gap = random_between(48.0f, 92.0f);
            float drop;
            float landing_y;
            float landing_length;
            float landing_slope;

            if (!terrain_gap_is_passable(gap, PLAYER_MIN_SPEED)) {
                gap = MAX_SAFE_GAP;
            }
            if (!add_segment(SEG_GAP, distance, distance + gap, y, 0.0f, 0)) break;
            distance += gap;

            drop = random_between(30.0f, 70.0f);
            landing_y = clampf(y + drop, 250.0f, MAX_GROUND_Y);
            landing_length = random_between(100.0f, 170.0f);
            landing_slope = random_between(0.06f, 0.14f);
            if (landing_y + landing_length * landing_slope > MAX_GROUND_Y) {
                landing_slope = (MAX_GROUND_Y - landing_y) / landing_length;
            }
            if (!add_segment(SEG_LANDING, distance, distance + landing_length,
                             landing_y, landing_slope, 1)) break;
            y = clampf(landing_y + landing_length * landing_slope,
                       MIN_GROUND_Y, MAX_GROUND_Y);
            distance += landing_length;
            last_gap_end = distance;
        }
    }

    if (segment_count < MAX_SEGMENTS) {
        add_segment(SEG_FLAT, distance, WORLD_END, y, 0.0f, 1);
    }
}

static const Segment *segment_at(float distance)
{
    int low;
    int high;
    int best;

    if (segment_count <= 0) return 0;
    if (distance <= segments[0].start) return &segments[0];

    low = 0;
    high = segment_count - 1;
    best = 0;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (segments[mid].start <= distance) {
            best = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return &segments[best];
}

static void ensure_initialized(void)
{
    if (!terrain_ready) terrain_init(0);
}

void terrain_init(unsigned seed)
{
    segment_count = 0;
    current_seed = seed;
    terrain_ready = 1;

    if (seed == 0) {
        add_fixed_route();
    } else {
        add_random_route(seed);
    }
}

unsigned terrain_seed(void)
{
    return current_seed;
}

int terrain_gap_is_passable(float width, float speed)
{
    /* 36 帧而不是理论上的 40 帧，留出起跳和落地的操作余量。 */
    if (width < 0.0f || speed <= 0.0f) return 0;
    return width <= speed * SAFE_JUMP_FRAMES;
}

Ground ground_at(float distance)
{
    const Segment *segment;
    Ground ground = {0, 0.0f, 0.0f};

    ensure_initialized();
    segment = segment_at(distance);
    if (segment == 0) return ground;

    if (!segment->has_ground) {
        ground.has_ground = 0;
        ground.y = segment->start_y;
        return ground;
    }

    ground.has_ground = 1;
    ground.y = segment->start_y + (distance - segment->start) * segment->slope;
    ground.slope = segment->slope;
    return ground;
}

Ground ramp_at(float distance)
{
    Ground ramp = {0, 0.0f, 0.0f};
    int i;

    ensure_initialized();
    for (i = 0; i < segment_count; ++i) {
        const Segment *segment = &segments[i];
        if (segment->has_ramp &&
            distance >= segment->ramp_start && distance < segment->ramp_end) {
            ramp.has_ground = 1;
            ramp.y = segment->ramp_y0 +
                     (distance - segment->ramp_start) * segment->ramp_slope;
            ramp.slope = segment->ramp_slope;
            return ramp;
        }
    }
    return ramp;
}

float terrain_safe_distance(void)
{
    int i;

    ensure_initialized();
    for (i = 0; i < segment_count; ++i) {
        const Segment *segment = &segments[i];
        if (segment->has_ground && segment->start >= 600.0f &&
            segment->end - segment->start >= 100.0f) {
            return segment->start + 30.0f;
        }
    }
    return 0.0f;
}
