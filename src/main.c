#include "game.h"
#include "round.h"

/* 复现某条随机路线时，把这里改成 HUD 上记录的种子，再运行 run.bat。 */
#define INITIAL_TERRAIN_SEED 0u

int main(void){
    terrain_init(INITIAL_TERRAIN_SEED);  /* 0 = 固定样例坡；F2 再切到随机雪道。 */
    Player player=start_player(0);
    RoundState round;                  /* #9 的一局数据放在循环外，R 时只清本局 */
    round_init(&round);
    if(!open_window()) return 1;
    round_set_draw(&round);           /* 把状态交给 draw.cpp 的 HUD 覆盖层 */

    /* 循环一次就是一帧：先读键，再算新位置，最后把这一帧画出来。 */
    while(1){
        Keys keys=read_key();
        if(keys.quit) break;
        /* TODO #9：把分数、备用命和已拾取的物品记在本局。用 R 开新局时一起清零；
           摔倒后用备用命继续时保留这些记录。角色本帧的结果要在 update_player 后读。 */
        if(keys.reseed || keys.retry) round_restart(&round);  /* 重开只清本局，最高分保留 */
        if(keys.reseed){
            terrain_init(terrain_seed()+1u);  /* 换一条路线，本局从起点重来。 */
            player=start_player(0);
        }else if(keys.retry){
            terrain_init(terrain_seed());     /* 同一种子，重开本局并重置金币。 */
            player=start_player(0);
        }else{
            player=update_player(player,keys);
            terrain_collect_coins(player.distance,player.feet_y);
            /* #9：LAND_FLIP 时可按 flip_count(player) 计分。LAND_CRASH 时只扣一次命；
               有备用命就用 start_player(安全位置) 恢复角色，本局分数和剩余命由 #9 保管。 */
            round_update(&round,&player);  /* 读 LAND_FLIP/LAND_CRASH、物品和距离 */
        }
        draw_frame(player);
    }
    close_window();
    return 0;
}
