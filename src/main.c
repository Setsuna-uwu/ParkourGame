#include "game.h"

int main(void){
    Player player=start_player(0);
    if(!open_window()) return 1;

    /* 循环一次就是一帧：先读键，再算新位置，最后把这一帧画出来。 */
    while(1){
        Keys keys=read_key();
        if(keys.quit) break;
        /* TODO #9：把分数、备用命和已拾取的物品记在本局。用 R 开新局时一起清零；
           摔倒后用备用命继续时保留这些记录。角色本帧的结果要在 update_player 后读。 */
        if(keys.retry){
            player=start_player(0);
        }else{
            player=update_player(player,keys);
            /* #9：LAND_FLIP 时可按 flip_count(player) 计分。LAND_CRASH 时只扣一次命；
               有备用命就用 start_player(安全位置) 恢复角色，本局分数和剩余命由 #9 保管。 */
        }
        draw_frame(player);
    }
    close_window();
    return 0;
}
