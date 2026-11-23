#include "game.h"
#include <graphics.h>
#include <windows.h>

#include <mmsystem.h>

/* 创建 EasyX 窗口并把窗口信息存到 platform。extern "C" 让 C++ 编译器按 C 的链接方式导出函数，main.c 就能调用。 */
extern "C" bool platform_init(Platform *platform, const wchar_t *title, int width, int height)
{
    /* HWND 是 Windows 用来辨认某个窗口的句柄。后面改窗口标题和位置时，都传入这个值。 */
    HWND hwnd;
    RECT rect;
    int window_width;
    int window_height;
    int x;
    int y;

    if (platform == 0 || width <= 0 || height <= 0) {
        return false;
    }

    /* platform 是变量的地址。platform->width 等同于 (*platform).width，即沿着地址改调用者的窗口信息。 */
    platform->initialized = false;
    platform->quit = false;
    platform->width = width;
    platform->height = height;
    platform->hwnd = 0;
    platform->start_time = 0.0;

    /* 按屏幕像素处理显示缩放，并向系统申请 1 毫秒的计时分辨率；关闭时用 timeEndPeriod 配对释放。 */
    SetProcessDPIAware();
    timeBeginPeriod(1);

    hwnd = initgraph(width, height);
    if (hwnd == 0) {
        timeEndPeriod(1);
        return false;
    }

    if (title != 0) {
        SetWindowTextW(hwnd, title);
    }


    /* 把窗口边界写进 rect，两个方向分别用（屏幕尺寸减窗口尺寸）除以二来居中。 */
    GetWindowRect(hwnd, &rect);
    window_width = rect.right - rect.left;
    window_height = rect.bottom - rect.top;
    x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    /* 竖线 | 把两个选项合起来：保留窗口大小，同时把窗口显示出来。 */
    SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    setbkcolor(RGB(9, 13, 30));
    cleardevice();
    BeginBatchDraw();

    platform->hwnd = hwnd;
    platform->initialized = true;
    platform_time_init(platform);
    platform->start_time = platform_now_seconds(platform);

    return true;
}

/* 关闭图形窗口并结束计时设置，然后清空保存的句柄。 */
extern "C" void platform_shutdown(Platform *platform)
{
    if (platform == 0) {
        return;
    }

    if (platform->initialized) {
        /* 为使 C 文件不用引入 Windows 头文件，结构体中的 hwnd 存为 void *。这里转回 Windows 所需的 HWND。 */
        HWND hwnd = static_cast<HWND>(platform->hwnd);
        if (hwnd != 0 && IsWindow(hwnd)) {
            EndBatchDraw();
            closegraph();
        }
        timeEndPeriod(1);
    }

    platform->initialized = false;
    platform->hwnd = 0;
}

/* 检查先前保存的窗口是否还存在。窗口已经被关掉时就在共享状态里记下退出请求。 */
extern "C" void platform_pump(Platform *platform)
{
    if (platform == 0 || !platform->initialized) {
        return;
    }
    if (platform->hwnd == 0 || !IsWindow(static_cast<HWND>(platform->hwnd))) {
        platform->quit = true;
    }
}

/* 把这一帧在后台画好的图一次显示到窗口上，避免玩家看到半幅画面。 */
extern "C" void platform_present(Platform *platform)
{
    if (platform != 0 && platform->initialized && !platform->quit) {
        FlushBatchDraw();
    }
}

/* 主循环用它判断是否退出。|| 从左往右判断，空指针时不会继续访问 platform->quit。 */
extern "C" bool platform_should_quit(const Platform *platform)
{
    return (platform == 0) || platform->quit;
}

#include <graphics.h>
#include <windows.h>

