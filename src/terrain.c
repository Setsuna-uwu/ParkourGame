#include "game.h"

/* TODO #8：先用固定距离区间表示缓坡、断崖和落坡，供 #7 反复试同一跳；
   验证后再让这些坡段随机出现。角色和画面都调用这个函数。 */
Ground ground_at(float distance)
{
    (void)distance;  /* 当前处处是平地；按距离选坡段时去掉这一句。 */
    Ground ground = {1, 330, 0};
    return ground;
}
