#pragma once  /* 每个源文件只读一次这份公共声明。 */
#ifdef __cplusplus
extern "C" {  /* 让 draw.cpp 里的函数能和 C 文件互相调用。 */
#endif

enum { WINDOW_WIDTH = 800, WINDOW_HEIGHT = 450 };
#define PLAYER_X 160  /* 屏幕上角色脚底的横坐标。 */

typedef struct {
    int has_ground;  /* 1 表示查到表面（雪地或跳板）；0 表示这一层是空的。 */
    float y;         /* 雪面高度，单位像素，向下增大。 */
    float slope;     /* 向右走 1 像素时雪面下降多少。向右下坡为正。 */
} Ground;

Ground ground_at(float distance);
Ground ramp_at(float distance);  /* 单独查木跳板；同一位置下方还可能有雪地。 */

/* #8：在 start_player 之前调用一次。种子 0 = 原来的固定样例坡。 */
void terrain_init(unsigned seed);
unsigned terrain_seed(void);
int terrain_gap_is_passable(float width, float speed);
float terrain_safe_distance(void);

/* 起跳那次按住多久才算想翻（毫秒）。空中重新按一次则不用等。 */
enum { HOLD_TO_FLIP_MS = 160 };
typedef struct {
    int jump_pressed;  /* 本帧刚按下空格 */
    int space_down;    /* 处理完按键后，空格是否还按着 */
    int long_press;    /* 这一次空格是否连续按够了 HOLD_TO_FLIP_MS */
    int retry;         /* 按 R 从当前雪道起点重新试 */
    int reseed;        /* 按 F2 生成下一条随机雪道（调试用） */
    int quit;          /* 按 Esc 或关窗口 */
} Keys;

/* #9 每帧可以读取一次 player.event。下一帧它会恢复为 LAND_NONE。 */
enum { LAND_NONE, LAND_FLIP, LAND_CRASH };
enum { FLIP_OFF, FLIP_WAIT, FLIP_TURN };  /* 未翻、等起跳键按够时间、正在后翻 */

typedef struct {
    float distance;    /* 从起点向右走过的距离，也是 #9 计距离时读的数。 */
    float feet_y;      /* 脚底位置，数值越大越靠近屏幕下方。 */
    float speed_x;     /* 每帧向右滑多少像素。 */
    float speed_y;     /* 每帧上下移动多少像素；负数向上。 */
    float angle;       /* 板头朝右为 0 度，板头向下为正，后翻时角度减小。 */
    float turn;        /* 这一次腾空，主动后翻累计转了几度。 */
    float jump_speed;  /* 主动起跳额外给多少向上速度，普通跳为 10，可用于加强跳。 */
    float takeoff_y;   /* 离地时脚底的位置。 */
    float jump_height; /* 本次腾空相对起跳点最多升高多少像素，落地后保留到下次离地。 */
    int flip_state;    /* 记住空格在这一跳里用到了哪一步。 */
    int on_ground;     /* 1 踩着雪或板，0 在空中。 */
    int on_ramp;       /* 踩在跳板上时为 1；滑出板头要腾空，即使板下还有雪。 */
    int crashed;       /* 摔倒后停在原地，方便看清落地姿势。 */
    int event;         /* 本帧是否刚刚翻稳或摔倒。普通落地没有特技事件。 */
} Player;

Player start_player(float distance);  /* 新局传 0，复活传 #8 给的安全雪面位置。 */
int flip_count(Player player);        /* 这次腾空已转满几圈。#9 在 LAND_FLIP 那一帧计分。 */
Player update_player(Player player, Keys keys);

int open_window(void);
Keys read_key(void);
void draw_frame(Player player);
void close_window(void);

#ifdef __cplusplus
}
#endif
