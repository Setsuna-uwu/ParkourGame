#pragma once  /* 每个源文件只读一次这份公共声明。 */
#ifdef __cplusplus
extern "C" {  /* 让 draw.cpp 里的函数能和 C 文件互相调用。 */
#endif

enum { WINDOW_WIDTH = 800, WINDOW_HEIGHT = 450, PLAYER_X = 160 };

typedef struct {
    int has_ground;  /* 1 有雪面；0 是沟，此时不要读下面两个值。 */
    float y;         /* 雪面高度，单位像素，向下增大。 */
    float slope;     /* y 随水平距离的变化率；向右下坡为正。 */
} Ground;

Ground ground_at(float distance);  /* 世界距离，从起点向右增加。 */
float update_player(float distance);

int open_window(void);
int wants_to_quit(void);
void draw_frame(float distance, float feet_y);
void close_window(void);

#ifdef __cplusplus
}
#endif
