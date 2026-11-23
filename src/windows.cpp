#include "game.h"
#include <graphics.h>
#include <windows.h>

#include <mmsystem.h>

extern "C" bool platform_init(Platform *platform, const wchar_t *title, int width, int height)
{
    HWND hwnd;
    RECT rect;
    int window_width;
    int window_height;
    int x;
    int y;

    if (platform == 0 || width <= 0 || height <= 0) {
        return false;
    }

    platform->initialized = false;
    platform->quit = false;
    platform->width = width;
    platform->height = height;
    platform->hwnd = 0;
    platform->start_time = 0.0;

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

extern "C" void platform_shutdown(Platform *platform)
{
    if (platform == 0) {
        return;
    }

    if (platform->initialized) {
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

extern "C" void platform_pump(Platform *platform)
{
    if (platform == 0 || !platform->initialized) {
        return;
    }
    if (platform->hwnd == 0 || !IsWindow(static_cast<HWND>(platform->hwnd))) {
        platform->quit = true;
    }
}

extern "C" void platform_present(Platform *platform)
{
    if (platform != 0 && platform->initialized && !platform->quit) {
        FlushBatchDraw();
    }
}

extern "C" bool platform_should_quit(const Platform *platform)
{
    return (platform == 0) || platform->quit;
}

#include <graphics.h>
#include <windows.h>

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

    while (peekmessage(&message, EX_KEY | EX_MOUSE | EX_WINDOW)) {
        switch (message.message) {
        case WM_KEYDOWN:
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

    input->jump_held = ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0);

    hwnd = static_cast<HWND>(platform->hwnd);
    if (hwnd == 0 || !IsWindow(hwnd)) {
        platform->quit = true;
    }
}

#include <windows.h>

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

extern "C" double platform_now_seconds(const Platform *platform)
{
    LARGE_INTEGER counter;

    if (platform == 0 || platform->qpc_frequency <= 0.0) {
        return 0.0;
    }

    QueryPerformanceCounter(&counter);
    return (double)(counter.QuadPart - platform->qpc_origin) / platform->qpc_frequency;
}

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

#pragma pack(push, 1)
typedef struct HighScoreRecord {
    char magic[8];
    int32_t value;
    uint32_t checksum;
    uint32_t version;
} HighScoreRecord;
#pragma pack(pop)

static const char HIGH_SCORE_MAGIC[8] = {'S', 'P', 'H', 'S', '0', '0', '0', '1'};
static const uint32_t HIGH_SCORE_VERSION = 1u;

static uint32_t highscore_checksum(int32_t value)
{
    return ((uint32_t)value * 2654435761u) ^ 0xA5A5C0DEu;
}

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
    slash[1] = L'\0';

    if (wcsncat_s(buffer, capacity, filename, _TRUNCATE) != 0) {
        return false;
    }
    return true;
}

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
    read_count = fread(&record, 1, sizeof(record), file);
    fclose(file);

    if (read_count != sizeof(record)) {
        return false;
    }
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

    if (!MoveFileExW(temp_path, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp_path);
        return false;
    }
    return true;
}

#ifdef _DEBUG
#include <crtdbg.h>
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <sys/stat.h>
#endif

extern "C" void platform_crt_init(void)
{
#ifdef _DEBUG
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int leak_fd;

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