/* 清空上一帧输入，取完本帧积攒的窗口消息，把按键和点击写回调用者的 Input 变量。 */
extern "C" void platform_poll_input(Platform *platform, Input *input)
{
    ExMessage message;
    HWND hwnd;

    if (input == 0) {
        return;
    }
    input_reset(input);

    if (platform == 0 || !platform->initialized) {
        return;
    }

    /* | 同时选择键盘、鼠标和窗口消息。每次成功取一条就进入循环，取空为止。 */
    while (peekmessage(&message, EX_KEY | EX_MOUSE | EX_WINDOW)) {
        switch (message.message) {
        case WM_KEYDOWN:
            /* 按住键时系统也会重复发送消息。只有之前没按下的那次才算新按键，避免长按空格连跳。 */
            if (!message.prevdown) {
                switch (message.vkcode) {
                case VK_SPACE:
                    input->jump_pressed = true;
                    break;
                case 'P':
                    input->pause_pressed = true;
                    break;
                case 'R':
                    input->restart_pressed = true;
                    break;
                case VK_ESCAPE:
                    input->quit_pressed = true;
                    break;
                case VK_F1:
                    input->debug_pressed = true;
                    break;
                default:
                    break;
                }
            }
            break;
        case WM_LBUTTONDOWN:
            input->mouse_clicked = true;
            break;
        case WM_CLOSE:
        case WM_DESTROY:
            platform->quit = true;
            break;
        default:
            break;
        }
    }

    /* & 0x8000 是按位与，检查返回值最高位。该位为 1 表示查询的这一刻空格仍按着。 */
    input->jump_held = ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0);

    hwnd = static_cast<HWND>(platform->hwnd);
    if (hwnd == 0 || !IsWindow(hwnd)) {
        platform->quit = true;
    }
}

#include <windows.h>

/* 查询高精度时钟每秒跳多少次，同时记下起始读数，作为本次运行的零点。 */
extern "C" void platform_time_init(Platform *platform)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (platform == 0) {
        return;
    }

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);

    platform->qpc_frequency = (double)frequency.QuadPart;
    platform->qpc_origin = counter.QuadPart;
    platform->start_time = 0.0;
}

/* 用两次计数之差除以每秒的计数次数，得到从初始化至今过去的秒数。 */
extern "C" double platform_now_seconds(const Platform *platform)
{
    LARGE_INTEGER counter;

    if (platform == 0 || platform->qpc_frequency <= 0.0) {
        return 0.0;
    }

    QueryPerformanceCounter(&counter);
    return (double)(counter.QuadPart - platform->qpc_origin) / platform->qpc_frequency;
}

/* 算出应当结束等待的时刻。离目标还远时交出 CPU，接近目标时反复检查时钟。 */
extern "C" void platform_sleep_seconds(double seconds)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    double target;
    double now;
    double remain;

    if (seconds <= 0.0) {
        return;
    }

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    target = (double)counter.QuadPart / (double)frequency.QuadPart + seconds;

    /* for 的三个部分都留空表示一直循环；到达目标时间后，下面的 break 才结束它。 */
    for (;;) {
        QueryPerformanceCounter(&counter);
        now = (double)counter.QuadPart / (double)frequency.QuadPart;
        remain = target - now;
        if (remain <= 0.0) {
            break;
        }
        if (remain > 0.002) {
            Sleep(1);
        } else {
            /* 最后 2ms 自旋，避免 Sleep 粒度导致帧率抖动。 */
            YieldProcessor();
        }
    }
}

#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* 先记住原来的内存对齐设置，再让这个存档结构体按 1 字节对齐，使记录中的字段紧挨着存放。 */
#pragma pack(push, 1)
typedef struct HighScoreRecord {
    char magic[8];
    int32_t value;
    uint32_t checksum;
    uint32_t version;
} HighScoreRecord;
/* 恢复之前的对齐设置，这个存档格式的规则只作用于上面的结构体。 */
#pragma pack(pop)

static const char HIGH_SCORE_MAGIC[8] = {'S', 'P', 'H', 'S', '0', '0', '0', '1'};
static const uint32_t HIGH_SCORE_VERSION = 1u;

/* 把分数算成 32 位校验值。读档时按同一公式重算并比较，可以发现分数与校验值不一致。 */
static uint32_t highscore_checksum(int32_t value)
{
    return ((uint32_t)value * 2654435761u) ^ 0xA5A5C0DEu;
}

/* 在 buffer 里拼出 exe 同目录下的文件路径。capacity 是数组能容纳的宽字符个数。 */
static bool executable_path(wchar_t *buffer, size_t capacity, const wchar_t *filename)
{
    DWORD length;
    wchar_t *slash;

    if (buffer == 0 || capacity == 0 || filename == 0) {
        return false;
    }

    length = GetModuleFileNameW(0, buffer, (DWORD)capacity);
    if (length == 0 || (size_t)length >= capacity) {
        return false;
    }

    slash = wcsrchr(buffer, L'\\');
    if (slash == 0) {
        return false;
    }
    /* slash 指向最后一个目录分隔符。把它后面的第一个字符设为字符串结束符，再接上新的文件名。 */
    slash[1] = L'\0';

    if (wcsncat_s(buffer, capacity, filename, _TRUNCATE) != 0) {
        return false;
    }
    return true;
}

