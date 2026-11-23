#include "game.h"
#include <graphics.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <wchar.h>

extern "C" {
}

/* ------------------------------------------------------------------ */
/* 颜色与小工具                                                        */
/* ------------------------------------------------------------------ */

#define COL_SKY_TOP RGB(7, 11, 30)
#define COL_SKY_MID RGB(25, 38, 74)
#define COL_SKY_LOW RGB(82, 102, 139)
#define COL_SNOW RGB(238, 245, 252)
#define COL_SNOW_SHADE RGB(191, 210, 232)
#define COL_SNOW_DEEP RGB(149, 177, 210)
#define COL_SOIL RGB(27, 39, 65)
#define COL_SOIL_DARK RGB(19, 28, 49)
#define COL_PANEL RGB(13, 20, 39)
#define COL_PANEL_EDGE RGB(79, 104, 148)
#define COL_TEXT RGB(232, 240, 250)
#define COL_MUTED RGB(154, 174, 203)
#define COL_ACCENT RGB(255, 188, 92)
#define COL_ACCENT_RED RGB(240, 92, 84)
#define COL_ICE RGB(109, 207, 235)
#define COL_ICE_LIGHT RGB(189, 240, 250)
#define COL_ICE_EDGE RGB(40, 121, 158)
#define COL_WOOD RGB(139, 92, 56)
#define COL_WOOD_LIGHT RGB(184, 132, 82)
#define COL_WOOD_DARK RGB(91, 56, 36)
#define COL_JACKET RGB(224, 94, 57)
#define COL_JACKET_DARK RGB(164, 57, 39)
#define COL_PANTS RGB(39, 58, 92)
#define COL_SKIN RGB(246, 203, 164)
#define COL_SCARF RGB(236, 74, 87)
#define COL_DEBUG_PLAYER RGB(255, 82, 82)
#define COL_DEBUG_OBSTACLE RGB(255, 224, 76)

/* 这里的 static 表示函数只在 draw.cpp 内使用。把数值限制在两个端点之间，后面的调色和阴影都要用。 */
static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

/* 游戏坐标带小数，画图需要整像素。lroundf 取最近的整数，再转成 EasyX 使用的 int。 */
static int iround(float value)
{
    return (int)lroundf(value);
}

/* COLORREF 保存一种颜色。分别算红、绿、蓝三个分量；t 为 0 时取 a，为 1 时取 b，中间各取一部分。 */
static COLORREF blend_color(COLORREF a, COLORREF b, float t)
{
    int r;
    int g;
    int bl;
    t = clampf(t, 0.0f, 1.0f);
    r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    return RGB(r, g, bl);
}

/* 设置后续文字的字号和字重。L 开头的字符串存宽字符，便于传中文给 Windows。 */
static void set_font(int height, bool bold)
{
    /* 条件 ? 甲 : 乙 是一个表达式：bold 为真选粗体，为假选常规字重。 */
    settextstyle(height, 0, L"Microsoft YaHei UI", 0, 0,
                 bold ? FW_BOLD : FW_NORMAL, false, false, false);
    setbkmode(TRANSPARENT);
}

/* 将文字左上角放在 (x, y)。const wchar_t *text 指向字符串的开头，函数只读取它。 */
static void text_left(int x, int y, int height, COLORREF color, const wchar_t *text, bool bold)
{
    set_font(height, bold);
    settextcolor(color);
    outtextxy(x, y, text);
}

/* 先选定字体、量出整行的像素宽度，再从中心向左退半行宽作为起点。 */
static void text_center(int center_x, int y, int height, COLORREF color, const wchar_t *text, bool bold)
{
    int width;
    set_font(height, bold);
    settextcolor(color);
    width = textwidth(text);
    outtextxy(center_x - width / 2, y, text);
}

/* 先在右下方画一层暗色阴影，再在原位置画圆角底板和边框。 */
static void draw_panel(int x, int y, int width, int height, COLORREF fill, COLORREF border)
{
    setfillcolor(RGB(4, 7, 15));
    solidroundrect(x + 7, y + 8, x + width + 7, y + height + 8, 18, 18);
    setfillcolor(fill);
    solidroundrect(x, y, x + width, y + height, 18, 18);
    setlinecolor(border);
    setlinestyle(PS_SOLID, 2);
    roundrect(x, y, x + width, y + height, 18, 18);
    setlinestyle(PS_SOLID, 1);
}

/* 把编号打散成一个可重复的无符号数。同一颗星每帧算出同一个位置，画面就不会闪。 */
static unsigned int hash32(unsigned int value)
{
    /* >> 16 把原数的位向右移 16 位，^= 把移位结果和原数逐位异或后写回。后缀 u 表示无符号整数。 */
    value ^= value >> 16;
    value *= 0x7FEB352Du;
    value ^= value >> 15;
    value *= 0x846CA68Bu;
    value ^= value >> 16;
    return value;
}

