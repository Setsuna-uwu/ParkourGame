#include "game.h"
#include <math.h>
#include <string.h>

/* game 是整局共用的一份数据，game.player 表示取其中的角色。 */
GameState game;

/* 准备一局的初始数据，在第一次进入游戏和重开时都会调用。函数前的 static 表示只有本文件的代码能直接调用它。 */
static void game_start_run(void)
{
    /* 每局用局号算一个种子交给地形生成器。同一个起始种子和局号会排出相同的障碍。0x 表示十六进制，末尾 u 表示无符号整数。 */
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

    /* & 取地址，difficulty_init 拿到这个地址后可以直接填写 game.difficulty 的成员。 */
    difficulty_init(&game.difficulty);
    game.world_speed = game.difficulty.speed;

    score_reset(&game.score);
    game.player = starting_player(SP_PLAYER_START_X, game.ground_y);
    terrain_init(&game.terrain, game.seed, game.ground_y, &game.difficulty);
    input_reset(&game.last_input);
}

/* 程序启动时只做一次总初始化，再摆好第一局。 */
void game_init(uint32_t seed)
{
    /* 从 game 的首地址起，把整个结构体的字节置零。sizeof(game) 给出需要清零的字节数。 */
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

/* 玩家选择重开时，回到本局起点。 */
void game_restart(void)
{
    /* 开新局只重置本局数据，历史最高分和调试开关继续沿用。 */
    game_start_run();
}

/* main 每走一步都会传来本次按键。先处理退出、重开和暂停，只有还在跑时才移动、计分和检查碰撞。 */
void game_update(Input keys)
{
    CollisionResult collision;
    float dx;

    game.last_input = keys;

/* #if 在编译时判断开关；开启时才把下面到 #endif 之间的 F1 按键处理编进程序。 */
#if SP_DEBUG_HUD
    if (keys.debug_pressed) {
        /* ! 把真假翻转，所以按一次打开，再按一次关闭。 */
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

    /* 暂停期间走到这里就返回，下面的计时、位移和得分都被冻结。 */
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

    /* 把当前角色传进去算一步，再用返回的新角色覆盖旧值。dt 的单位是秒。 */
    game.player = move_player(game.player, keys, game.ground_y, SP_FIXED_DT);
    /* 跑得越远，摆腿动画的相位走得越多；这个值只给绘图用。 */
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
            /* 先用路段编号取出一段路，再用障碍编号找到段里的那一块，最后读它的类型。 */
            game.last_hit_type =
                game.terrain.segments[collision.segment_index].obstacles[collision.obstacle_index].type;
        }
        score_finish(&game.score);
    }

    game.fixed_steps += 1u;
}

/* 按当前状态选一个英文名称。返回值指向已有字符串，调用方只读取它。 */
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

/* 把数值限制在给定范围内。例如计时器减过头时，这个函数可以让它停在 0。 */
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

/* 填写开局的速度、间距和难度。params 保存调用方变量的地址，params->speed 就是该变量的 speed 成员。 */
void difficulty_init(DifficultyParams *params)
{
    /* 指针等于 0 表示没有指向有效变量，这时直接返回，避免往空地址写。 */
    if (params == 0) {
        return;
    }
    params->speed = 330.0f;
    params->gap = 470.0f;
    params->hard_weight = 0.0f;
    params->level = 1;
}

/* 用存活秒数推算当前难度，一次返回整组参数。超过 180 秒后沿用 180 秒的结果。 */
DifficultyParams difficulty_at(float survival_seconds)
{
    DifficultyParams params;
    float t;

    t = clamp_float(survival_seconds, 0.0f, 180.0f);

    /* 330 -> 约 520 px/s；前 60 秒变化最明显，之后缓慢逼近上限。 */
    /* expf 是指数函数。t 为零时括号内为零，t 增大时括号里的值逐渐接近 1。 */
    params.speed = 330.0f + 190.0f * (1.0f - expf(-t / 70.0f));

    /* 像素间距逐步收紧；模板内部还有自身宽度，难度还来自类型权重。 */
    params.gap = 470.0f - 110.0f * (t / 180.0f);
    params.hard_weight = clamp_float(t / 150.0f, 0.0f, 1.0f);

    /* 转成 int 会截掉小数，例如 39 秒时 39/20 得 1.95，截成 1 后再加 1 得到第 2 级。 */
    params.level = 1 + (int)(t / 20.0f);
    if (params.level > 10) {
        params.level = 10;
    }
    if (params.level < 1) {
        params.level = 1;
    }

    return params;
}

