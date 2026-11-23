#include "game.h"

int main(void)
{
    float distance = 0;  /* 玩家在雪道上的水平位置。 */
    if (!open_window()) return 1;

    while (!wants_to_quit()) {
        /* TODO #9：从这里接入开局、摔倒、结算和重开；重开时清掉本局数据。 */
        float feet_y = update_player(distance);
        draw_frame(distance, feet_y);
    }

    close_window();
    return 0;
}