/* ------------------------------------------------------------------ */
/* 背景                                                                */
/* ------------------------------------------------------------------ */

/* 从屏幕顶部向下，每 6 个像素画一个色带；上半段和下半段各自在两种颜色之间过渡。 */
static void draw_sky(int width, int height)
{
    int y;
    const int band = 6;

    for (y = 0; y < height; y += band) {
        float t = (float)y / (float)height;
        COLORREF color;
        if (t < 0.62f) {
            color = blend_color(COL_SKY_TOP, COL_SKY_MID, t / 0.62f);
        } else {
            color = blend_color(COL_SKY_MID, COL_SKY_LOW, (t - 0.62f) / 0.38f);
        }
        setfillcolor(color);
        solidrectangle(0, y, width, y + band);
    }
}

/* 依次用 96 个编号生成星点。用余数把坐标限制在天空范围内。 */
static void draw_stars(int width)
{
    int i;
    for (i = 0; i < 96; ++i) {
        unsigned int h = hash32((unsigned int)i * 2654435761u + 17u);
        int x = (int)(h % (unsigned int)width);
        int y = (int)((h >> 9) % 330u) + 18;
        /* 右移后用 & 1u 取最低一位，把半径分成两种。下一行用 & 3u 取两位来挑颜色。 */
        int radius = ((h >> 21) & 1u) ? 1 : 2;
        COLORREF color = ((h >> 23) & 3u) == 0u ? RGB(255, 227, 174) : RGB(205, 224, 250);
        setfillcolor(blend_color(COL_SKY_MID, color, 0.65f));
        solidcircle(x, y, radius);
    }
}

/* 从大到小叠圆形成月晕，最后画明亮的月面和上面的小圆斑。 */
static void draw_moon(void)
{
    int i;
    for (i = 7; i >= 1; --i) {
        setfillcolor(blend_color(COL_SKY_MID, RGB(207, 222, 249), 0.045f * (float)(8 - i)));
        solidcircle(1045, 122, 34 + i * 7);
    }
    setfillcolor(RGB(238, 243, 255));
    solidcircle(1045, 122, 37);
    setfillcolor(RGB(218, 226, 244));
    solidcircle(1033, 112, 7);
    solidcircle(1057, 132, 5);
    solidcircle(1052, 106, 3);
}

/* 一排山按 period 像素重复。每座山由山体和雪顶两个三角形组成。 */
static void draw_mountain_layer(int width, int base_y, float offset, float period, float peak_height,
                                COLORREF body, COLORREF cap)
{
    float x;

    /* fmodf 求小数余数：即使已经跑了很远，也只需要知道当前这一个周期内的偏移。 */
    offset = fmodf(offset, period);
    if (offset < 0.0f) {
        offset += period;
    }

    for (x = -period * 2.0f - offset; x < (float)width + period; x += period) {
        float peak_x = x + period * 0.5f;
        float peak_y = (float)base_y - peak_height;
        float cap_drop = peak_height * 0.30f;
        float half_at_cap = period * 0.25f * (cap_drop / peak_height);
        /* POINT 是 Windows 的整数坐标点。数组里三个点依次交给 solidpolygon，就能填满三角形。 */
        POINT mountain[3];
        POINT snow_cap[3];

        mountain[0].x = (LONG)iround(x);
        mountain[0].y = base_y;
        mountain[1].x = (LONG)iround(peak_x);
        mountain[1].y = (LONG)iround(peak_y);
        mountain[2].x = (LONG)iround(x + period);
        mountain[2].y = base_y;

        setfillcolor(body);
        solidpolygon(mountain, 3);

        snow_cap[0].x = (LONG)iround(peak_x);
        snow_cap[0].y = (LONG)iround(peak_y + 2.0f);
        snow_cap[1].x = (LONG)iround(peak_x + half_at_cap);
        snow_cap[1].y = (LONG)iround(peak_y + cap_drop);
        snow_cap[2].x = (LONG)iround(peak_x - half_at_cap);
        snow_cap[2].y = (LONG)iround(peak_y + cap_drop);
        setfillcolor(cap);
        solidpolygon(snow_cap, 3);
    }
}

