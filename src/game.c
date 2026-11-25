#include "game.h"
#include <math.h>

/* slope 说的是“每往右走 1 像素，下去多少”。atanf 把它换成角度：
   平地是 0 度，向右下坡是正角度。数学库算出来是弧度，再乘 180/π 换成度。 */
static float slope_angle(float slope){
    return atanf(slope)*180.0f/3.14159265f;
}

int flip_count(Player player){
    return (int)(player.turn/360);  /* 例如 810 度转满了两圈。 */
}

Player start_player(float distance){
    Player player={0};  /* 先清零，按 R 时也能一起清掉上次的转角和摔倒记录。 */
    Ground start=ground_at(distance);
    player.distance=distance;
    player.jump_speed=10;
    player.speed_x=4;
    if(start.has_ground){
        player.feet_y=start.y;
        player.angle=slope_angle(start.slope);
        player.on_ground=1;
    }
    return player;
}

Player update_player(Player player, Keys keys){
    player.event=LAND_NONE;  /* 上一帧的结果已经交给 #9，这一帧重新算。 */
    if(player.crashed) return player;

    Ground old_ramp=ramp_at(player.distance);
    Ground old=ground_at(player.distance);
    if(player.on_ramp) old=old_ramp;  /* 起跳用脚下那一层的坡度。 */
    if(player.on_ground && old.has_ground){
        /* 下坡时 slope 为正，越滑越快；上坡时为负，越滑越慢。速度限制在 3 到 8。 */
        player.speed_x+=0.2f*old.slope;
        if(player.speed_x<3) player.speed_x=3;
        if(player.speed_x>8) player.speed_x=8;
    }
    player.distance+=player.speed_x;
    Ground ground=ground_at(player.distance);  /* 向前滑了一帧，再问新位置有没有雪。 */
    Ground ramp=ramp_at(player.distance);
    int use_ramp=0;
    if(ramp.has_ground && (!ground.has_ground || ramp.y<=ground.y)){
        /* 上一帧脚在板面上方才可能踩到板。从板下通过时，仍然用下面的雪地。
           刚到入口时，按板的坡度往回算一帧，用来判断从哪一侧接近。 */
        float old_board_y=ramp.y-player.speed_x*ramp.slope;
        if(old_ramp.has_ground) old_board_y=old_ramp.y;
        use_ramp=player.feet_y<=old_board_y;
        if(use_ramp) ground=ramp;
    }

    if(player.on_ground){
        int left_ramp=player.on_ramp && !use_ramp;
        if(!keys.jump_pressed && ground.has_ground && !left_ramp){
            /* 踩着雪滑，脚底和板头都跟着新位置的坡面走。 */
            player.feet_y=ground.y;
            player.angle=slope_angle(ground.slope);
            player.on_ramp=use_ramp;
            return player;
        }
        /* 从崖沿自然滑出去，就保留刚才沿坡的上下速度。
           比如向右走 4、坡度为 -0.25，脚底原本每帧就会上升 1 像素。 */
        player.speed_y=0;
        if(old.has_ground) player.speed_y=player.speed_x*old.slope;
        player.on_ground=0;
        player.on_ramp=0;
        player.takeoff_y=player.feet_y;
        player.jump_height=0;
        player.turn=0;
        player.flip_state=FLIP_OFF;
        if(keys.jump_pressed){
            player.speed_y-=player.jump_speed;  /* 加强跳只改起跳力度，长按仍然控制后翻。 */
            player.flip_state=FLIP_WAIT;  /* 先起跳，给手指留出松键时间。 */
        }
    }else if(keys.jump_pressed || (player.flip_state==FLIP_WAIT && keys.long_press)){
        /* 上一帧就已经在空中：重新按空格，或起跳键一直按到了长按时间。 */
        player.flip_state=FLIP_TURN;
    }

    /* 空中先改速度，再改位置。平地起跳速度是 -10，第一帧就往上走 9.5 像素。 */
    float old_y=player.feet_y;
    player.speed_y+=0.5f;
    player.feet_y+=player.speed_y;
    float rise=player.takeoff_y-player.feet_y;  /* 脚底坐标越小，人升得越高。 */
    if(rise>player.jump_height) player.jump_height=rise;
    if(player.flip_state==FLIP_TURN && (keys.space_down || keys.jump_pressed)){
        player.angle-=9;  /* 即使空中的第二次轻点在一帧内就松开，也能转动这一下。 */
        player.turn+=9;
    }
    if(!keys.space_down) player.flip_state=FLIP_OFF;  /* 松手就停，已经转过的角度保留。 */

    if(!ground.has_ground || player.feet_y<ground.y){
        /* 还没碰到雪面，只要人还在画面里，就接着飞。 */
        if(player.feet_y<=WINDOW_HEIGHT) return player;
    }else if(use_ramp || old.has_ground || old_y<=ground.y){
        /* 从上方落到雪面才检查板角。从沟里横着撞到崖壁，直接走下面的摔倒处理。 */
        player.feet_y=ground.y;
        float snow_angle=slope_angle(ground.slope);
        /* 转过一圈板头又回到原方向，比如 -355 度与平地只差 5 度。 */
        float difference=remainderf(player.angle-snow_angle,360.0f);
        if(fabsf(difference)<=40){
            player.on_ground=1;
            player.on_ramp=use_ramp;
            player.flip_state=FLIP_OFF;
            player.angle=snow_angle;
            player.speed_y=0;
            if(flip_count(player)>0) player.event=LAND_FLIP;
            return player;
        }
    }

    /* 掉出画面、撞到崖壁、落地角度太大，三种情况都只报一次摔倒。 */
    player.crashed=1;
    player.flip_state=FLIP_OFF;
    player.event=LAND_CRASH;
    return player;
}
