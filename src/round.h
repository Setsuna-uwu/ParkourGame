#pragma once  /* 一局游戏的全部运行数据；只由 round.c 修改。 */
#ifdef __cplusplus
extern "C" {
#endif

#include "game.h"

/* 课程报告需要真实姓名/学号；这里先放组员的 GitHub 名，提交前替换成学校要求的信息。 */
#define ROUND_AUTHOR L"制作：Setsuna-uwu（组长） / Wty-Phoenixmoon / shrink ink（学号待补）"

typedef struct {
    int score;          /* 当前分 = 最远距离 / 10 + 金币 * 5 + 特技分 */
    int best;           /* 历史最高分，从 highscore.dat 读取 */
    int lives;          /* 备用命；开局 0，捡到 ITEM_LIFE 增加 */
    int coins;          /* 本局已收金币，来自 #8 的 terrain_coin_count */
    int max_distance;   /* 本局到过的最远距离，复活后不回退 */
    int flip_score;     /* 每次成功落地按完整圈数累计的特技分 */
    int game_over;      /* 1 表示已经结算，等待 R/F2 开新局 */
    unsigned char picked[96]; /* 按 Pickup.id 标记已吃过的备用命，防止重复 */
} RoundState;

void round_init(RoundState *round);                 /* 读最高分，准备第一局 */
void round_restart(RoundState *round);              /* 保留最高分，清空本局 */
void round_update(RoundState *round, Player *player);
void round_set_draw(const RoundState *round);       /* 把本局指针交给 draw.cpp，不改变原绘图签名 */

#ifdef __cplusplus
}
#endif
