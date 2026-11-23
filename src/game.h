/* 同一个 .c 或 .cpp 编译时，即使不同文件都包含了 game.h，编译器也只读它一次，避免重复定义结构体。 */
#pragma once
/* include 引入已有声明。下面三个标准头文件分别提供 bool、size_t 等类型和固定位数的整数类型。 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* 用 C++ 编译时才读 extern "C" 这行，让 .cpp 能按 C 的函数名和 .c 连起来。末尾有配对的右花括号。 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * 雪原跑酷 - 全局常量
 * 纯 C，不依赖任何平台头文件。
 */

/* #define 给值取名字，编译前会替换使用它的地方。下面的大写名字在整个游戏中共用。 */
#define SP_GAME_VERSION "0.1.0"
#define SP_GAME_NAME "雪原跑酷"

/* 如果构建时还没指定调试面板开关，这里默认取 1（开启）。#endif 表示这段条件结束。 */
#ifndef SP_DEBUG_HUD
#define SP_DEBUG_HUD 1
#endif

#define SP_SCREEN_WIDTH 1280
#define SP_SCREEN_HEIGHT 720
#define SP_TARGET_FPS 60
/* 一次更新在游戏里走过的秒数。数字末尾的 f 表示 float 浮点数，1.0f / 60.0f 大约是 0.0167 秒。 */
#define SP_FIXED_DT (1.0f / 60.0f)

#define SP_GROUND_Y 612.0f
#define SP_PLAYER_START_X 260.0f
#define SP_PLAYER_W 42.0f
#define SP_PLAYER_H 56.0f

#define SP_GRAVITY 2100.0f
#define SP_JUMP_SPEED (-880.0f)
#define SP_MAX_FALL_SPEED 1500.0f
#define SP_COYOTE_TIME 0.10f
#define SP_JUMP_BUFFER_TIME 0.13f
#define SP_MAX_JUMPS 2

#define SP_TERRAIN_MAX_SEGMENTS 40
#define SP_TERRAIN_MAX_OBSTACLES 4
#define SP_TERRAIN_AHEAD 900.0f
#define SP_TERRAIN_RECYCLE_X (-180.0f)
#define SP_TERRAIN_SAFE_LEAD 1180.0f

#define SP_SCORE_PIXELS_PER_METER 45.0f
#define SP_SCORE_TIME_BONUS 6.0f
#define SP_HIGH_SCORE_LIMIT 100000000

/*
 * windows.cpp 读取键盘和鼠标，game.c 根据按键更新游戏。
 */

/* struct 把相关变量装成一组；typedef 让我们直接用 Input 声明变量，例如 main 里的 Input keys。 */
typedef struct Input {
    bool jump_pressed;    /* 本帧按下（边沿） */
    bool jump_held;       /* 当前按住 */
    bool pause_pressed;   /* 本帧按下 P */
    bool restart_pressed; /* 本帧按下 R */
    bool quit_pressed;    /* 本帧按下 ESC */
    bool debug_pressed;   /* 本帧按下 F1 */
    bool mouse_clicked;   /* 本帧左键按下 */
} Input;

/* 这里是函数声明，函数体在 .c 文件。Input * 表示接收地址：传入 &keys，函数就能清空原来的 keys。 */
void input_reset(Input *input);

/*
 * 显式种子的 xorshift32 随机数发生器。
 * 不使用 time()/rand()，保证同一种子产生同一序列。
 */

typedef struct Rng {
    uint32_t state;
} Rng;

void rng_seed(Rng *rng, uint32_t seed);
uint32_t rng_next_u32(Rng *rng);
int rng_range(Rng *rng, int min_inclusive, int max_exclusive);
float rng_float01(Rng *rng);

/*
 * 难度曲线：只依赖存活时间，输出速度 / 间距 / 高难模板权重。
 */

typedef struct DifficultyParams {
    float speed;       /* 世界滚动速度 px/s */
    float gap;         /* 模板之间的基础空隙 px */
    float hard_weight; /* 0..1，越高越偏向复杂模板 */
    int level;         /* 1..10 */
} DifficultyParams;

void difficulty_init(DifficultyParams *params);
DifficultyParams difficulty_at(float survival_seconds);
const wchar_t *difficulty_level_name(int level);

/*
 * 玩家垂直物理：重力、跳跃、二段跳、土狼时间、跳跃缓冲。
 * 横向位移由世界滚动表达，玩家屏幕 x 保持不变。
 */

typedef struct Player {
    float x;
    float y;
    /* 竖直速度，单位为像素/秒。屏幕 y 向下增加，因此负速度让角色上升。 */
    float vy;
    bool on_ground;
    int jumps_used;
    /* 离开地面后还能补按起跳的剩余秒数。下一项记录提前按下跳跃还能保留多久。 */
    float coyote_timer;
    float jump_buffer_timer;
    float run_phase;
} Player;

Player starting_player(float x, float ground_y);
/* 角色按值传递，函数在副本上算出新位置；调用者用返回的 Player 更新 game.player。 */
Player move_player(Player player, Input keys, float ground_y, float dt);

/*
 * 计分与最高分逻辑。core 不进行文件 IO。
 */

