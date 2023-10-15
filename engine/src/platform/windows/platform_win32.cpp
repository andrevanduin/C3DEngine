
#ifdef C3D_PLATFORM_WINDOWS
#include "platform_win32.h"

#include "containers/cstring.h"
#include "core/engine.h"
#include "core/events/event_context.h"
#include "systems/events/event_system.h"
#include "systems/input/input_system.h"

namespace C3D
{
    Platform::Platform() : SystemWithConfig(nullptr, "PLATFORM") {}

    Platform::Platform(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager, "PLATFORM") {}

    bool Platform::Init(const PlatformSystemConfig& config)
    {
        m_logger.Info("Init() - Started.");

        m_handle.hInstance = GetModuleHandle(nullptr);

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &m_stdOutputConsoleScreenBufferInfo);
        GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &m_stdErrorConsoleScreenBufferInfo);

        if (config.makeWindow)
        {
            // Setup and register our window class
            HICON icon                = LoadIcon(m_handle.hInstance, IDI_APPLICATION);
            WNDCLASSA windowClass     = {};
            windowClass.style         = CS_DBLCLKS;  // Make sure to get double-clicks
            windowClass.lpfnWndProc   = StaticProcessMessage;
            windowClass.cbClsExtra    = 0;
            windowClass.cbWndExtra    = sizeof(this);
            windowClass.hInstance     = m_handle.hInstance;
            windowClass.hIcon         = icon;
            windowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);  // We provide NULL since we want to manage the cursor manually
            windowClass.hbrBackground = nullptr;                         // Transparent
            windowClass.lpszClassName = "C3D_ENGINE_WINDOW_CLASS";

            if (!RegisterClass(&windowClass))
            {
                m_logger.Error("Init() - Window registration failed.");
                return false;
            }

            // Create our window
            i32 windowX      = config.x;
            i32 windowY      = config.y;
            i32 windowWidth  = config.width;
            i32 windowHeight = config.height;

            u32 windowStyle   = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
            u32 windowExStyle = WS_EX_APPWINDOW;

            // Obtain the size of the border
            RECT borderRect = { 0, 0, 0, 0 };
            AdjustWindowRectEx(&borderRect, windowStyle, false, windowExStyle);

            // Adjust the x, y, width and height of the window to account for the border
            windowX += borderRect.left;
            windowY += borderRect.top;
            windowWidth += borderRect.right - borderRect.left;
            windowHeight += borderRect.bottom - borderRect.top;

            HWND handle = CreateWindowEx(windowExStyle, "C3D_ENGINE_WINDOW_CLASS", config.applicationName, windowStyle, windowX, windowY,
                                         windowWidth, windowHeight, nullptr, nullptr, m_handle.hInstance, nullptr);

            if (!handle)
            {
                m_logger.Error("Init() - Window registration failed.");
                return false;
            }

            m_handle.hwnd = handle;

            SetWindowLongPtr(m_handle.hwnd, 0, reinterpret_cast<LONG_PTR>(this));

            m_logger.Info("Init() - Window Creation successful.");

            // Actually show our window
            // TODO: Make configurable. This should be false when the window should not accept input
            constexpr bool shouldActivate = true;
            i32 showWindowCommandFlags    = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;

            ShowWindow(m_handle.hwnd, showWindowCommandFlags);

            m_logger.Info("Init() - ShowWindow successful.");
        }

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        m_clockFrequency = 1.0 / static_cast<f64>(frequency.QuadPart);

        LARGE_INTEGER startTime;
        QueryPerformanceCounter(&startTime);

        m_startTime = startTime.QuadPart;

        m_initialized = true;
        return true;
    }

    void Platform::Shutdown()
    {
        m_logger.Info("Shutdown() - Started.");

        // Unwatch all files that we are currently watching
        for (const auto& watch : m_fileWatches)
        {
            if (watch.id != INVALID_ID)
            {
                UnwatchFile(watch.id);
            }
        }
        m_fileWatches.Destroy();
    }

    bool Platform::PumpMessages()
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        return true;
    }

    CopyFileStatus Platform::CopyFile(const char* source, const char* dest, const bool overwriteIfExists = true)
    {
        if (const auto result = CopyFileA(source, dest, !overwriteIfExists); !result)
        {
            const auto error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
            {
                return CopyFileStatus::NotFound;
            }

            if (error == ERROR_SHARING_VIOLATION)
            {
                return CopyFileStatus::Locked;
            }

            return CopyFileStatus::Unknown;
        }

        return CopyFileStatus::Success;
    }

    FileWatchId Platform::WatchFile(const char* filePath)
    {
        if (!filePath)
        {
            m_logger.Error("WatchFile() - Failed due to filePath being invalid");
            return INVALID_ID;
        }

        WIN32_FIND_DATAA data = {};
        const auto fileHandle = FindFirstFileA(filePath, &data);
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            m_logger.Error("WatchFile() - Could not find file at: '{}'", filePath);
            return INVALID_ID;
        }

        if (!FindClose(fileHandle))
        {
            m_logger.Error("WatchFile() - Could not close file at: '{}'", filePath);
            return INVALID_ID;
        }

        for (u32 i = 0; i < m_fileWatches.Size(); i++)
        {
            auto& watch = m_fileWatches[i];

            if (watch.id == INVALID_ID)
            {
                // We have found an empty slot so we put the FileWatch in this slot and set the id to it's index
                watch.id            = i;
                watch.filePath      = filePath;
                watch.lastWriteTime = data.ftLastWriteTime;

                m_logger.Info("WatchFile() - Registered watch for: '{}'", filePath);
                return i;
            }
        }

        // We were unable to find an empty slot so let's add a new slot
        const auto nextIndex = static_cast<u32>(m_fileWatches.Size());

        const Win32FileWatch watch = { nextIndex, filePath, data.ftLastWriteTime };
        m_fileWatches.PushBack(watch);

        m_logger.Info("WatchFile() - Registered watch for: '{}'", filePath);
        return nextIndex;
    }

    bool Platform::UnwatchFile(const FileWatchId watchId)
    {
        if (watchId == INVALID_ID)
        {
            m_logger.Error("UnwatchFile() - Failed due to watchId being invalid");
            return false;
        }

        if (m_fileWatches.Empty())
        {
            m_logger.Error("UnwatchFile() - Failed since there are no files being watched currently");
            return false;
        }

        if (watchId >= m_fileWatches.Size())
        {
            m_logger.Error("UnwatchFile() - Failed since there is no watch for the provided id: {}", watchId);
            return false;
        }

        // Set the id to INVALID_ID to indicate that we no longer are interested in this watch
        // This makes the slot available to be filled by a different FileWatch in the future
        Win32FileWatch& watch = m_fileWatches[watchId];

        m_logger.Info("UnwatchFile() - Stopped watching: '{}'", watch.filePath);

        watch.id = INVALID_ID;
        watch.filePath.Clear();
        std::memset(&watch.lastWriteTime, 0, sizeof(FILETIME));

        return true;
    }

    void Platform::WatchFiles()
    {
        for (auto& watch : m_fileWatches)
        {
            if (watch.id != INVALID_ID)
            {
                WIN32_FIND_DATAA data;
                const HANDLE fileHandle = FindFirstFileA(watch.filePath.Data(), &data);
                if (fileHandle == INVALID_HANDLE_VALUE)
                {
                    // File was not found
                    EventContext context = {};
                    context.data.u32[0]  = watch.id;
                    // Fire a watched file removed event
                    Event.Fire(EventCodeWatchedFileRemoved, nullptr, context);
                    // Unwatch the file since it no longer exists
                    UnwatchFile(watch.id);
                    continue;
                }

                if (!FindClose(fileHandle)) continue;

                // Check the time the file was last edited to see if it has been changed
                if (CompareFileTime(&watch.lastWriteTime, &data.ftLastWriteTime) != 0)
                {
                    // File has been changed since last time
                    watch.lastWriteTime  = data.ftLastWriteTime;
                    EventContext context = {};
                    context.data.u32[0]  = watch.id;
                    // Fire the watched file changed event
                    Event.Fire(EventCodeWatchedFileChanged, nullptr, context);
                }
            }
        }
    }

    f64 Platform::GetAbsoluteTime() const
    {
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);
        return static_cast<f64>(nowTime.QuadPart) * m_clockFrequency;
    }

    void Platform::SleepMs(const u64 ms) { Sleep(static_cast<u32>(ms)); }

    i32 Platform::GetProcessorCount()
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return static_cast<i32>(sysInfo.dwNumberOfProcessors);
    }

    u64 Platform::GetThreadId() { return GetCurrentThreadId(); }

    bool Platform::LoadDynamicLibrary(const char* name, void** libraryData, u64& size)
    {
        if (!name)
        {
            printf("[PLATFORM] - LoadDynamicLibrary() Failed - Please provide a valid name");
            return false;
        }

        CString<128> path(name);
        path += ".dll";

        HMODULE library = LoadLibraryA(path.Data());
        if (!library)
        {
            const auto errorMsg = GetLastErrorMsg();
            printf("[PLATFORM] - LoadDynamicLibrary() Failed - %s", errorMsg.data());
            return false;
        }

        *libraryData = library;
        size         = sizeof(HMODULE);
        return true;
    }

    bool Platform::UnloadDynamicLibrary(void* libraryData)
    {
        const auto library = static_cast<HMODULE>(libraryData);
        if (!library)
        {
            printf("[PLATFORM] - UnloadDynamicLibrary() Failed - Please provide a valid library");
            return false;
        }

        if (FreeLibrary(library) == 0)
        {
            const auto errorMsg = GetLastErrorMsg();
            printf("[PLATFORM] - UnloadDynamicLibrary() Failed - %s", errorMsg.data());
            return false;
        }

        return true;
    }

    LRESULT CALLBACK Platform::StaticProcessMessage(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
    {
        auto platform = reinterpret_cast<Platform*>(GetWindowLongPtr(hwnd, 0));
        if (platform)
        {
            return platform->ProcessMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK Platform::ProcessMessage(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
            case WM_ERASEBKGND:
                // Notify the OS that erasing of the background will be handled by the application to prevent flicker
                return true;
            case WM_CLOSE:
            {
                // Fire an event for the application to quit
                EventContext data = {};
                Event.Fire(EventCodeApplicationQuit, this, data);
                return 0;
            }
            case WM_DESTROY:
            {
                // Make sure we return 0 on a quit (where nothing went wrong)
                PostQuitMessage(0);
                return 0;
            }
            case WM_SIZE:
            {
                // Window resize, let's get the updated size
                RECT r;
                GetClientRect(hwnd, &r);
                u32 width  = r.right - r.left;
                u32 height = r.bottom - r.top;

                // Fire a resize event
                EventContext context = {};
                context.data.u16[0]  = static_cast<u16>(width);
                context.data.u16[1]  = static_cast<u16>(height);
                Event.Fire(EventCodeResized, this, context);
                break;
            }
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                Keys key = static_cast<Keys>(wParam);

                // Check for extended scan code
                bool isExtended = (HIWORD(lParam) & KF_EXTENDED) == KF_EXTENDED;

                if (wParam == VK_MENU)
                {
                    key = isExtended ? KeyRAlt : KeyLAlt;
                }
                else if (wParam == VK_SHIFT)
                {
                    // KF_EXTENDED is not set for the shift keys so we need to check separately
                    u32 leftShift = MapVirtualKey(VK_LSHIFT, MAPVK_VK_TO_VSC);
                    u32 scanCode  = ((lParam & (0xFF << 16)) >> 16);
                    key           = (scanCode == leftShift) ? KeyLShift : KeyRShift;
                }
                else if (wParam == VK_CONTROL)
                {
                    key = isExtended ? KeyRControl : KeyLControl;
                }

                const auto down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                Input.ProcessKey(key, down ? InputState::Down : InputState::Up);
                // Return 0 to prevent default behaviour for keys
                return 0;
            }
            case WM_MOUSEMOVE:
            {
                i32 xPos = GET_X_LPARAM(lParam);
                i32 yPos = GET_Y_LPARAM(lParam);
                Input.ProcessMouseMove(xPos, yPos);
                break;
            }
            case WM_MOUSEWHEEL:
            {
                i32 delta = GET_WHEEL_DELTA_WPARAM(wParam);
                if (delta != 0)
                {
                    // Convert into OS-independent (-1 or 1)
                    delta = (delta < 0) ? -1 : 1;
                    Input.ProcessMouseWheel(delta);
                }
                break;
            }
            case WM_LBUTTONDOWN:
                Input.ProcessButton(ButtonLeft, InputState::Down);
                break;
            case WM_MBUTTONDOWN:
                Input.ProcessButton(ButtonMiddle, InputState::Down);
                break;
            case WM_RBUTTONDOWN:
                Input.ProcessButton(ButtonRight, InputState::Down);
                break;
            case WM_LBUTTONUP:
                Input.ProcessButton(ButtonLeft, InputState::Up);
                break;
            case WM_MBUTTONUP:
                Input.ProcessButton(ButtonMiddle, InputState::Up);
                break;
            case WM_RBUTTONUP:
                Input.ProcessButton(ButtonRight, InputState::Up);
                break;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    std::string Platform::GetLastErrorMsg()
    {
        const auto errorCode = GetLastError();
        if (errorCode == 0)
        {
            // No Error has been reported
            return "NONE";
        }

        // Pointer to buffer holding our message
        LPSTR messageBuffer = nullptr;

        // Let the win32 API allocate and populate our message buffer
        const auto size =
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

        // Copy the message into a String
        std::string msg(messageBuffer, size);

        // Free our allocated buffer so we don't leak
        LocalFree(messageBuffer);

        return msg;
    }
}  // namespace C3D
#endif