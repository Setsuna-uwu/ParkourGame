#include <easyx.h>
#include <math.h>
#include <wchar.h>
#include "game.h"

int open_window(void){
    if(!initgraph(WINDOW_WIDTH,WINDOW_HEIGHT)) return 0;
    setbkcolor(RGB(35,49,71));  // 深蓝天空也透过雪沟显示出来。
    setbkmode(TRANSPARENT);
    BeginBatchDraw();  // 先画完整帧，再一次显示。
    return 1;
}

Keys read_key(void){
    static int space_down=0;  // 两帧之间也要记住空格是否还按着。
    static DWORD pressed_at=0;  // 第一次按下的时刻，系统按毫秒计时。
    Keys keys={};
    ExMessage message;
    while(peekmessage(&message,EX_KEY | EX_WINDOW)){
        if(message.message==WM_CLOSE || message.message==WM_DESTROY) keys.quit=1;
        if(message.message==WM_ACTIVATE && LOWORD(message.wParam)==WA_INACTIVE){
            space_down=0;  // 切到别的窗口时可能收不到松键消息。
        }
        if(message.message==WM_KEYDOWN && !message.prevdown){
            if(message.vkcode==VK_ESCAPE) keys.quit=1;
            if(message.vkcode=='R') keys.retry=1;
            if(message.vkcode==VK_F2) keys.reseed=1;
            if(message.vkcode==VK_SPACE){
                space_down=1;
                pressed_at=GetTickCount();  // 长按产生的重复消息不能重新计时。
                keys.jump_pressed=1;
            }
        }
        if(message.message==WM_KEYUP && message.vkcode==VK_SPACE) space_down=0;
    }
    keys.space_down=space_down;
    keys.long_press=space_down && GetTickCount()-pressed_at>=HOLD_TO_FLIP_MS;
    if(!IsWindow(GetHWnd())) keys.quit=1;
    return keys;
}

// 把脚底当作原点。forward 表示沿板向前多少，up 表示从板面向上多少。
// 板头水平时，前方就是屏幕右方，上方就是屏幕 y 减小的方向。
static POINT board_point(Player player, float forward, float up){
    float rad=player.angle*3.14159265f/180;  // sin 和 cos 用弧度。
    POINT point;
    point.x=(LONG)(PLAYER_X+forward*cosf(rad)+up*sinf(rad));
    point.y=(LONG)(player.feet_y+forward*sinf(rad)-up*cosf(rad));
    return point;
}

static void draw_snow(float distance){
    // 角色脚底的横坐标固定在 PLAYER_X。若往右看 20 像素，就查距离玩家前方 20 像素的雪面。
    // 玩家沿雪道前进 4 像素，同一个崖沿在屏幕上就往左移 4 像素。
    setlinestyle(PS_SOLID,1);
    setlinecolor(RGB(119,153,177));
    for(int x=0;x<WINDOW_WIDTH;x++){
        float world_x=distance+x-PLAYER_X;
        Ground ground=ground_at(world_x);
        if(ground.has_ground){
            int y=(int)ground.y;
            line(x,y,x,WINDOW_HEIGHT);  // 只填雪地，板下的空隙要透出天空。
            putpixel(x,y,WHITE);
        }
        Ground ramp=ramp_at(world_x);
        if(ramp.has_ground){
            int y=(int)ramp.y;
            int plank=(int)world_x%24;
            setlinecolor(RGB(97,66,58));
            if(plank<3 && ground.has_ground) line(x,y,x,(int)ground.y);
            int brace_y=y+5+plank/2;
            if(!ground.has_ground || brace_y+3<ground.y) line(x,brace_y,x,brace_y+3);
            setlinecolor(RGB(196,143,99));
            line(x,y+1,x,y+5);
            putpixel(x,y,WHITE);
            setlinecolor(RGB(119,153,177));
        }
    }
}

static void draw_items(Player player){
    int count,i;
    const Pickup *items=terrain_pickups(&count);
    for(i=0;i<count;i++){
        int x=PLAYER_X+(int)(items[i].distance-player.distance);
        int y=(int)items[i].y;
        if(x<-20 || x>WINDOW_WIDTH+20) continue;
        if(items[i].kind==ITEM_COIN){
            setfillcolor(RGB(255,205,55));
            solidcircle(x,y,5);
            setlinecolor(RGB(255,245,180));
            circle(x,y,6);
        }else if(items[i].kind==ITEM_LIFE){
            setfillcolor(RGB(245,80,95));
            solidrectangle(x-6,y-6,x+6,y+6);
            setlinecolor(WHITE);
            line(x-3,y,x+3,y);
            line(x,y-3,x,y+3);
        }else if(items[i].kind==ITEM_ROCK){
            setfillcolor(RGB(85,92,105));
            solidellipse(x-9,y-5,x+9,y+7);
            setlinecolor(RGB(160,170,185));
            line(x-6,y-4,x+3,y+4);
        }
    }
}

void draw_frame(Player player){
    cleardevice();
    draw_snow(player.distance);
    draw_items(player);
    // 人和板共用一个角度。橙色翘头朝前，空中也能看清翻到了哪里。
    POINT tail=board_point(player,-15,0);
    POINT nose=board_point(player,13,0);
    POINT tip=board_point(player,18,4);
    POINT feet=board_point(player,0,0);
    POINT hip=board_point(player,-4,9);
    POINT head=board_point(player,1,20);
    setlinestyle(PS_SOLID,3);
    setlinecolor(WHITE);
    line(feet.x,feet.y,hip.x,hip.y);
    line(hip.x,hip.y,head.x,head.y);
    setfillcolor(WHITE);
    solidcircle(head.x,head.y,4);
    setlinecolor(RGB(75,190,255));
    line(tail.x,tail.y,nose.x,nose.y);
    setlinecolor(RGB(255,155,55));
    line(nose.x,nose.y,tip.x,tip.y);

    outtextxy(12,10,_T("轻点空格跳；长按或空中再按后翻，松开停转    R 重试    F2 换雪道    Esc 退出"));
    wchar_t coin_text[32];
    swprintf(coin_text,32,L"金币 %d",terrain_coin_count());
    settextcolor(RGB(255,210,80));
    outtextxy(WINDOW_WIDTH-12-textwidth(coin_text),10,coin_text);
    if(player.crashed) outtextxy(12,36,_T("摔倒了，按 R 重试本条，或按 F2 换一条"));
    else if(player.on_ground && flip_count(player)>0){
        wchar_t message[48];
        swprintf(message,48,L"后翻 %d 圈，站稳了！",flip_count(player));
        outtextxy(12,36,message);
    }
    {
        wchar_t seed_text[96];
        if(terrain_seed()==0){
            swprintf(seed_text,96,L"雪道：固定样例（种子 0）    按 F2 生成随机雪道");
        }else{
            swprintf(seed_text,96,L"雪道种子：%u    按 R 重试本条，按 F2 换一条",terrain_seed());
        }
        settextcolor(RGB(215,230,245));
        outtextxy(12,WINDOW_HEIGHT-24,seed_text);
    }
    // TODO #9：把本局分数、备用命和是否已结算传到这里显示。计分、扣命和存纪录放在 C 文件里。
    FlushBatchDraw();
    Sleep(16);
}

void close_window(void){
    if(IsWindow(GetHWnd())){
        EndBatchDraw();
        closegraph();
    }
}