/* 以 (x, base_y) 为树根，先画树干，再按树高画三层三角形枝叶和积雪。 */
static void draw_pine(int x, int base_y, int height, COLORREF color)
{
    POINT lower[3];
    POINT middle[3];
    POINT upper[3];

    setfillcolor(RGB(58, 42, 40));
    solidrectangle(x - 2, base_y - height / 3, x + 2, base_y);

    lower[0].x = x - height / 3;
    lower[0].y = base_y - height / 4;
    lower[1].x = x;
    lower[1].y = base_y - height * 2 / 3;
    lower[2].x = x + height / 3;
    lower[2].y = base_y - height / 4;
    setfillcolor(color);
    solidpolygon(lower, 3);

    middle[0].x = x - height / 4;
    middle[0].y = base_y - height / 2;
    middle[1].x = x;
    middle[1].y = base_y - height;
    middle[2].x = x + height / 4;
    middle[2].y = base_y - height / 2;
    solidpolygon(middle, 3);

    upper[0].x = x - height / 6;
    upper[0].y = base_y - height * 3 / 4;
    upper[1].x = x;
    upper[1].y = base_y - height - 8;
    upper[2].x = x + height / 6;
    upper[2].y = base_y - height * 3 / 4;
    solidpolygon(upper, 3);

    setlinecolor(blend_color(color, COL_SNOW, 0.52f));
    setlinestyle(PS_SOLID, 2);
    line(x, base_y - height - 5, x - height / 7, base_y - height / 2);
    line(x, base_y - height - 5, x + height / 7, base_y - height / 2);
    setlinestyle(PS_SOLID, 1);
}

/* 每 176 像素摆一棵树，树列只随跑动距离移动 34%，看起来位于跑道后方。 */
static void draw_forest(float distance, int width, int base_y)
{
    float offset = fmodf(distance * 0.34f, 176.0f);
    int i;
    for (i = -1; i < width / 150 + 3; ++i) {
        int x = (int)((float)i * 176.0f - offset);
        int h = 48 + (int)(hash32((unsigned int)i + 91u) % 34u);
        draw_pine(x, base_y, h, RGB(33, 61, 66));
    }
}

/* ------------------------------------------------------------------ */
/* 地面与装饰                                                          */
/* ------------------------------------------------------------------ */

/* 画出雪面和下方土层。state 是指向当前游戏状态的指针，state->ground_y 读出其中的地面高度。 */
static void draw_ground(const GameState *state)
{
    int width = state->screen_width;
    int height = state->screen_height;
    int ground = (int)state->ground_y;
    float offset = fmodf(state->distance_px, 190.0f);
    int i;

    setfillcolor(COL_SOIL);
    solidrectangle(0, ground + 12, width, height);
    setfillcolor(COL_SOIL_DARK);
    solidrectangle(0, ground + 116, width, height);

    setfillcolor(COL_SNOW);
    solidrectangle(0, ground, width, ground + 13);
    setfillcolor(COL_SNOW_SHADE);
    solidrectangle(0, ground + 11, width, ground + 22);

    for (i = -1; i < width / 150 + 3; ++i) {
        float x = (float)i * 150.0f - fmodf(offset * 0.55f, 150.0f);
        setfillcolor(COL_SNOW);
        solidellipse((int)x, ground - 8, (int)x + 150, ground + 12);
    }

    setlinecolor(COL_SNOW_DEEP);
    setlinestyle(PS_SOLID, 2);
    line(0, ground + 22, width, ground + 22);
    setlinestyle(PS_SOLID, 1);

    for (i = 0; i < 18; ++i) {
        unsigned int h = hash32((unsigned int)i * 113u + 5u);
        int x = (int)(h % (unsigned int)width);
        int y = ground + 35 + (int)((h >> 8) % 58u);
        setfillcolor(blend_color(COL_SOIL, COL_SOIL_DARK, 0.45f));
        solidcircle(x, y, 2 + (int)((h >> 18) % 3u));
    }

    setlinecolor(RGB(44, 61, 91));
    line(0, ground + 70, width, ground + 70);
    line(0, ground + 102, width, ground + 102);
}

/* 把雪盖石块当作路边装饰，按跑动距离平移。这里算出的坐标只用于画图。 */
static void draw_ground_decor(float distance, int width, int ground)
{
    float offset = fmodf(distance, 240.0f);
    int i;

    for (i = -1; i < width / 220 + 2; ++i) {
        int x = (int)((float)i * 240.0f - offset);
        unsigned int h = hash32((unsigned int)i * 71u + 13u);
        int rock_x = x + (int)(h % 140u);
        int rock_y = ground + 42 + (int)((h >> 6) % 28u);
        setfillcolor(RGB(52, 69, 96));
        solidellipse(rock_x, rock_y, rock_x + 12, rock_y + 6);
        setfillcolor(COL_SNOW_SHADE);
        solidellipse(rock_x + 1, rock_y - 2, rock_x + 11, rock_y + 2);
    }
}

/* ------------------------------------------------------------------ */
/* 障碍物                                                              */
/* ------------------------------------------------------------------ */

