#include "game.h"

/* TODO #8：先按 R 多试几次这条固定坡，用它对照新雪道的落点。
   接随机时，每段只生成一次。画面和角色会反复查同一个距离，这里每次应返回同一个高度。
   地表由 ground_at 查，独立跳台由 ramp_at 查。物品编号和安全复活位置约好后交给 #9。 */
Ground ground_at(float distance){
    Ground ground={1,250,0};  /* 起点是平地，雪面在屏幕 y=250。 */
    if(distance<300) return ground;

    if(distance<700){
        ground.slope=0.25f;   /* 向右走 4 像素，雪面往下 1 像素。 */
        ground.y=250+(distance-300)*ground.slope;
    }else if(distance<750){
        ground.has_ground=0;  /* 地表在 700 到 750 之间断开。 */
    }else if(distance<1150){
        ground.slope=0.125f;  /* 对岸低一些，留出飞行和后翻的空间。 */
        ground.y=345+(distance-750)*ground.slope;
    }else{
        ground.y=395;         /* 落坡后接平地，按 R 可以再试同一个断崖。 */
    }
    return ground;
}

/* #8 的跳台入口：现在是直板，入口与雪地相接，末端架空。
   换成弧形时，一起改板面的 y 和该点的 slope；起飞时会沿板头的坡度飞出。 */
Ground ramp_at(float distance){
    Ground ramp={0};
    if(distance>=560 && distance<680){
        ramp.has_ground=1;
        ramp.slope=-0.25f;
        ramp.y=315+(distance-560)*ramp.slope;
    }
    return ramp;
}
