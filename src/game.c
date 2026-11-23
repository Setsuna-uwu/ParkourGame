#include "game.h"
#include <math.h>
#include <string.h>

GameState game;

static void game_start_run(void)
{
    game.seed = game.base_seed + game.run_index * 0x9E3779B9u;
    game.run_index += 1u;

    game.phase = GAME_PHASE_PLAYING;
    game.elapsed_time = 0.0f;
    game.distance_px = 0.0f;
    game.death_time = 0.0f;
    game.fixed_steps = 0u;
    game.collision_segment = -1;
    game.collision_obstacle = -1;
    game.last_hit_type = -1;
    game.quit_requested = false;

    difficulty_init(&game.difficulty);
    game.world_speed = game.difficulty.speed;

    score_reset(&game.score);
    game.player = starting_player(SP_PLAYER_START_X, game.ground_y);
    terrain_init(&game.terrain, game.seed, game.ground_y, &game.difficulty);
    input_reset(&game.last_input);
}

void game_init(uint32_t seed)
{
    memset(&game, 0, sizeof(game));
    game.base_seed = seed;
    game.run_index = 0u;
    game.screen_width = SP_SCREEN_WIDTH;
    game.screen_height = SP_SCREEN_HEIGHT;
    game.ground_y = SP_GROUND_Y;
    game.debug_hud = false;
    game.quit_requested = false;
    game.last_hit_type = -1;

    game_start_run();
}

void game_restart(void)
{
    /* 开新局只重置本局数据，历史最高分和调试开关继续沿用。 */
    game_start_run();
}

void game_update(Input keys)
{
    CollisionResult collision;
    float dx;

    game.last_input = keys;

#if SP_DEBUG_HUD
    if (keys.debug_pressed) {
        game.debug_hud = !game.debug_hud;
    }
#endif
    if (keys.quit_pressed) {
        game.quit_requested = true;
        return;
    }

    if (game.phase == GAME_PHASE_GAME_OVER) {
        game.death_time += SP_FIXED_DT;
        if (keys.restart_pressed || keys.mouse_clicked) {
            game_restart();
        }
        return;
    }

    if (keys.pause_pressed) {
        if (game.phase == GAME_PHASE_PLAYING) {
            game.phase = GAME_PHASE_PAUSED;
        } else if (game.phase == GAME_PHASE_PAUSED) {
            game.phase = GAME_PHASE_PLAYING;
        }
        return;
    }

    if (game.phase != GAME_PHASE_PLAYING) {
        return;
    }

    game.elapsed_time += SP_FIXED_DT;
    game.difficulty = difficulty_at(game.elapsed_time);
    game.world_speed = game.difficulty.speed;

    /* 跑者的横坐标固定，每一步把整条路向左移。 */
    dx = game.world_speed * SP_FIXED_DT;
    game.distance_px += dx;
    terrain_scroll(&game.terrain, dx, &game.difficulty);

    game.player = move_player(game.player, keys, game.ground_y, SP_FIXED_DT);
    game.player.run_phase += dx * 0.045f;

    score_update(&game.score, game.distance_px, game.elapsed_time);

    collision.hit = false;
    collision.segment_index = -1;
    collision.obstacle_index = -1;
    /* 撞到障碍时结算分数，下一步会进入上面的结束分支。 */
    if (collision_check_player(&game.player, &game.terrain, &collision)) {
        game.phase = GAME_PHASE_GAME_OVER;
        game.death_time = 0.0f;
        game.collision_segment = collision.segment_index;
        game.collision_obstacle = collision.obstacle_index;
        if (collision.segment_index >= 0 && collision.obstacle_index >= 0) {
            game.last_hit_type =
                game.terrain.segments[collision.segment_index].obstacles[collision.obstacle_index].type;
        }
        score_finish(&game.score);
    }

    game.fixed_steps += 1u;
}

const char *game_phase_name(GamePhase phase)
{
    switch (phase) {
    case GAME_PHASE_PLAYING:
        return "PLAYING";
    case GAME_PHASE_PAUSED:
        return "PAUSED";
    case GAME_PHASE_GAME_OVER:
        return "GAME_OVER";
    default:
        return "UNKNOWN";
    }
}