/* 读取并核对最高分存档。通过检查才返回 true，并用 *out_value 把分数写回调用者传来的变量。 */
extern "C" bool platform_storage_load_highscore(int *out_value)
{
    wchar_t path[MAX_PATH];
    FILE *file = 0;
    HighScoreRecord record;
    size_t read_count;

    if (out_value == 0) {
        return false;
    }
    *out_value = 0;

    if (!executable_path(path, MAX_PATH, L"highscore.dat")) {
        return false;
    }
    if (_wfopen_s(&file, path, L"rb") != 0 || file == 0) {
        return false;
    }

    memset(&record, 0, sizeof(record));
    /* 每次读 1 字节，总共要读 sizeof(record) 个；少读就说明这条记录不完整。 */
    read_count = fread(&record, 1, sizeof(record), file);
    fclose(file);

    if (read_count != sizeof(record)) {
        return false;
    }
    /* memcmp 比较文件开头的 8 个标记字节，相同返回 0；随后再检查版本、校验值和分数范围。 */
    if (memcmp(record.magic, HIGH_SCORE_MAGIC, sizeof(HIGH_SCORE_MAGIC)) != 0) {
        return false;
    }
    if (record.version != HIGH_SCORE_VERSION) {
        return false;
    }
    if (record.checksum != highscore_checksum(record.value)) {
        return false;
    }
    if (record.value < 0 || record.value > 100000000) {
        return false;
    }

    *out_value = (int)record.value;
    return true;
}

/* 先把标记、分数和校验值一起写进临时文件。写完整才用它替换旧纪录。 */
extern "C" bool platform_storage_save_highscore(int value)
{
    wchar_t path[MAX_PATH];
    wchar_t temp_path[MAX_PATH];
    FILE *file = 0;
    HighScoreRecord record;
    size_t written;
    bool ok;

    if (value < 0) {
        value = 0;
    }
    if (value > 100000000) {
        value = 100000000;
    }

    if (!executable_path(path, MAX_PATH, L"highscore.dat")) {
        return false;
    }
    if (!executable_path(temp_path, MAX_PATH, L"highscore.tmp")) {
        return false;
    }

    memset(&record, 0, sizeof(record));
    memcpy(record.magic, HIGH_SCORE_MAGIC, sizeof(HIGH_SCORE_MAGIC));
    record.value = (int32_t)value;
    record.checksum = highscore_checksum(record.value);
    record.version = HIGH_SCORE_VERSION;

    if (_wfopen_s(&file, temp_path, L"wb") != 0 || file == 0) {
        return false;
    }

    written = fwrite(&record, 1, sizeof(record), file);
    ok = (written == sizeof(record));
    if (ok) {
        ok = (fflush(file) == 0);
    }
    fclose(file);

    if (!ok) {
        DeleteFileW(temp_path);
        return false;
    }

    /* 两个标志合用，允许覆盖同名旧存档并要求写透。替换失败就清理这次的临时文件。 */
    if (!MoveFileExW(temp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp_path);
        return false;
    }
    return true;
}

/* 仅在定义了 _DEBUG 的构建中包含这些调试库头文件；下面的 CRT 检查也受同一条件控制。 */
#ifdef _DEBUG
#include <crtdbg.h>
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <sys/stat.h>
#endif

/* 配置 C 运行库的调试报告：在调试配置下，进程退出时会检查仍未释放的内存。 */
extern "C" void platform_crt_init(void)
{
#ifdef _DEBUG
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int leak_fd;

    /* |= 把指定标志位设为 1，之前已经启用的其他选项继续生效。 */
    flags |= _CRTDBG_ALLOC_MEM_DF;
    flags |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(flags);

    /*
     * GUI 程序没有控制台。把 CRT 警告（包括退出时的泄漏报告）写入
     * 工作目录下的 leak_report.txt，便于 harness 直接检查。
     * fd 故意保持打开，由 CRT 在进程退出时完成泄漏转储。
     */
    leak_fd = _open("leak_report.txt", _O_CREAT | _O_WRONLY | _O_TRUNC, _S_IREAD | _S_IWRITE);
    if (leak_fd != -1) {
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, (_HFILE)(intptr_t)leak_fd);
    } else {
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    }
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
}