typedef struct ScoreState {
    int current;
    int high_score;
    float distance_px;
    float distance_m;
    bool new_record;
    /* 为 true 时，内存里的最高分需要写入存档。 */
    bool high_score_dirty;
} ScoreState;

void score_reset(ScoreState *score);
void score_set_high_score(ScoreState *score, int value);
void score_update(ScoreState *score, float distance_px, float elapsed_seconds);
void score_finish(ScoreState *score);

/*
 * 程序化地形：模板池 + 显式随机种子。
 * 地形段只保存障碍模板，地面本身是连续的；段与段之间保留最小反应空隙。
 */

/* enum 给整数起可读的名字。例如代码用 OBSTACLE_SPIKE 表示冰刺，判断种类时就不用猜 0 的含义。 */
typedef enum ObstacleType {
    OBSTACLE_SPIKE = 0,
    OBSTACLE_CRATE = 1,
    OBSTACLE_TALL = 2,
    OBSTACLE_WIDE = 3
} ObstacleType;

typedef struct Obstacle {
    float x;
    float y;
    float w;
    float h;
    uint8_t type;
} Obstacle;

typedef struct TerrainSegment {
    float x;
    float width;
    int pattern;
    int obstacle_count;
    /* 一段路里可以放几个障碍。obstacles[0] 是第一个，obstacle_count 记录实际用了几个。 */
    Obstacle obstacles[SP_TERRAIN_MAX_OBSTACLES];
} TerrainSegment;

typedef struct Terrain {
    /* 整条路由数组里的多段组成。例如 segments[1].obstacles[0] 取出第二段路的第一个障碍。 */
    TerrainSegment segments[SP_TERRAIN_MAX_SEGMENTS];
    int segment_count;
    float next_x;
    float ground_y;
    Rng rng;
    uint32_t generated;
} Terrain;

void terrain_init(Terrain *terrain, uint32_t seed, float ground_y, const DifficultyParams *difficulty);
void terrain_scroll(Terrain *terrain, float dx, const DifficultyParams *difficulty);
int terrain_obstacle_count(const Terrain *terrain);

/*
 * AABB 碰撞。core 不负责绘制，也不负责把失败写回 GameState。
 */

typedef struct Aabb {
    float x;
    float y;
    float w;
    float h;
} Aabb;

typedef struct CollisionResult {
    bool hit;
    /* 碰到的是哪一段路、段内的第几个障碍。两个下标都从 0 开始，-1 表示没有命中。 */
    int segment_index;
    int obstacle_index;
} CollisionResult;

bool collision_aabb_overlap(Aabb a, Aabb b);
Aabb collision_player_box(const Player *player);
Aabb collision_obstacle_box(const Obstacle *obstacle);
/* const Player * 允许通过地址读玩家数据；最后的 result 允许函数写回碰到哪一段、哪一块。 */
bool collision_check_player(const Player *player, const Terrain *terrain, CollisionResult *result);

/*
 * GameState：保存当前这一局的数据。
 */

typedef enum GamePhase {
    GAME_PHASE_PLAYING = 0,
    GAME_PHASE_PAUSED = 1,
    GAME_PHASE_GAME_OVER = 2
} GamePhase;

typedef struct GameState {
    GamePhase phase;
    Player player;
    Terrain terrain;
    DifficultyParams difficulty;
    ScoreState score;
    Input last_input;

    uint32_t base_seed;
    uint32_t seed;
    uint32_t run_index;
    uint64_t fixed_steps;

    float elapsed_time;
    float distance_px;
    float world_speed;
    float death_time;

    int screen_width;
    int screen_height;
    float ground_y;
    bool debug_hud;
    bool quit_requested;

    int collision_segment;
    int collision_obstacle;
    int last_hit_type;
} GameState;

/* extern 告诉编译器去别的文件找这个变量。这里的声明让多个文件能访问同一份游戏状态。 */
extern GameState game;  /* 定义在 game.c，main 和画面读取同一局。 */
void game_init(uint32_t seed);
void game_restart(void);
void game_update(Input keys);
const char *game_phase_name(GamePhase phase);

typedef struct Platform {
    bool initialized;
    bool quit;
    int width;
    int height;
    /* Windows 用窗口句柄识别窗口。这里暂存为通用指针，windows.cpp 调系统函数时再转回 HWND。 */
    void *hwnd;
    double qpc_frequency;
    long long qpc_origin;
    double start_time;
} Platform;

bool platform_init(Platform *platform, const wchar_t *title, int width, int height);
void platform_shutdown(Platform *platform);
void platform_pump(Platform *platform);
void platform_poll_input(Platform *platform, Input *input);
void platform_render(Platform *platform, const GameState *state, double fps);
void platform_present(Platform *platform);
bool platform_should_quit(const Platform *platform);

void platform_time_init(Platform *platform);
double platform_now_seconds(const Platform *platform);
void platform_sleep_seconds(double seconds);

/* out_value 指向调用者接收分数的变量；函数自己的 bool 返回值另行表示读档成功与否。 */
bool platform_storage_load_highscore(int *out_value);
bool platform_storage_save_highscore(int value);

void platform_crt_init(void);

#ifdef __cplusplus
}
#endif