/* 根据障碍的横坐标和宽度，在它正下方画一条略宽的椭圆阴影。 */
static void draw_obstacle_shadow(const Obstacle *obstacle, int ground)
{
    setfillcolor(RGB(151, 172, 199));
    solidellipse(iround(obstacle->x) - 4, ground - 4,
                 iround(obstacle->x + obstacle->w) + 4, ground + 4);
}

/* 用大三角形画冰锥轮廓，再叠一个浅色三角形表现光照。 */
static void draw_spike(const Obstacle *obstacle, int ground)
{
    POINT outer[3];
    POINT facet[3];
    int x = iround(obstacle->x);
    int y = iround(obstacle->y);
    int w = iround(obstacle->w);
    int h = iround(obstacle->h);

    outer[0].x = x;
    outer[0].y = ground;
    outer[1].x = x + w / 2;
    outer[1].y = y;
    outer[2].x = x + w;
    outer[2].y = ground;

    setfillcolor(COL_ICE);
    solidpolygon(outer, 3);

    facet[0].x = x + w / 2;
    facet[0].y = y + 4;
    facet[1].x = x + w / 2;
    facet[1].y = ground - 2;
    facet[2].x = x + w / 5;
    facet[2].y = ground - 2;
    setfillcolor(blend_color(COL_ICE, COL_ICE_LIGHT, 0.55f));
    solidpolygon(facet, 3);

    setlinecolor(COL_ICE_EDGE);
    setlinestyle(PS_SOLID, 2);
    line(outer[0].x, outer[0].y, outer[1].x, outer[1].y);
    line(outer[1].x, outer[1].y, outer[2].x, outer[2].y);
    setlinestyle(PS_SOLID, 1);

    setfillcolor(COL_SNOW);
    solidellipse(x + w / 2 - 6, y + h / 3, x + w / 2 + 4, y + h / 3 + 5);
}

/* 先填木箱的矩形，再用斜线画木板支架，最后盖上箱顶的积雪。 */
static void draw_crate(const Obstacle *obstacle, int ground)
{
    int x = iround(obstacle->x);
    int y = iround(obstacle->y);
    int w = iround(obstacle->w);
    int h = iround(obstacle->h);

    setfillcolor(COL_WOOD);
    solidrectangle(x, y, x + w, ground);
    setlinecolor(COL_WOOD_DARK);
    setlinestyle(PS_SOLID, 2);
    rectangle(x, y, x + w, ground);
    line(x + 2, y + 2, x + w - 2, ground - 2);
    line(x + w - 2, y + 2, x + 2, ground - 2);
    line(x, y + h / 2, x + w, y + h / 2);
    setlinestyle(PS_SOLID, 1);

    setfillcolor(COL_WOOD_LIGHT);
    solidrectangle(x + 4, y + 4, x + w - 4, y + 10);
    setfillcolor(COL_SNOW);
    solidellipse(x - 2, y - 7, x + w + 3, y + 5);
    solidrectangle(x - 2, y + 1, x + w + 3, y + 5);
}

/* 按障碍顶端和地面位置确定冰柱的高度，再叠加亮面、裂纹和顶端积雪。 */
static void draw_tall_ice(const Obstacle *obstacle, int ground)
{
    int x = iround(obstacle->x);
    int y = iround(obstacle->y);
    int w = iround(obstacle->w);

    setfillcolor(RGB(74, 155, 193));
    solidrectangle(x, y, x + w, ground);
    setfillcolor(RGB(133, 211, 235));
    solidrectangle(x + 4, y + 5, x + w - 8, ground - 5);
    setlinecolor(COL_ICE_EDGE);
    setlinestyle(PS_SOLID, 2);
    rectangle(x, y, x + w, ground);
    line(x + 7, y + 8, x + w - 9, ground - 10);
    line(x + w - 10, y + 14, x + 9, ground - 6);
    setlinestyle(PS_SOLID, 1);
    setfillcolor(COL_SNOW);
    solidellipse(x - 3, y - 7, x + w + 4, y + 6);
}

/* 用宽矩形画冰墙，每隔一段换一种蓝色，形成四条明暗相间的竖纹。 */
static void draw_wide_barrier(const Obstacle *obstacle, int ground)
{
    int x = iround(obstacle->x);
    int y = iround(obstacle->y);
    int w = iround(obstacle->w);
    int h = iround(obstacle->h);
    int i;

    setfillcolor(RGB(84, 169, 203));
    solidrectangle(x, y, x + w, ground);
    setfillcolor(RGB(145, 220, 241));
    for (i = 0; i < 4; ++i) {
        int sx = x + 7 + i * 17;
        setfillcolor(i % 2 == 0 ? RGB(173, 231, 246) : RGB(112, 196, 223));
        solidrectangle(sx, y + 5, sx + 11, ground - 4);
    }
    setlinecolor(COL_ICE_EDGE);
    setlinestyle(PS_SOLID, 2);
    rectangle(x, y, x + w, ground);
    setlinestyle(PS_SOLID, 1);
    setfillcolor(COL_SNOW);
    solidellipse(x - 2, y - 6, x + w + 2, y + 5);
    /* 把 h 转成 void 表示有意忽略这个局部变量，它不参与绘制。 */
    (void)h;
}

