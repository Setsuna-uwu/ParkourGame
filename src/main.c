#include "game.h"

/* 一次循环走一步。每秒约走 60 步，跳跃和障碍都按这个节奏计算。 */
int main(void)
{
    /* window 保存窗口和计时器的数据；keys 保存这一步读到的按键。 */
    Platform window;
    Input keys;
    int record = 0;
    double next_frame;
    double last_frame;
    double fps = SP_TARGET_FPS;

    platform_crt_init();
    /* L 开头的字符串用宽字符保存中文标题。初始化返回真假，前面的 ! 表示取反，建窗失败就退出。 */
    if (!platform_init(&window, L"雪原跑酷", SP_SCREEN_WIDTH, SP_SCREEN_HEIGHT)) {
        return 1;
    }
    /* 给地形随机数一个固定起点，方便复现。0x 表示十六进制，末尾 u 表示无符号整数。 */
    game_init(0x5EED2026u);
    /* 读档成功才会走进大括号。&record 让读档函数把分数写回 main 里的变量。 */
    if (platform_storage_load_highscore(&record)) {
        score_set_high_score(&game.score, record);
    }

    next_frame = platform_now_seconds(&window);
    last_frame = next_frame;
    /* && 表示两边都要成立：游戏还在运行，并且玩家也没有关掉窗口，就继续循环。 */
    while (!game.quit_requested && !platform_should_quit(&window)) {
        double now = platform_now_seconds(&window);
        if (now - last_frame > 0.001) {
            /* 两帧相隔 0.02 秒时，瞬时帧率就是 1 / 0.02 = 50。显示值每次向它靠近一成，避免数字跳动。 */
            fps += 0.1 * (1.0 / (now - last_frame) - fps);
        }
        last_frame = now;

        /* &keys 是变量的地址，读键函数往这个变量里填结果。 */
        platform_poll_input(&window, &keys);
        if (platform_should_quit(&window)) {
            break;
        }
        /* 从这里跳到 game.c。它用本步按键更新角色、障碍和分数，下面才会画出更新后的画面。 */
        game_update(keys);
        if (game.score.high_score_dirty) {
            /* dirty 表示有新纪录待写盘。保存成功返回 true，取反后标志清零；失败就留到下一轮再试。 */
            game.score.high_score_dirty = !platform_storage_save_highscore(game.score.high_score);
        }
        /* 先在后台画好完整的一帧，present 再把成品一次显示出来。 */
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
