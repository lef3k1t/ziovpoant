#include <windows.h>
#include <shellapi.h>

#include <memory>
#include <string>

namespace
{
constexpr wchar_t kWindowClassName[] = L"ZIOVPOANT.MainWindow";
constexpr wchar_t kWindowTitle[] = L"ZIOVPOANT";
constexpr wchar_t kMutexName[] = L"Local\\ZIOVPOANT.Singleton";
constexpr wchar_t kTrayTooltip[] = L"ZIOVPOANT";

constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kRestoreMessage = WM_APP + 2;

constexpr UINT_PTR kMenuFileExit = 1001;
constexpr UINT_PTR kMenuTrayOpen = 2001;
constexpr UINT_PTR kMenuTrayExit = 2002;

HINSTANCE g_instance = nullptr;
HANDLE g_singleInstanceMutex = nullptr;
UINT g_taskbarCreatedMessage = 0;
bool g_isExiting = false;

bool IsHiddenLaunch()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
    {
        return false;
    }

    const std::unique_ptr<wchar_t*, decltype(&LocalFree)> argvHolder(argv, LocalFree);

    for (int index = 1; index < argc; ++index)
    {
        const std::wstring argument = argv[index];
        if (argument == L"--hidden" || argument == L"/hidden" ||
            argument == L"--tray-only" || argument == L"/tray-only")
        {
            return true;
        }
    }

    return false;
}

void ShowMainWindow(HWND window)
{
    ShowWindow(window, IsIconic(window) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(window);
}

void HideMainWindow(HWND window)
{
    ShowWindow(window, SW_HIDE);
}

HMENU CreateMainMenu()
{
    HMENU mainMenu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();

    AppendMenuW(fileMenu, MF_STRING, kMenuFileExit, L"\u0412\u044B\u0445\u043E\u0434");
    AppendMenuW(mainMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"\u0424\u0430\u0439\u043B");

    return mainMenu;
}

bool UpdateTrayIcon(HWND window, DWORD message)
{
    NOTIFYICONDATAW trayIcon{};
    trayIcon.cbSize = sizeof(trayIcon);
    trayIcon.hWnd = window;
    trayIcon.uID = kTrayIconId;
    trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayIcon.uCallbackMessage = kTrayCallbackMessage;
    trayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(trayIcon.szTip, kTrayTooltip, ARRAYSIZE(trayIcon.szTip));

    return Shell_NotifyIconW(message, &trayIcon) == TRUE;
}

void RemoveTrayIcon(HWND window)
{
    NOTIFYICONDATAW trayIcon{};
    trayIcon.cbSize = sizeof(trayIcon);
    trayIcon.hWnd = window;
    trayIcon.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &trayIcon);
}

void ShowTrayMenu(HWND window)
{
    POINT cursor{};
    GetCursorPos(&cursor);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kMenuTrayOpen, L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuTrayExit, L"\u0412\u044B\u0445\u043E\u0434");

    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        cursor.x, cursor.y, 0, window, nullptr);
    DestroyMenu(menu);
}

void ExitApplication(HWND window)
{
    g_isExiting = true;
    PostMessageW(window, WM_CLOSE, 0, 0);
}

void NotifyExistingInstance(bool shouldShowWindow)
{
    HWND existingWindow = FindWindowW(kWindowClassName, nullptr);
    if (!existingWindow)
    {
        return;
    }

    if (shouldShowWindow)
    {
        PostMessageW(existingWindow, kRestoreMessage, 0, 0);
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_taskbarCreatedMessage)
    {
        UpdateTrayIcon(window, NIM_ADD);
        return 0;
    }

    switch (message)
    {
    case WM_CREATE:
        SetMenu(window, CreateMainMenu());
        UpdateTrayIcon(window, NIM_ADD);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case kMenuFileExit:
        case kMenuTrayExit:
            ExitApplication(window);
            return 0;

        case kMenuTrayOpen:
            ShowMainWindow(window);
            return 0;

        default:
            break;
        }
        break;

    case WM_CLOSE:
        if (!g_isExiting)
        {
            HideMainWindow(window);
            return 0;
        }
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon(window);
        if (g_singleInstanceMutex)
        {
            ReleaseMutex(g_singleInstanceMutex);
            CloseHandle(g_singleInstanceMutex);
            g_singleInstanceMutex = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    case kRestoreMessage:
        ShowMainWindow(window);
        return 0;

    case kTrayCallbackMessage:
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONUP:
            ShowMainWindow(window);
            return 0;

        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu(window);
            return 0;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    g_instance = instance;
    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    const bool hiddenLaunch = IsHiddenLaunch();

    g_singleInstanceMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!g_singleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        NotifyExistingInstance(!hiddenLaunch);
        if (g_singleInstanceMutex)
        {
            CloseHandle(g_singleInstanceMutex);
            g_singleInstanceMutex = nullptr;
        }
        return 0;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&windowClass))
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
        return 1;
    }

    HWND window = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        500,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!window)
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
        return 1;
    }

    if (!hiddenLaunch)
    {
        ShowWindow(window, showCommand == SW_HIDE ? SW_SHOW : showCommand);
        UpdateWindow(window);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
