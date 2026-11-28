#define _CRT_SECURE_NO_WARNINGS  /* MSVC 下仍可使用标准 fopen/fread，不引入平台 API */
#include "round.h"

#include <stdio.h>
#include <string.h>

#define HIGH_SCORE_FILE "highscore.dat"  /* 运行目录/exe 目录下的简单二进制存档 */
#define MAX_PICKED 96                    /* 与 #8 的 Pickup 数量上限一致 */

/* 从文件读历史最高分；文件不存在、长度不对或数值异常都安全回退为 0。 */
static int load_best(void){
    FILE *file;
    int value=0;
    file=fopen(HIGH_SCORE_FILE,"rb");
    if(!file) return 0;
    if(fread(&value,sizeof(value),1,file)!=1) value=0;
    fclose(file);
    return value>=0 && value<=100000000 ? value : 0;
}

/* 只在真正刷新纪录时调用；写失败不弹窗、不中断游戏。 */
static void save_best(int value){
    FILE *file=fopen(HIGH_SCORE_FILE,"wb");
    if(!file) return;
    fwrite(&value,sizeof(value),1,file);
    fclose(file);
}

/* 分数拆成三部分，便于解释：距离、金币、完整后翻。 */
static int score_of(const RoundState *round){
    return round->max_distance/10 + round->coins*5 + round->flip_score;
}

/* 玩家脚底和物品的距离/高度判定；up 越大，允许拾取的高度越高。 */
static int near_player(const Player *player, const Pickup *item, int up){
    return item->distance>player->distance-18 && item->distance<player->distance+18 &&
           item->y>player->feet_y-up && item->y<player->feet_y+10;
}

/* 扣命：有备用命就在附近安全雪面复活，否则结算并保存新纪录。 */
static void damage(RoundState *round, Player *player){
    if(round->lives>0){
        round->lives--;
        *player=start_player(terrain_safe_distance(player->distance));
        return;
    }
    round->game_over=1;
    player->crashed=1;
    round->score=score_of(round);
    if(round->score>round->best){
        round->best=round->score;
        save_best(round->best);
    }
}

void round_init(RoundState *round){
    memset(round,0,sizeof(*round));
    round->best=load_best();
}

void round_restart(RoundState *round){
    int best=round->best;  /* R/F2 只清本局，历史最高分继续保留 */
    memset(round,0,sizeof(*round));
    round->best=best;
}

/* 每帧在 update_player 之后调用：读一次性事件、物品和当前距离。
   金币由 #8 的 terrain_collect_coins 处理，这里只读取累计数量，避免重复计数。 */
void round_update(RoundState *round, Player *player){
    const Pickup *items;
    int count,i,hit_rock=0;

    if(!round || !player || round->game_over) return;

    round->coins=terrain_coin_count();
    if((int)player->distance>round->max_distance)
        round->max_distance=(int)player->distance;

    items=terrain_pickups(&count);
    for(i=0;i<count;i++){
        if(items[i].kind==ITEM_LIFE && items[i].id<MAX_PICKED &&
           !round->picked[items[i].id] && near_player(player,&items[i],80)){
            round->picked[items[i].id]=1;
            round->lives++;
        }else if(items[i].kind==ITEM_ROCK && near_player(player,&items[i],50)){
            hit_rock=1;  /* 岩石和摔倒共用同一套扣命逻辑 */
        }
    }

    if(player->event==LAND_FLIP)
        round->flip_score+=flip_count(*player)*20;  /* 只有完整翻稳的事件才给分 */
    round->score=score_of(round);

    if(player->event==LAND_CRASH || hit_rock)
        damage(round,player);
}