static float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

void difficulty_init(DifficultyParams *params)
{
    if (params == 0) {
        return;
    }
    params->speed = 330.0f;
    params->gap = 470.0f;
    params->hard_weight = 0.0f;
    params->level = 1;
}

DifficultyParams difficulty_at(float survival_seconds)
{
    DifficultyParams params;
    float t;

    t = clamp_float(survival_seconds, 0.0f, 180.0f);

    /* 330 -> 约 520 px/s；前 60 秒变化最明显，之后缓慢逼近上限。 */
    params.speed = 330.0f + 190.0f * (1.0f - expf(-t / 70.0f));

    /* 像素间距逐步收紧；模板内部还有自身宽度，难度还来自类型权重。 */
    params.gap = 470.0f - 110.0f * (t / 180.0f);
    params.hard_weight = clamp_float(t / 150.0f, 0.0f, 1.0f);

    params.level = 1 + (int)(t / 20.0f);
    if (params.level > 10) {
        params.level = 10;
    }
    if (params.level < 1) {
        params.level = 1;
    }

    return params;
}

const wchar_t *difficulty_level_name(int level)
{
    switch (level) {
    case 1:
        return L"轻松";
    case 2:
        return L"热身";
    case 3:
        return L"渐快";
    case 4:
        return L"加速";
    case 5:
        return L"紧张";
    case 6:
        return L"疾驰";
    case 7:
        return L"严酷";
    case 8:
        return L"极限";
    case 9:
        return L"风暴";
    default:
        return L"雪崩";
    }
}

Player starting_player(float x, float ground_y)
{
    Player player;
    player.x = x;
    player.y = ground_y - SP_PLAYER_H;
    player.vy = 0.0f;
    player.on_ground = true;
    player.jumps_used = 0;
    player.coyote_timer = SP_COYOTE_TIME;
    player.jump_buffer_timer = 0.0f;
    player.run_phase = 0.0f;
    return player;
}

Player move_player(Player player, Input keys, float ground_y, float dt)
{
    bool wants_jump;

    if (dt <= 0.0f) {
        return player;
    }

    if (player.on_ground) {
        player.coyote_timer = SP_COYOTE_TIME;
    } else {
        player.coyote_timer = clamp_float(player.coyote_timer - dt, 0.0f, SP_COYOTE_TIME);
    }

    player.jump_buffer_timer = clamp_float(player.jump_buffer_timer - dt, 0.0f, SP_JUMP_BUFFER_TIME);
    if (keys.jump_pressed) {
        player.jump_buffer_timer = SP_JUMP_BUFFER_TIME;
    }

    wants_jump = (player.jump_buffer_timer > 0.0f);
    if (wants_jump) {
        if (player.on_ground || player.coyote_timer > 0.0f) {
            player.vy = SP_JUMP_SPEED;
            player.on_ground = false;
            player.jumps_used = 1;
            player.coyote_timer = 0.0f;
            player.jump_buffer_timer = 0.0f;
        } else if (player.jumps_used < SP_MAX_JUMPS) {
            player.vy = SP_JUMP_SPEED * 0.96f;
            player.on_ground = false;
            player.jumps_used += 1;
            player.jump_buffer_timer = 0.0f;
        }
    }

    /* 屏幕向下是正方向：起跳给负速度，重力每步把速度加回来。 */
    player.vy = clamp_float(player.vy + SP_GRAVITY * dt, SP_JUMP_SPEED, SP_MAX_FALL_SPEED);
    player.y += player.vy * dt;

    if (player.y + SP_PLAYER_H >= ground_y) {
        player.y = ground_y - SP_PLAYER_H;
        if (player.vy > 0.0f) {
            player.vy = 0.0f;
        }
        player.on_ground = true;
        player.jumps_used = 0;
        player.coyote_timer = SP_COYOTE_TIME;
    } else {
        player.on_ground = false;
    }
    return player;
}

void score_reset(ScoreState *score)
{
    if (score == 0) {
        return;
    }
    score->current = 0;
    score->distance_px = 0.0f;
    score->distance_m = 0.0f;
    score->new_record = false;
    /* high_score / high_score_dirty 属于跨局数据，保留。 */
}