/* 外层遍历路段，内层遍历这一段里的障碍。先画阴影，再按类型挑选画法。 */
static void draw_obstacles(const GameState *state)
{
    int ground = (int)state->ground_y;
    int i;
    int j;

    for (i = 0; i < state->terrain.segment_count; ++i) {
        const TerrainSegment *segment = &state->terrain.segments[i];
        /* 整段在屏幕右边或左边 80 像素以外时，continue 直接跳去检查下一段。 */
        if (segment->x > (float)state->screen_width + 80.0f ||
            segment->x + segment->width < -80.0f) {
            continue;
        }
        for (j = 0; j < segment->obstacle_count; ++j) {
            const Obstacle *obstacle = &segment->obstacles[j];
            draw_obstacle_shadow(obstacle, ground);
            switch (obstacle->type) {
            case OBSTACLE_SPIKE:
                draw_spike(obstacle, ground);
                break;
            case OBSTACLE_CRATE:
                draw_crate(obstacle, ground);
                break;
            case OBSTACLE_TALL:
                draw_tall_ice(obstacle, ground);
                break;
            case OBSTACLE_WIDE:
                draw_wide_barrier(obstacle, ground);
                break;
            default:
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* 玩家                                                                */
/* ------------------------------------------------------------------ */

/* 用地面高度减去脚底坐标得到离地高度。跳得越高，影子越小，缩到设定的下限就停止。 */
static void draw_player_shadow(const GameState *state)
{
    const Player *player = &state->player;
    float jump_height = state->ground_y - (player->y + SP_PLAYER_H);
    float scale = clampf(1.0f - jump_height / 520.0f, 0.36f, 1.0f);
    int width = (int)(33.0f * scale);
    int height = (int)(9.0f * scale);

    setfillcolor(RGB(151, 173, 201));
    solidellipse(iround(player->x + SP_PLAYER_W / 2.0f) - width,
                 iround(state->ground_y) - height / 2,
                 iround(player->x + SP_PLAYER_W / 2.0f) + width,
                 iround(state->ground_y) + height / 2);
}

/* 把玩家位置当作人物左上角，各部位都相对它定位。手脚摆动只影响外观。 */
static void draw_player(const GameState *state)
{
    const Player *player = &state->player;
    int x = iround(player->x);
    int y = iround(player->y);
    int ground = iround(state->ground_y);
    bool air = !player->on_ground;
    bool dead = (state->phase == GAME_PHASE_GAME_OVER);
    /* sinf 让 swing 在 -1 和 1 之间往复；两脚用相反的偏移，跑起来就会交替迈步。 */
    float swing = sinf(player->run_phase);
    int hip_x = x + 21;
    int hip_y = y + 46;
    int foot1_x;
    int foot1_y;
    int foot2_x;
    int foot2_y;
    POINT scarf[4];

    if (air) {
        foot1_x = hip_x - 9;
        foot1_y = y + 56;
        foot2_x = hip_x + 8;
        foot2_y = y + 52;
    } else {
        foot1_x = hip_x + (int)(swing * 11.0f);
        foot1_y = ground - (swing > 0.25f ? 7 : 0);
        foot2_x = hip_x - (int)(swing * 11.0f);
        foot2_y = ground - (swing < -0.25f ? 7 : 0);
    }

    /* 速度线 */
    if (state->world_speed > 400.0f && state->phase == GAME_PHASE_PLAYING) {
        int i;
        setlinecolor(RGB(177, 199, 225));
        setlinestyle(PS_SOLID, 2);
        for (i = 0; i < 3; ++i) {
            int ly = y + 14 + i * 13;
            line(x - 20 - i * 9, ly, x - 45 - i * 16, ly);
        }
        setlinestyle(PS_SOLID, 1);
    }

    /* 围巾向后飘 */
    scarf[0].x = x + 9;
    scarf[0].y = y + 25;
    scarf[1].x = x - 22 - (int)(fabsf(swing) * 7.0f);
    scarf[1].y = y + 23 + (int)(sinf(player->run_phase * 1.7f) * 4.0f);
    scarf[2].x = x - 18;
    scarf[2].y = y + 34;
    scarf[3].x = x + 9;
    scarf[3].y = y + 33;
    setfillcolor(COL_SCARF);
    solidpolygon(scarf, 4);

    /* 腿 */
    setlinecolor(COL_PANTS);
    setlinestyle(PS_SOLID, 8);
    line(hip_x, hip_y, foot1_x, foot1_y);
    line(hip_x, hip_y, foot2_x, foot2_y);
    setlinecolor(RGB(29, 42, 70));
    line(foot1_x - 4, foot1_y, foot1_x + 6, foot1_y);
    line(foot2_x - 4, foot2_y, foot2_x + 6, foot2_y);
    setlinestyle(PS_SOLID, 1);

    /* 后臂 */
    setlinecolor(COL_JACKET_DARK);
    setlinestyle(PS_SOLID, 6);
    line(x + 22, y + 30, x + 12 - (int)(swing * 5.0f), y + 43);
    setlinestyle(PS_SOLID, 1);

    /* 身体 */
    setfillcolor(COL_JACKET);
    solidroundrect(x + 7, y + 21, x + 36, y + 50, 12, 12);
    setfillcolor(COL_JACKET_DARK);
    solidrectangle(x + 8, y + 42, x + 35, y + 49);
    setfillcolor(RGB(247, 214, 150));
    solidrectangle(x + 9, y + 35, x + 34, y + 39);

    /* 前臂 */
    setlinecolor(COL_JACKET);
    setlinestyle(PS_SOLID, 6);
    line(x + 22, y + 30, x + 34 + (int)(swing * 4.0f), y + 44);
    setlinestyle(PS_SOLID, 1);

    /* 头部与兜帽 */
    setfillcolor(RGB(188, 73, 48));
    solidcircle(x + 25, y + 13, 13);
    setfillcolor(COL_SKIN);
    solidcircle(x + 27, y + 13, 9);
    setfillcolor(RGB(54, 44, 48));
    solidellipse(x + 18, y + 4, x + 34, y + 12);
    setfillcolor(RGB(34, 29, 34));
    solidcircle(x + 30, y + 13, 2);
    if (dead) {
        setlinecolor(RGB(70, 35, 35));
        line(x + 28, y + 10, x + 33, y + 15);
        line(x + 33, y + 10, x + 28, y + 15);
        setlinestyle(PS_SOLID, 1);
    }

    /* 围巾结 */
    setfillcolor(COL_SCARF);
    solidroundrect(x + 9, y + 23, x + 25, y + 32, 6, 6);

    if (dead) {
        setlinecolor(RGB(255, 235, 235));
        setlinestyle(PS_SOLID, 3);
        line(x + 12, y - 12, x + 20, y - 4);
        line(x + 20, y - 12, x + 12, y - 4);
        line(x + 28, y - 14, x + 36, y - 6);
        line(x + 36, y - 14, x + 28, y - 6);
        setlinestyle(PS_SOLID, 1);
    }
}

/* ------------------------------------------------------------------ */
/* HUD / 覆盖层                                                        */
/* ------------------------------------------------------------------ */

/* 把分数、距离、速度和按键提示画在游戏场景上。这些一直看得见的信息叫 HUD。 */
static void draw_hud(const GameState *state)
{
    wchar_t buffer[128];
    int width = state->screen_width;

    draw_panel(26, 24, 360, 128, COL_PANEL, COL_PANEL_EDGE);
    text_left(46, 38, 17, COL_MUTED, L"分数", false);
    /* 把整数写进宽字符数组再画出来。%06d 表示至少占六位，不足时左边补 0。 */
    swprintf_s(buffer, 128, L"%06d", state->score.current);
    text_left(44, 55, 45, COL_TEXT, buffer, true);
    text_left(240, 40, 16, COL_MUTED, L"最高分", false);
    swprintf_s(buffer, 128, L"%06d", state->score.high_score);
    text_left(238, 62, 24, COL_ACCENT, buffer, true);
    swprintf_s(buffer, 128, L"距离 %d m", (int)state->score.distance_m);
    text_left(46, 114, 16, COL_MUTED, buffer, false);

    draw_panel(width - 334, 24, 308, 128, COL_PANEL, COL_PANEL_EDGE);
    text_left(width - 312, 38, 16, COL_MUTED, L"速度", false);
    swprintf_s(buffer, 128, L"%d", (int)state->world_speed);
    text_left(width - 314, 57, 38, COL_TEXT, buffer, true);
    text_left(width - 314, 103, 15, COL_MUTED, L"px/s", false);

    text_left(width - 188, 38, 16, COL_MUTED, L"难度", false);
    swprintf_s(buffer, 128, L"Lv %d", state->difficulty.level);
    text_left(width - 190, 58, 24, COL_ICE_LIGHT, buffer, true);
    text_center(width - 140, 92, 17, COL_TEXT, difficulty_level_name(state->difficulty.level), true);
    swprintf_s(buffer, 128, L"%d 秒", (int)state->elapsed_time);
    text_left(width - 190, 116, 15, COL_MUTED, buffer, false);

    if (state->elapsed_time < 4.2f && state->phase == GAME_PHASE_PLAYING && !state->debug_hud) {
        draw_panel(width / 2 - 292, 166, 584, 62, RGB(19, 29, 53), RGB(91, 116, 157));
        text_center(width / 2, 180, 20, COL_TEXT,
                    L"空格 跳跃   ·   空中再按一次 二段跳", true);
        text_center(width / 2, 205, 14, COL_MUTED,
                    L"跳过冰锥与木箱，跑得越远分数越高", false);
    }

    draw_panel(width / 2 - 354, state->screen_height - 47, 708, 33,
               RGB(15, 23, 42), RGB(66, 88, 126));
    text_center(width / 2, state->screen_height - 41, 15, COL_MUTED,
                L"SPACE 跳跃 / 二段跳     P 暂停     R 重开     F1 调试     ESC 退出", false);
}

/* 把暂停面板叠在屏幕中心，坐标都从中心 (cx, cy) 往外计算。 */
static void draw_pause_overlay(const GameState *state)
{
    int cx = state->screen_width / 2;
    int cy = state->screen_height / 2;
    wchar_t buffer[128];

    draw_panel(cx - 272, cy - 124, 544, 248, RGB(12, 19, 37), RGB(107, 141, 192));
    text_center(cx, cy - 94, 20, COL_ICE_LIGHT, L"PAUSED", true);
    text_center(cx, cy - 60, 44, COL_TEXT, L"已暂停", true);
    text_center(cx, cy + 6, 18, COL_MUTED, L"按 P 继续奔跑", false);
    swprintf_s(buffer, 128, L"当前分数  %d", state->score.current);
    text_center(cx, cy + 42, 19, COL_ACCENT, buffer, true);
    text_center(cx, cy + 82, 15, COL_MUTED, L"ESC 退出游戏", false);
}

/* 在结算面板上显示本局得分和纪录，再提示重开；提示的颜色随结束后的时间变化。 */
static void draw_game_over_overlay(const GameState *state)
{
    int cx = state->screen_width / 2;
    int cy = state->screen_height / 2;
    wchar_t buffer[128];
    COLORREF prompt_color;

    /* 正弦的范围是 -1 到 1，乘以 0.5 再加 0.5 就得到调色所需的 0 到 1。 */
    prompt_color = blend_color(COL_TEXT, COL_ACCENT,
                               0.5f + 0.5f * sinf(state->death_time * 4.0f));

    draw_panel(cx - 326, cy - 184, 652, 368, RGB(12, 19, 37), RGB(128, 82, 95));
    text_center(cx, cy - 150, 18, COL_ACCENT_RED, L"GAME OVER", true);
    text_center(cx, cy - 116, 40, COL_TEXT, L"撞上冰障", true);

    swprintf_s(buffer, 128, L"%d", state->score.current);
    text_center(cx, cy - 56, 62, COL_ACCENT, buffer, true);
    text_center(cx, cy - 2, 14, COL_MUTED, L"本局得分", false);

    swprintf_s(buffer, 128, L"最高分  %d     距离  %d m", state->score.high_score,
               (int)state->score.distance_m);
    text_center(cx, cy + 34, 19, COL_TEXT, buffer, false);

    if (state->score.new_record) {
        text_center(cx, cy + 66, 20, COL_ACCENT, L"★  新纪录  ★", true);
    } else {
        text_center(cx, cy + 66, 15, COL_MUTED, L"再来一次，冲击更高分", false);
    }

    text_center(cx, cy + 112, 21, prompt_color,
                L"按 R 或点击鼠标 重新开始", true);
    text_center(cx, cy + 148, 14, COL_MUTED, L"ESC 退出游戏", false);
}

/* 把游戏状态枚举换成中文文字，供调试面板直接显示。 */
static const wchar_t *phase_label(GamePhase phase)
{
    switch (phase) {
    case GAME_PHASE_PLAYING:
        return L"进行中";
    case GAME_PHASE_PAUSED:
        return L"已暂停";
    case GAME_PHASE_GAME_OVER:
        return L"已结束";
    default:
        return L"未知";
    }
}

/* 编译时开关。只有 SP_DEBUG_HUD 的值非零，编译器才会把下面的调试绘制函数编进程序。 */
#if SP_DEBUG_HUD
/* 用碰撞函数算出实际判定框：红框是角色，黄框是障碍。右侧面板列出同一帧的运行数据。 */
static void draw_debug_overlay(const GameState *state, double fps)
{
    int i;
    int j;
    int panel_x = state->screen_width - 390;
    int panel_y = 168;
    int line_y = panel_y + 42;
    wchar_t buffer[160];
    Aabb player_box;
    int ground = (int)state->ground_y;

    setlinecolor(COL_DEBUG_PLAYER);
    setlinestyle(PS_SOLID, 2);
    player_box = collision_player_box(&state->player);
    rectangle(iround(player_box.x), iround(player_box.y),
              iround(player_box.x + player_box.w), iround(player_box.y + player_box.h));

    setlinecolor(COL_DEBUG_OBSTACLE);
    for (i = 0; i < state->terrain.segment_count; ++i) {
        const TerrainSegment *segment = &state->terrain.segments[i];
        for (j = 0; j < segment->obstacle_count; ++j) {
            Aabb box = collision_obstacle_box(&segment->obstacles[j]);
            rectangle(iround(box.x), iround(box.y),
                      iround(box.x + box.w), iround(box.y + box.h));
        }
    }
    setlinecolor(RGB(255, 140, 90));
    line(0, ground, state->screen_width, ground);
    setlinestyle(PS_SOLID, 1);

    draw_panel(panel_x, panel_y, 364, 330, RGB(9, 14, 28), RGB(95, 120, 160));
    text_left(panel_x + 20, panel_y + 13, 18, COL_ICE_LIGHT, L"DEBUG  ·  F1 关闭", true);
    swprintf_s(buffer, 160, L"FPS        %.1f", fps);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"速度       %.1f  |  间距 %.1f", state->difficulty.speed,
               state->difficulty.gap);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"难度       Lv %d  hard %.2f", state->difficulty.level,
               state->difficulty.hard_weight);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"状态       %s", phase_label(state->phase));
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"第 %u 局  seed %08X", state->run_index, state->seed);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"y %.1f  vy %.1f  地面 %s", state->player.y, state->player.vy,
               state->player.on_ground ? L"是" : L"否");
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"跳跃 %d/2  土狼 %.2f  缓冲 %.2f", state->player.jumps_used,
               state->player.coyote_timer, state->player.jump_buffer_timer);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"输入 空格 %s  P %s  R %s",
               state->last_input.jump_held ? L"按下" : L"松开",
               state->last_input.pause_pressed ? L"1" : L"0",
               state->last_input.restart_pressed ? L"1" : L"0");
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"地形段 %d  障碍 %d", state->terrain.segment_count,
               terrain_obstacle_count(&state->terrain));
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 22;
    swprintf_s(buffer, 160, L"碰撞 %s  seg %d / obs %d", state->phase == GAME_PHASE_GAME_OVER ? L"是" : L"否",
               state->collision_segment, state->collision_obstacle);
    text_left(panel_x + 20, line_y, 15, COL_TEXT, buffer, false);
    line_y += 28;
    text_left(panel_x + 20, line_y, 13, COL_DEBUG_PLAYER, L"红框 玩家 AABB", false);
    text_left(panel_x + 170, line_y, 13, COL_DEBUG_OBSTACLE, L"黄框 障碍 AABB", false);
}
#endif

