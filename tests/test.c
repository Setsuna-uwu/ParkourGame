#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef SP_TEST_FRAMEWORK_H
#define SP_TEST_FRAMEWORK_H

extern int tf_checks;
extern int tf_failures;

#define TF_CHECK(condition)                                                          \
    do {                                                                             \
        tf_checks += 1;                                                              \
        if (!(condition)) {                                                          \
            tf_failures += 1;                                                        \
            printf("  FAIL  %s:%d  %s\n", __FILE__, __LINE__, #condition);           \
        }                                                                            \
    } while (0)

#define TF_CHECK_NEAR(actual, expected, tolerance)                                   \
    do {                                                                             \
        float tf_a_ = (float)(actual);                                               \
        float tf_e_ = (float)(expected);                                             \
        float tf_t_ = (float)(tolerance);                                            \
        tf_checks += 1;                                                              \
        if (fabsf(tf_a_ - tf_e_) > tf_t_) {                                          \
            tf_failures += 1;                                                        \
            printf("  FAIL  %s:%d  %s == %.6f, expected %.6f (+/- %.6f)\n",          \
                   __FILE__, __LINE__, #actual, (double)tf_a_, (double)tf_e_,         \
                   (double)tf_t_);                                                   \
        }                                                                            \
    } while (0)

#define TF_CHECK_INT(actual, expected)                                               \
    do {                                                                             \
        long long tf_a_ = (long long)(actual);                                       \
        long long tf_e_ = (long long)(expected);                                     \
        tf_checks += 1;                                                              \
        if (tf_a_ != tf_e_) {                                                        \
            tf_failures += 1;                                                        \
            printf("  FAIL  %s:%d  %s == %lld, expected %lld\n",                     \
                   __FILE__, __LINE__, #actual, tf_a_, tf_e_);                       \
        }                                                                            \
    } while (0)

void test_physics_suite(void);
void test_collision_suite(void);
void test_terrain_suite(void);
void test_difficulty_suite(void);
void test_score_suite(void);
void test_game_suite(void);

#endif /* SP_TEST_FRAMEWORK_H */

int tf_checks = 0;
int tf_failures = 0;

typedef struct TestSuite {
    const char *name;
    void (*function)(void);
} TestSuite;

int main(int argc, char **argv)
{
    static const TestSuite suites[] = {
        {"physics", test_physics_suite},
        {"collision", test_collision_suite},
        {"terrain", test_terrain_suite},
        {"difficulty", test_difficulty_suite},
        {"score", test_score_suite},
        {"game", test_game_suite}
    };
    const char *filter = (argc > 1) ? argv[1] : 0;
    int matched = 0;
    int i;
    int failures_before;
    int checks_before;

    printf("SnowParkour core tests | %s\n", filter ? filter : "all");
    printf("------------------------------------------------------------\n");

    for (i = 0; i < (int)(sizeof(suites) / sizeof(suites[0])); ++i) {
        if (filter != 0 && strstr(suites[i].name, filter) == 0) {
            continue;
        }
        matched += 1;
        checks_before = tf_checks;
        failures_before = tf_failures;
        printf("[ RUN  ] %s\n", suites[i].name);
        suites[i].function();
        printf("[ %s ] %s  (%d checks, %d failures)\n",
               tf_failures == failures_before ? " OK " : "FAIL",
               suites[i].name,
               tf_checks - checks_before,
               tf_failures - failures_before);
    }

    printf("------------------------------------------------------------\n");
    if (matched == 0) {
        printf("No suite matched filter '%s'.\n", filter);
        return 2;
    }
    printf("RESULT: %d checks, %d failures\n", tf_checks, tf_failures);
    return (tf_failures == 0) ? 0 : 1;
}

void test_physics_suite(void)
{
    Player player;
    Input input;
    int i;

    player = starting_player(100.0f, SP_GROUND_Y);
    TF_CHECK(player.on_ground);
    TF_CHECK_INT(player.jumps_used, 0);
    TF_CHECK_NEAR(player.y, SP_GROUND_Y - SP_PLAYER_H, 0.01f);

    input_reset(&input);
    input.jump_pressed = true;
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK(!player.on_ground);
    TF_CHECK(player.vy < 0.0f);
    TF_CHECK_INT(player.jumps_used, 1);

    input.jump_pressed = false;
    for (i = 0; i < 14; ++i) {
        player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    }

    input.jump_pressed = true;
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK_INT(player.jumps_used, 2);
    TF_CHECK(player.vy < 0.0f);

    input.jump_pressed = true;
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK_INT(player.jumps_used, 2);

    input_reset(&input);
    for (i = 0; i < 300 && !player.on_ground; ++i) {
        player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    }
    TF_CHECK(player.on_ground);
    TF_CHECK_INT(player.jumps_used, 0);

    /* 落地前按下跳跃：缓冲不应浪费，落地后的下一次更新应起跳。 */
    player = starting_player(100.0f, SP_GROUND_Y);
    player.on_ground = false;
    player.y = SP_GROUND_Y - SP_PLAYER_H - 5.0f;
    player.vy = 0.0f;
    player.jumps_used = SP_MAX_JUMPS;
    player.coyote_timer = 0.0f;

    input_reset(&input);
    input.jump_pressed = true;
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    input.jump_pressed = false;
    for (i = 0; i < 20 && !player.on_ground; ++i) {
        player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    }
    TF_CHECK(player.on_ground);
    TF_CHECK(player.jump_buffer_timer > 0.0f);
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK(!player.on_ground);
    TF_CHECK_INT(player.jumps_used, 1);

    /* 土狼时间：刚离开地面仍可起跳。 */
    player = starting_player(100.0f, SP_GROUND_Y);
    player.on_ground = false;
    player.y = SP_GROUND_Y - SP_PLAYER_H - 2.0f;
    player.vy = 0.0f;
    player.coyote_timer = 0.08f;
    player.jumps_used = 0;
    input_reset(&input);
    input.jump_pressed = true;
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK_INT(player.jumps_used, 1);
    TF_CHECK(player.vy < 0.0f);

    /* 下落速度有上限。 */
    player = starting_player(100.0f, SP_GROUND_Y);
    player.on_ground = false;
    player.y = 0.0f;
    player.vy = 9000.0f;
    input_reset(&input);
    player = move_player(player, input, SP_GROUND_Y, SP_FIXED_DT);
    TF_CHECK(player.vy <= SP_MAX_FALL_SPEED + 0.01f);

    /* dt 为 0 时状态不变。 */
    player = starting_player(100.0f, SP_GROUND_Y);
    player = move_player(player, input, SP_GROUND_Y, 0.0f);
    TF_CHECK(player.on_ground);
    TF_CHECK_NEAR(player.y, SP_GROUND_Y - SP_PLAYER_H, 0.01f);
}

void test_collision_suite(void)
{
    Aabb a;
    Aabb b;
    Player player;
    Terrain terrain;
    CollisionResult result;
    Obstacle *obstacle;

    a.x = 10.0f;
    a.y = 10.0f;
    a.w = 20.0f;
    a.h = 20.0f;
    b = a;
    TF_CHECK(collision_aabb_overlap(a, b));

    b.x = 30.0f;
    TF_CHECK(!collision_aabb_overlap(a, b));
    b.x = 29.9f;
    TF_CHECK(collision_aabb_overlap(a, b));

    b = a;
    b.y = 30.0f;
    TF_CHECK(!collision_aabb_overlap(a, b));

    b = a;
    b.w = 0.0f;
    TF_CHECK(!collision_aabb_overlap(a, b));

    player = starting_player(100.0f, SP_GROUND_Y);
    memset(&terrain, 0, sizeof(terrain));
    terrain.ground_y = SP_GROUND_Y;
    terrain.segment_count = 1;
    terrain.segments[0].x = 0.0f;
    terrain.segments[0].width = 400.0f;
    terrain.segments[0].obstacle_count = 1;

    obstacle = &terrain.segments[0].obstacles[0];
    obstacle->x = 120.0f;
    obstacle->y = 550.0f;
    obstacle->w = 40.0f;
    obstacle->h = 62.0f;
    obstacle->type = OBSTACLE_CRATE;

    TF_CHECK(collision_check_player(&player, &terrain, &result));
    TF_CHECK(result.hit);
    TF_CHECK_INT(result.segment_index, 0);
    TF_CHECK_INT(result.obstacle_index, 0);

    /* 障碍左边缘等于玩家碰撞盒右边缘时不算碰撞。 */
    obstacle->x = collision_player_box(&player).x + collision_player_box(&player).w;
    TF_CHECK(!collision_check_player(&player, &terrain, &result));

    obstacle->x = collision_player_box(&player).x + collision_player_box(&player).w - 0.5f;
    TF_CHECK(collision_check_player(&player, &terrain, &result));

    /* 冰锥的判定框向内收缩。 */
    obstacle->x = 100.0f;
    obstacle->y = SP_GROUND_Y - 42.0f;
    obstacle->w = 34.0f;
    obstacle->h = 42.0f;
    obstacle->type = OBSTACLE_SPIKE;
    a = collision_obstacle_box(obstacle);
    TF_CHECK_NEAR(a.x, 105.0f, 0.01f);
    TF_CHECK_NEAR(a.w, 24.0f, 0.01f);

    terrain.segment_count = 0;
    TF_CHECK(!collision_check_player(&player, &terrain, &result));
    TF_CHECK(!result.hit);
    TF_CHECK_INT(result.segment_index, -1);

    TF_CHECK(!collision_check_player(0, &terrain, 0));
    TF_CHECK(!collision_check_player(&player, 0, 0));
}

void test_difficulty_suite(void)
{
    DifficultyParams start;
    DifficultyParams middle;
    DifficultyParams end;
    DifficultyParams clamped_low;
    DifficultyParams clamped_high;

    start = difficulty_at(0.0f);
    middle = difficulty_at(60.0f);
    end = difficulty_at(180.0f);
    clamped_low = difficulty_at(-25.0f);
    clamped_high = difficulty_at(9999.0f);

    TF_CHECK(start.speed > 0.0f);
    TF_CHECK(middle.speed > start.speed);
    TF_CHECK(end.speed > middle.speed);
    TF_CHECK(end.speed < 550.0f);

    TF_CHECK(middle.gap < start.gap);
    TF_CHECK(end.gap < middle.gap);
    TF_CHECK(end.gap >= 350.0f);

    TF_CHECK(middle.hard_weight > start.hard_weight);
    TF_CHECK(end.hard_weight > middle.hard_weight);
    TF_CHECK_NEAR(end.hard_weight, 1.0f, 0.001f);

    TF_CHECK_INT(start.level, 1);
    TF_CHECK(middle.level > start.level);
    TF_CHECK(end.level <= 10);
    TF_CHECK(end.level > middle.level);

    TF_CHECK_NEAR(clamped_low.speed, start.speed, 0.001f);
    TF_CHECK_NEAR(clamped_low.gap, start.gap, 0.001f);
    TF_CHECK_NEAR(clamped_high.speed, end.speed, 0.001f);
    TF_CHECK_NEAR(clamped_high.gap, end.gap, 0.001f);

    TF_CHECK(isfinite(start.speed));
    TF_CHECK(isfinite(end.speed));
    TF_CHECK(difficulty_level_name(1) != 0);
    TF_CHECK(difficulty_level_name(10) != 0);
}

static int terrain_matches(const Terrain *a, const Terrain *b)
{
    int i;
    int j;

    if (a->segment_count != b->segment_count || a->generated != b->generated ||
        a->rng.state != b->rng.state) {
        return 0;
    }
    for (i = 0; i < a->segment_count; ++i) {
        const TerrainSegment *sa = &a->segments[i];
        const TerrainSegment *sb = &b->segments[i];
        if (sa->pattern != sb->pattern || sa->obstacle_count != sb->obstacle_count ||
            fabsf(sa->x - sb->x) > 0.001f || fabsf(sa->width - sb->width) > 0.001f) {
            return 0;
        }
        for (j = 0; j < sa->obstacle_count; ++j) {
            const Obstacle *oa = &sa->obstacles[j];
            const Obstacle *ob = &sb->obstacles[j];
            if (oa->type != ob->type || fabsf(oa->x - ob->x) > 0.001f ||
                fabsf(oa->y - ob->y) > 0.001f || fabsf(oa->w - ob->w) > 0.001f ||
                fabsf(oa->h - ob->h) > 0.001f) {
                return 0;
            }
        }
    }
    return 1;
}

void test_terrain_suite(void)
{
    Terrain terrain;
    Terrain copy;
    DifficultyParams difficulty;
    int i;
    int j;

    difficulty_init(&difficulty);
    terrain_init(&terrain, 12345u, SP_GROUND_Y, &difficulty);

    TF_CHECK(terrain.segment_count > 0);
    TF_CHECK(terrain.segment_count <= SP_TERRAIN_MAX_SEGMENTS);
    TF_CHECK(terrain.generated > 0u);
    TF_CHECK(terrain.next_x > (float)SP_SCREEN_WIDTH);
    TF_CHECK(terrain.segments[0].obstacles[0].x > SP_PLAYER_START_X + 600.0f);
    TF_CHECK(terrain_obstacle_count(&terrain) > 0);

    for (i = 0; i < terrain.segment_count; ++i) {
        const TerrainSegment *segment = &terrain.segments[i];
        TF_CHECK(segment->obstacle_count > 0);
        TF_CHECK(segment->obstacle_count <= SP_TERRAIN_MAX_OBSTACLES);
        for (j = 0; j < segment->obstacle_count; ++j) {
            const Obstacle *obstacle = &segment->obstacles[j];
            TF_CHECK(obstacle->x >= segment->x - 0.01f);
            TF_CHECK(obstacle->x + obstacle->w <= segment->x + segment->width + 0.01f);
            TF_CHECK_NEAR(obstacle->y + obstacle->h, SP_GROUND_Y, 0.01f);
        }
    }

    for (i = 1; i < terrain.segment_count; ++i) {
        const TerrainSegment *previous = &terrain.segments[i - 1];
        const TerrainSegment *current = &terrain.segments[i];
        const Obstacle *last = &previous->obstacles[previous->obstacle_count - 1];
        const Obstacle *first = &current->obstacles[0];
        TF_CHECK(first->x - (last->x + last->w) >= 250.0f);
    }

    /* 同种子 + 同样步数，得到完全一致的地形。 */
    terrain_init(&copy, 12345u, SP_GROUND_Y, &difficulty);
    TF_CHECK(terrain_matches(&terrain, &copy));

    /* 移动 5000px，确认回收后数量受控且仍然一致。 */
    copy = terrain;
    for (i = 0; i < 300; ++i) {
        float t = (float)i * SP_FIXED_DT;
        DifficultyParams current = difficulty_at(t);
        terrain_scroll(&terrain, current.speed * SP_FIXED_DT, &current);
        terrain_scroll(&copy, current.speed * SP_FIXED_DT, &current);
        TF_CHECK(terrain.segment_count > 0);
        TF_CHECK(terrain.segment_count <= SP_TERRAIN_MAX_SEGMENTS);
    }
    TF_CHECK(terrain_matches(&terrain, &copy));

    for (i = 0; i < terrain.segment_count; ++i) {
        const TerrainSegment *segment = &terrain.segments[i];
        TF_CHECK(segment->x + segment->width >= SP_TERRAIN_RECYCLE_X - 1.0f);
        for (j = 0; j < segment->obstacle_count; ++j) {
            const Obstacle *obstacle = &segment->obstacles[j];
            TF_CHECK(obstacle->x >= segment->x - 0.01f);
            TF_CHECK(obstacle->x + obstacle->w <= segment->x + segment->width + 0.01f);
        }
    }

    /* 障碍物必须与地形段同步滚动，而不是留在原地。 */
    {
        float before_segment;
        float before_obstacle;
        terrain_init(&terrain, 5150u, SP_GROUND_Y, &difficulty);
        before_segment = terrain.segments[0].x;
        before_obstacle = terrain.segments[0].obstacles[0].x;
        terrain_scroll(&terrain, 100.0f, &difficulty);
        TF_CHECK_NEAR(terrain.segments[0].x, before_segment - 100.0f, 0.01f);
        TF_CHECK_NEAR(terrain.segments[0].obstacles[0].x, before_obstacle - 100.0f, 0.01f);
    }

    /* 不同种子应产生不同模板序列。 */
    {
        Terrain other;
        terrain_init(&other, 987654u, SP_GROUND_Y, &difficulty);
        TF_CHECK(!terrain_matches(&terrain, &other));
    }

    /* 长局滚动，覆盖生成/回收的稳定性。 */
    difficulty_init(&difficulty);
    terrain_init(&terrain, 777u, SP_GROUND_Y, &difficulty);
    for (i = 0; i < 7200; ++i) {
        DifficultyParams current = difficulty_at((float)i * SP_FIXED_DT);
        terrain_scroll(&terrain, current.speed * SP_FIXED_DT, &current);
    }
    TF_CHECK(terrain.segment_count > 0);
    TF_CHECK(terrain.segment_count < SP_TERRAIN_MAX_SEGMENTS);
    TF_CHECK(terrain.generated > 100u);
    TF_CHECK(terrain.next_x > (float)SP_SCREEN_WIDTH);
    TF_CHECK(terrain_obstacle_count(&terrain) > 0);
}

void test_score_suite(void)
{
    ScoreState score;

    score.current = 0;
    score.high_score = 0;
    score.distance_px = 0.0f;
    score.distance_m = 0.0f;
    score.new_record = false;
    score.high_score_dirty = false;

    score_update(&score, 0.0f, 0.0f);
    TF_CHECK_INT(score.current, 0);

    /* 4500px = 100m，10 秒 = 60 分。 */
    score_update(&score, 4500.0f, 10.0f);
    TF_CHECK_NEAR(score.distance_m, 100.0f, 0.01f);
    TF_CHECK_INT(score.current, 160);

    score.high_score = 500;
    score_update(&score, 4500.0f, 10.0f);
    score_finish(&score);
    TF_CHECK_INT(score.high_score, 500);
    TF_CHECK(!score.new_record);
    TF_CHECK(!score.high_score_dirty);

    score.high_score = 100;
    score_update(&score, 9000.0f, 20.0f);
    score_finish(&score);
    TF_CHECK(score.new_record);
    TF_CHECK(score.high_score_dirty);
    TF_CHECK_INT(score.high_score, score.current);
    TF_CHECK(score.current > 100);

    {
        int preserved_high_score = score.high_score;
        score_reset(&score);
        TF_CHECK_INT(score.current, 0);
        TF_CHECK_INT(score.high_score, preserved_high_score);
        TF_CHECK(score.high_score_dirty);
    }

    score_finish(&score);
    TF_CHECK(!score.new_record);

    score_set_high_score(&score, -5);
    TF_CHECK_INT(score.high_score, 0);
    score_set_high_score(&score, SP_HIGH_SCORE_LIMIT + 10);
    TF_CHECK_INT(score.high_score, SP_HIGH_SCORE_LIMIT);
}

static int nearest_obstacle_dx(const GameState *state, float *out_dx, float *out_width,
                               float *out_height)
{
    const Player *player = &state->player;
    float best = 1000000.0f;
    int best_segment = -1;
    int found = 0;
    int i;
    int j;

    for (i = 0; i < state->terrain.segment_count; ++i) {
        const TerrainSegment *segment = &state->terrain.segments[i];
        for (j = 0; j < segment->obstacle_count; ++j) {
            const Obstacle *obstacle = &segment->obstacles[j];
            float dx;
            if (obstacle->x + obstacle->w <= player->x) {
                continue;
            }
            dx = obstacle->x - (player->x + SP_PLAYER_W);
            if (dx < best) {
                best = dx;
                best_segment = i;
                found = 1;
            }
        }
    }
    if (found) {
        const TerrainSegment *segment = &state->terrain.segments[best_segment];
        float nearest_x = player->x + SP_PLAYER_W + best;
        float rightmost = nearest_x;
        float highest = 0.0f;
        for (j = 0; j < segment->obstacle_count; ++j) {
            float right = segment->obstacles[j].x + segment->obstacles[j].w;
            if (right > rightmost) {
                rightmost = right;
            }
            if (segment->obstacles[j].h > highest) {
                highest = segment->obstacles[j].h;
            }
        }
        *out_dx = best;
        *out_width = rightmost - nearest_x;
        *out_height = highest;
    }
    return found;
}

static void run_autoplay(GameState *state, int total_steps, int *steps_run)
{
    int i;
    int jump_count = 0;

    for (i = 0; i < total_steps; ++i) {
        Input input;
        float dx = 0.0f;
        float obstacle_width = 0.0f;
        float obstacle_height = 0.0f;

        input_reset(&input);
        if (nearest_obstacle_dx(state, &dx, &obstacle_width, &obstacle_height)) {
            const float jump_speed = 880.0f;
            const float gravity = 2100.0f;
            float target_height = obstacle_height + 12.0f;
            float discriminant = jump_speed * jump_speed - 2.0f * gravity * target_height;
            float t_fall = (2.0f * jump_speed) / gravity;
            float player_box_width = SP_PLAYER_W - 10.0f;
            float max_jump_dx;

            if (discriminant > 0.0f) {
                t_fall = (jump_speed + sqrtf(discriminant)) / gravity;
            }
            t_fall -= 0.04f;
            if (t_fall < 0.08f) {
                t_fall = 0.08f;
            }
            max_jump_dx = state->world_speed * t_fall - player_box_width - obstacle_width - 8.0f;
            if (max_jump_dx < 45.0f) {
                max_jump_dx = 45.0f;
            }
            if (state->player.on_ground && dx > 10.0f && dx < max_jump_dx) {
                input.jump_pressed = true;
                jump_count += 1;
            }
        }
        game_update(input);
        if (state->phase == GAME_PHASE_GAME_OVER) {
            int k;
            printf("  death detail: t=%.2f jumps=%d py=%.1f vy=%.1f nearest=%.1f jumps_used=%d\n",
                   (double)state->elapsed_time, jump_count, (double)state->player.y,
                   (double)state->player.vy, (double)dx, state->player.jumps_used);
            for (k = 0; k < state->terrain.segment_count; ++k) {
                const TerrainSegment *sg = &state->terrain.segments[k];
                int m;
                printf("    seg[%d] x=%.1f w=%.1f obs=%d", k, (double)sg->x,
                       (double)sg->width, sg->obstacle_count);
                for (m = 0; m < sg->obstacle_count; ++m) {
                    printf(" (%d x=%.1f w=%.1f h=%.1f)", sg->obstacles[m].type,
                           (double)sg->obstacles[m].x, (double)sg->obstacles[m].w,
                           (double)sg->obstacles[m].h);
                }
                printf("\n");
            }
            break;
        }
    }
    if (steps_run != 0) {
        *steps_run = i;
    }
}

void test_game_suite(void)
{

    Input input;
    int i;

    game_init(0x5EED2026u);
    TF_CHECK(game.phase == GAME_PHASE_PLAYING);
    TF_CHECK(game.player.on_ground);
    TF_CHECK(game.terrain.segment_count > 0);
    TF_CHECK_INT(game.run_index, 1u);
    TF_CHECK_NEAR(game.player.y, SP_GROUND_Y - SP_PLAYER_H, 0.01f);

    /* 暂停期间时间与距离冻结。 */
    input_reset(&input);
    input.pause_pressed = true;
    game_update(input);
    TF_CHECK(game.phase == GAME_PHASE_PAUSED);
    {
        float frozen_time = game.elapsed_time;
        float frozen_distance = game.distance_px;
        input_reset(&input);
        for (i = 0; i < 30; ++i) {
            game_update(input);
        }
        TF_CHECK_NEAR(game.elapsed_time, frozen_time, 0.0001f);
        TF_CHECK_NEAR(game.distance_px, frozen_distance, 0.0001f);
    }
    input_reset(&input);
    input.pause_pressed = true;
    game_update(input);
    TF_CHECK(game.phase == GAME_PHASE_PLAYING);

    /* F1 调试开关。 */
    input_reset(&input);
    input.debug_pressed = true;
    game_update(input);
    TF_CHECK(game.debug_hud);
    input_reset(&input);
    input.debug_pressed = true;
    game_update(input);
    TF_CHECK(!game.debug_hud);

    /* 无输入必撞上第一个障碍。 */
    game_init(42u);
    input_reset(&input);
    for (i = 0; i < 900 && game.phase == GAME_PHASE_PLAYING; ++i) {
        game_update(input);
    }
    TF_CHECK(game.phase == GAME_PHASE_GAME_OVER);
    TF_CHECK(game.score.high_score > 0);
    TF_CHECK(game.score.high_score_dirty);
    TF_CHECK(game.score.new_record);
    TF_CHECK(game.death_time >= 0.0f);

    {
        int saved_high_score = game.score.high_score;
        uint32_t saved_run = game.run_index;
        input_reset(&input);
        game_update(input);
        TF_CHECK(game.death_time > 0.0f);
        game_restart();
        TF_CHECK(game.phase == GAME_PHASE_PLAYING);
        TF_CHECK_INT(game.score.current, 0);
        TF_CHECK_INT(game.score.high_score, saved_high_score);
        TF_CHECK(game.run_index > saved_run);
        TF_CHECK(game.player.on_ground);
    }

    /* ESC 退出请求。 */
    input_reset(&input);
    input.quit_pressed = true;
    game_update(input);
    TF_CHECK(game.quit_requested);

    /* 同种子、同输入 -> 同结果。 */
    {
        GameState a;
        GameState b;
        input_reset(&input);
        game_init(0xABCDEF01u);
        for (i = 0; i < 600; ++i) {
            game_update(input);
        }
        a = game;
        game_init(0xABCDEF01u);
        for (i = 0; i < 600; ++i) {
            game_update(input);
        }
        b = game;
        TF_CHECK(a.phase == b.phase);
        TF_CHECK_INT(a.fixed_steps, b.fixed_steps);
        TF_CHECK_INT(a.collision_segment, b.collision_segment);
        TF_CHECK_INT(a.collision_obstacle, b.collision_obstacle);
        TF_CHECK_NEAR(a.elapsed_time, b.elapsed_time, 0.0001f);
        TF_CHECK_NEAR(a.player.y, b.player.y, 0.0001f);
        TF_CHECK_INT(a.score.current, b.score.current);
        TF_CHECK_INT(a.seed, b.seed);
        TF_CHECK_INT(a.terrain.segment_count, b.terrain.segment_count);
        for (i = 0; i < a.terrain.segment_count; ++i) {
            TF_CHECK_NEAR(a.terrain.segments[i].x, b.terrain.segments[i].x, 0.0001f);
            TF_CHECK_INT(a.terrain.segments[i].pattern, b.terrain.segments[i].pattern);
        }
    }

    /* 自动玩家连续跑 120 秒，验证地形可玩性与长局稳定性。 */
    {
        int steps = 0;
        game_init(0xBEEF1234u);
        run_autoplay(&game, SP_TARGET_FPS * 120, &steps);
        if (game.phase == GAME_PHASE_GAME_OVER) {
            printf("  autoplay died at %.2fs (step %d), score %d\n",
                   (double)game.elapsed_time, steps, game.score.current);
        }
        TF_CHECK(game.phase == GAME_PHASE_PLAYING);
        TF_CHECK(game.elapsed_time >= 119.9f);
        TF_CHECK(game.score.current > 1000);
        TF_CHECK(game.difficulty.speed > 480.0f);
        TF_CHECK_INT(game.difficulty.level, 7);
    }
}