void score_set_high_score(ScoreState *score, int value)
{
    if (score == 0) {
        return;
    }
    if (value < 0) {
        value = 0;
    }
    if (value > SP_HIGH_SCORE_LIMIT) {
        value = SP_HIGH_SCORE_LIMIT;
    }
    score->high_score = value;
}

void score_update(ScoreState *score, float distance_px, float elapsed_seconds)
{
    int distance_score;
    int time_score;

    if (score == 0) {
        return;
    }
    if (distance_px < 0.0f) {
        distance_px = 0.0f;
    }
    if (elapsed_seconds < 0.0f) {
        elapsed_seconds = 0.0f;
    }

    score->distance_px = distance_px;
    score->distance_m = distance_px / SP_SCORE_PIXELS_PER_METER;

    distance_score = (int)score->distance_m;
    time_score = (int)(elapsed_seconds * SP_SCORE_TIME_BONUS);
    score->current = distance_score + time_score;
    if (score->current > SP_HIGH_SCORE_LIMIT) {
        score->current = SP_HIGH_SCORE_LIMIT;
    }
}

void score_finish(ScoreState *score)
{
    if (score == 0) {
        return;
    }

    score->new_record = false;
    if (score->current > score->high_score) {
        score->high_score = score->current;
        score->new_record = true;
        score->high_score_dirty = true;
    }
}

bool collision_aabb_overlap(Aabb a, Aabb b)
{
    if (a.w <= 0.0f || a.h <= 0.0f || b.w <= 0.0f || b.h <= 0.0f) {
        return false;
    }
    return (a.x < b.x + b.w) && (a.x + a.w > b.x) &&
           (a.y < b.y + b.h) && (a.y + a.h > b.y);
}

Aabb collision_player_box(const Player *player)
{
    Aabb box = {0.0f, 0.0f, 0.0f, 0.0f};

    if (player == 0) {
        return box;
    }

    /* 留少量宽容，避免视觉上擦边却判死。 */
    box.x = player->x + 5.0f;
    box.y = player->y + 6.0f;
    box.w = SP_PLAYER_W - 10.0f;
    box.h = SP_PLAYER_H - 7.0f;
    return box;
}

Aabb collision_obstacle_box(const Obstacle *obstacle)
{
    Aabb box = {0.0f, 0.0f, 0.0f, 0.0f};

    if (obstacle == 0) {
        return box;
    }

    box.x = obstacle->x;
    box.y = obstacle->y;
    box.w = obstacle->w;
    box.h = obstacle->h;

    if (obstacle->type == OBSTACLE_SPIKE) {
        box.x += 5.0f;
        box.y += 7.0f;
        box.w -= 10.0f;
        box.h -= 7.0f;
    } else if (obstacle->type == OBSTACLE_WIDE) {
        box.x += 3.0f;
        box.w -= 6.0f;
    }

    return box;
}

bool collision_check_player(const Player *player, const Terrain *terrain, CollisionResult *result)
{
    Aabb player_box;
    int i;
    int j;

    if (result != 0) {
        result->hit = false;
        result->segment_index = -1;
        result->obstacle_index = -1;
    }
    if (player == 0 || terrain == 0) {
        return false;
    }

    player_box = collision_player_box(player);
    for (i = 0; i < terrain->segment_count; ++i) {
        const TerrainSegment *segment = &terrain->segments[i];

        /* 已经完全离开屏幕左侧的段不参与判定。 */
        if (segment->x + segment->width < -20.0f) {
            continue;
        }
        for (j = 0; j < segment->obstacle_count; ++j) {
            Aabb obstacle_box = collision_obstacle_box(&segment->obstacles[j]);
            if (collision_aabb_overlap(player_box, obstacle_box)) {
                if (result != 0) {
                    result->hit = true;
                    result->segment_index = i;
                    result->obstacle_index = j;
                }
                return true;
            }
        }
    }
    return false;
}

void input_reset(Input *input)
{
    if (input != 0) {
        memset(input, 0, sizeof(*input));
    }
}