/* ------------------------------------------------------------------ */
/* 对外渲染入口                                                        */
/* ------------------------------------------------------------------ */

/* main.c 每帧调用一次。按天空、远景、地面、角色、面板的顺序画，后画的图形会盖住先画的部分。 */
extern "C" void platform_render(Platform *platform, const GameState *state, double fps)
{
    if (platform == 0 || state == 0 || !platform->initialized) {
        return;
    }

    setbkcolor(COL_SKY_TOP);
    cleardevice();

    draw_sky(state->screen_width, state->screen_height);
    draw_stars(state->screen_width);
    draw_moon();

    /* 同一段路程乘不同的系数，远山移动少、近山移动多，看起来就有远近层次。 */
    draw_mountain_layer(state->screen_width, 590, state->distance_px * 0.07f, 410.0f, 186.0f,
                        RGB(50, 68, 105), RGB(168, 190, 220));
    draw_mountain_layer(state->screen_width, 616, state->distance_px * 0.14f, 300.0f, 116.0f,
                        RGB(35, 51, 81), RGB(143, 169, 202));
    draw_forest(state->distance_px, state->screen_width, (int)state->ground_y + 2);

    draw_ground(state);
    draw_ground_decor(state->distance_px, state->screen_width, (int)state->ground_y);
    draw_obstacles(state);
    draw_player_shadow(state);
    draw_player(state);

    draw_hud(state);

#if SP_DEBUG_HUD
    if (state->debug_hud) {
        draw_debug_overlay(state, fps);
    }
#else
    (void)fps;
#endif
    if (state->phase == GAME_PHASE_PAUSED) {
        draw_pause_overlay(state);
    } else if (state->phase == GAME_PHASE_GAME_OVER) {
        draw_game_over_overlay(state);
    }
}
