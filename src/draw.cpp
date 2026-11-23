#include <easyx.h>
#include "game.h"

int open_window(void)
{
    if (!initgraph(WINDOW_WIDTH, WINDOW_HEIGHT)) return 0;
    BeginBatchDraw();  // 先画完整帧，再一次显示。
    return 1;
}

int wants_to_quit(void)
{
    ExMessage message;
    while (peekmessage(&message, EX_KEY | EX_WINDOW)) {
        if (message.message == WM_KEYDOWN && message.vkcode == VK_ESCAPE) return 1;
        if (message.message == WM_CLOSE || message.message == WM_DESTROY) return 1;
        // TODO #7：在这里读取空格的按下和松开，再把结果交给角色逻辑。
    }
    return !IsWindow(GetHWnd());
}

void draw_frame(float distance, float feet_y)
{
    cleardevice();
    // 屏幕 x 对应雪道上的 distance + x - PLAYER_X。
    for (int x = 0; x < WINDOW_WIDTH; ++x) {
        Ground ground = ground_at(distance + x - PLAYER_X);
        if (ground.has_ground) putpixel(x, (int)ground.y, WHITE);
    }
    // TODO #7：方块底边代表脚底。旋转雪板时仍以脚底对齐落坡。
    solidrectangle(PLAYER_X - 8, (int)feet_y - 16, PLAYER_X + 8, (int)feet_y);
    // TODO #9：在这里显示分数、生命和结束提示，判定规则写在 C 文件里。
    FlushBatchDraw();
    Sleep(16);
}

void close_window(void)
{
    if (IsWindow(GetHWnd())) {
        EndBatchDraw();
        closegraph();
    }
}