/* 把等级对应到界面上的中文名称。L 放在引号前表示宽字符串，供 Windows 中文界面使用。 */
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

/* 创建站在地上的角色。y 是角色顶边，所以地面高度减去身高才是起始 y。 */
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

/* 算经过 dt 秒后的位置与速度。player 是一份结构体副本，调用方用函数末尾返回的值更新角色。 */
Player move_player(Player player, Input keys, float ground_y, float dt)
{
    bool wants_jump;

    if (dt <= 0.0f) {
        return player;
    }

    /* 站在地面时补满离地宽限时间，离地后逐步扣除。这样刚离开边缘的一小段时间仍能起跳。 */
    if (player.on_ground) {
        player.coyote_timer = SP_COYOTE_TIME;
    } else {
        player.coyote_timer = clamp_float(player.coyote_timer - dt, 0.0f, SP_COYOTE_TIME);
    }

    /* 按跳会留下一个短暂请求。如果按键早于落地，接下来几步仍有机会用掉它。 */
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
            /* 空中还有跳跃次数时可以二段跳。速度乘 0.96，使这次起跳略弱；用完请求后清零，避免一次按键触发两跳。 */
            player.vy = SP_JUMP_SPEED * 0.96f;
            player.on_ground = false;
            player.jumps_used += 1;
            player.jump_buffer_timer = 0.0f;
        }
    }

    /* 屏幕向下是正方向：起跳给负速度，重力每步把速度加回来。 */
    player.vy = clamp_float(player.vy + SP_GRAVITY * dt, SP_JUMP_SPEED, SP_MAX_FALL_SPEED);
    player.y += player.vy * dt;

    /* 顶边 y 加身高就是脚底。脚底碰到或穿过地面时，把角色放回地表并恢复跳跃次数。 */
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

/* 开局先清掉本局分数和跑过的距离。 */
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

/* 把读档得到的历史分数写入游戏状态，超出允许范围时取最近的边界值。 */
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

/* 把像素距离换成米，距离和存活时间各贡献一部分分数，两项分别截取整数后相加。 */
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

/* 撞到障碍时结算。破纪录会标记 high_score_dirty，main 看到这个标记就把新纪录写入存档。 */
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

/* 判断两个与屏幕边缘平行的矩形有没有重叠面积。x、y 是左上角，w、h 是宽高。 */
bool collision_aabb_overlap(Aabb a, Aabb b)
{
    if (a.w <= 0.0f || a.h <= 0.0f || b.w <= 0.0f || b.h <= 0.0f) {
        return false;
    }
    /* 横向需要 a 的左边在 b 的右边以左、a 的右边在 b 的左边以右；纵向同理。四项都成立才算碰撞，只贴边不算。 */
    return (a.x < b.x + b.w) && (a.x + a.w > b.x) &&
           (a.y < b.y + b.h) && (a.y + a.h > b.y);
}

/* 根据玩家位置算判定矩形。const Player * 表示可以通过地址读取玩家，不能通过它修改玩家。 */
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

/* 从障碍外框生成判定矩形，冰刺与宽冰墙会往里收一点，留出擦边空间。 */
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

/* 逐段、逐个障碍检查。返回值说明是否撞到；传进 result 的地址时，还会把命中位置写回去。 */
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
        /* i 是路段下标。取数组中第 i 段的地址后，segment->obstacle_count 就是该段的障碍数量。 */
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

/* 还没有读到按键时，把各个按键标志一起清零。sizeof(*input) 取整个 Input 结构体的大小。 */
void input_reset(Input *input)
{
    if (input != 0) {
        memset(input, 0, sizeof(*input));
    }
}
