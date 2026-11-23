#include "game.h"

/* 一次循环走一步。每秒约走 60 步，跳跃和障碍都按这个节奏计算。 */
int main(void)
{
    Platform window;
    Input keys;
    int record = 0;
    double next_frame;
    double last_frame;
    double fps = SP_TARGET_FPS;

    platform_crt_init();
    if (!platform_init(&window, L"雪原跑酷", SP_SCREEN_WIDTH, SP_SCREEN_HEIGHT)) {
        return 1;
    }
    game_init(0x5EED2026u);
    if (platform_storage_load_highscore(&record)) {
        score_set_high_score(&game.score, record);
    }

    next_frame = platform_now_seconds(&window);
    last_frame = next_frame;
    while (!game.quit_requested && !platform_should_quit(&window)) {
        double now = platform_now_seconds(&window);
        if (now - last_frame > 0.001) {
            fps += 0.1 * (1.0 / (now - last_frame) - fps);
        }
        last_frame = now;

        /* &keys 是变量的地址，读键函数往这个变量里填结果。 */
        platform_poll_input(&window, &keys);
        if (platform_should_quit(&window)) {
            break;
        }
        game_update(keys);
        if (game.score.high_score_dirty) {
            game.score.high_score_dirty = !platform_storage_save_highscore(game.score.high_score);
        }
        platform_render(&window, &game, fps);
        platform_present(&window);

        /* 到点再画下一帧。拖动窗口等长时间卡顿后从当前时间接着走。 */
        next_frame += SP_FIXED_DT;
        now = platform_now_seconds(&window);
        if (now - next_frame > 8 * SP_FIXED_DT) {
            next_frame = now;
        }
        platform_sleep_seconds(next_frame - now);
    }
    platform_shutdown(&window);
    return 0;
}
