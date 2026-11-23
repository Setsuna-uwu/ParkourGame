#include "game.h"

/* TODO #7：先在平地做空格起跳和重力，再用同一段固定坡测试离地、翻转、落地。
   目前只摆放方块。加入运动时要保存上一帧的高度、速度和姿态。 */
float update_player(float distance)
{
    Ground ground = ground_at(distance);
    return ground.has_ground ? ground.y : WINDOW_HEIGHT;
}
