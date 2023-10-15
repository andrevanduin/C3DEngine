
#include "platform/platform.h"

#ifdef C3D_PLATFORM_LINUX

#include <X11/XKBlib.h>    // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h>  // sudo apt-get install libxkbcommon-x11-dev libx11-xcb-dev
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <xcb/xcb.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#endif

#include <errno.h>  // For error reporting
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>  // Processor info
#include <unistd.h>

namespace C3D
{
    bool Platform::Init(const PlatformSystemConfig& config)
    {
        // Connect to X
        m_display = XOpenDisplay(nullptr);

        // Turn off key repeats
        XAutoRepeatOff(m_display);

        // Retrieve the connection from the display
        m_handle.connection = XGetXCBConnection(m_display);

        if (xcb_connection_has_error(m_handle.connection))
        {
            m_logger.Error("Init() - Failed to connect to X server via XCB.");
            return false;
        }

        // Get data from the X server
        const xcb_setup_t* setup = xcb_get_setup(m_handle.connection);

        // Loop through screens
        xcb_screen_iterator_t it = xcb_setup_roots_iterator(m_handle.connection);
        for (i32 s = 0; s > 0; s--)
        {
            xcb_screen_next(&it);
        }

        // After screens have been looped through, we assign it
        m_screen = it.data;

        // Allocate a XID for the window to be created
        m_handle.window = xcb_generate_id(m_handle.connection);

        // Register event types
        // XCB_CW_BACK_PIXEL = filling then window bg with a single colour
        // XCB_CW_EVENT_MASK is required.
        u32 eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        // Listen for keyboard and mouse buttons
        u32 eventValues = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
                          XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
                          XCB_EVENT_MASK_STRUCTURE_NOTIFY;

        // Values to be sent over XCB (background color and events)
        u32 valueList[2] = { m_screen->black_pixel, eventValues };

        // Create the window
        xcb_create_window(m_handle.connection, XCB_COPY_FROM_PARENT, m_handle.window, m_screen->root, config.x, config.y, config.width,
                          config.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, m_screen->root_visual, eventMask, valueList);

        // Change the title
        xcb_change_property(m_handle.connection, XCB_PROP_MODE_REPLACE, m_handle.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                            std::strlen(config.applicationName), config.applicationName);

        // Tell the server to notify when the window manager attempts to destroy the window
        xcb_intern_atom_cookie_t wmDeleteCookie =
            xcb_intern_atom(m_handle.connection, 0, std::strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");

        xcb_intern_atom_cookie_t wmProtocolsCookie = xcb_intern_atom(m_handle.connection, 0, std::strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
        xcb_intern_atom_reply_t* wmDeleteReply     = xcb_intern_atom_reply(m_handle.connection, wmDeleteCookie, nullptr);
        xcb_intern_atom_reply_t* wmProtocolsReply  = xcb_intern_atom_reply(m_handle.connection, wmProtocolsCookie, nullptr);

        xcb_atom_t m_wmProtocols = wmDeleteReply->atom;
        xcb_atom_t m_wmDeleteWin = wmProtocolsReply->atom;

        xcb_change_property(m_handle.connection, XCB_PROP_MODE_REPLACE, m_handle.window, wmProtocolsReply->atom, 4, 32, 1,
                            &wmDeleteReply->atom);

        // Map the window to the screen
        xcb_map_window(m_handle.connection, m_handle.window);

        // Flush the stream
        i32 streamResult = xcb_flush(m_handle.connection);
        if (streamResult <= 0)
        {
            m_logger.Error("Init() - Flushing the stream failed: '{}'.", streamResult);
            return false;
        }

        return true;
    }

    void Platform::Shutdown()
    {
        if (m_display)
        {
            // Turn key repeats back on since it's global for the entire OS
            XAutoRepeatOn(m_display);
        }
        if (m_handle.connection && m_handle.window)
        {
            // Destroy the window
            xcb_destroy_window(m_handle.connection, m_handle.window);
        }
    }

    bool Platform::PumpMessages() {}

    f64 Platform::GetAbsoluteTime() const
    {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        return now.tv_sec + now.tv_nsec * 0.000000001;
    }

    void Platform::SleepMs(const u64 ms)
    {
#if _POSIX_C_SOURCE >= 199309L
        timespec ts;
        ts.tv_sec  = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000 * 1000;
        nanosleep(&ts, 0);
#else
        if (ms >= 1000)
        {
            sleep(ms / 1000);
        }
        usleep((ms % 1000) * 1000);
#endif
    }

    i32 Platform::GetProcessorCount()
    {
        auto processorCount      = get_nprocs_conf();
        auto processorsAvailable = get_nprocs();
        return processorsAvailable;
    }

    u64 Platform::GetThreadId() { return static_cast<u64>(pthread_self()); }

    static bool LoadDynamicLibrary(const char* name, void** libraryData, u64& size)
    {
        if (!name)
        {
            printf("[PLATFORM] - LoadDynamicLibrary() Failed - Please provide a valid name.");
            return false;
        }

        CString<256> path;
        path.FromFormat("{}{}{}", GetDynamicLibraryPrefix(), name, GetDynamicLibraryExtension());

        void* library = dlopen(path.Data(), RTLD_NOW);
        if (!library)
        {
            printf("[PLATFORM] - LoadDynamicLibrary() - Failed.");
            return false;
        }

        *libraryData = library;
        size         = 8;
        return true;
    }
    
    static bool UnloadDynamicLibrary(void* libraryData)
    {

    }
}  // namespace C3D
#endif