////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#ifdef _WIN32_WINDOWS
    #undef _WIN32_WINDOWS
#endif
#ifdef _WIN32_WINNT
    #undef _WIN32_WINNT
#endif
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT   0x0501
#include <SFML/Window/Win32/WindowImplWin32.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <GL/gl.h>
#include <SFML/Window/glext/wglext.h>
#include <SFML/Window/glext/glext.h>
#include <SFML/System/Err.hpp>
#include <vector>

// MinGW lacks the definition of some Win32 constants
#ifndef XBUTTON1
    #define XBUTTON1 0x0001
#endif
#ifndef XBUTTON2
    #define XBUTTON2 0x0002
#endif
#ifndef MAPVK_VK_TO_VSC
    #define MAPVK_VK_TO_VSC (0)
#endif


namespace
{
    unsigned int               windowCount      = 0;
    const char*                classNameA       = "SFML_Window";
    const wchar_t*             classNameW       = L"SFML_Window";
    sf::priv::WindowImplWin32* fullscreenWindow = NULL;
}

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
WindowImplWin32::WindowImplWin32(WindowHandle handle) :
m_handle          (handle),
m_callback        (0),
m_icon            (NULL),
m_lastSize        (0, 0),
m_resizing        (false)
{
    if (m_handle)
    {
        // We change the event procedure of the control (it is important to save the old one)
        SetWindowLongPtr(m_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_callback = SetWindowLongPtr(m_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowImplWin32::globalOnEvent));
    }
}


////////////////////////////////////////////////////////////
WindowImplWin32::WindowImplWin32(VideoMode mode, const std::string& title, Uint32 style) :
m_handle          (NULL),
m_callback        (0),
m_icon            (NULL),
m_lastSize        (mode.width, mode.height),
m_resizing        (false)
{
    // Register the window class at first call
    if (windowCount == 0)
        registerWindowClass();

    // Compute position and size
    HDC screenDC = GetDC(NULL);
    int left   = (GetDeviceCaps(screenDC, HORZRES) - mode.width)  / 2;
    int top    = (GetDeviceCaps(screenDC, VERTRES) - mode.height) / 2;
    int width  = mode.width;
    int height = mode.height;
    ReleaseDC(NULL, screenDC);

    // Choose the window style according to the Style parameter
    DWORD win32Style = WS_VISIBLE;
    if (style == Style::None)
    {
        win32Style |= WS_POPUP;
    }
    else
    {
        if (style & Style::Titlebar) win32Style |= WS_CAPTION | WS_MINIMIZEBOX;
        if (style & Style::Resize)   win32Style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        if (style & Style::Close)    win32Style |= WS_SYSMENU;
    }

    // In windowed mode, adjust width and height so that window will have the requested client area
    bool fullscreen = (style & Style::Fullscreen) != 0;
    if (!fullscreen)
    {
        RECT rectangle = {0, 0, width, height};
        AdjustWindowRect(&rectangle, win32Style, false);
        width  = rectangle.right - rectangle.left;
        height = rectangle.bottom - rectangle.top;
    }

    // Create the window
    if (hasUnicodeSupport())
    {
        wchar_t wTitle[256];
        int count = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, title.c_str(), static_cast<int>(title.size()), wTitle, sizeof(wTitle) / sizeof(*wTitle));
        wTitle[count] = L'\0';
        m_handle = CreateWindowW(classNameW, wTitle, win32Style, left, top, width, height, NULL, NULL, GetModuleHandle(NULL), this);
    }
    else
    {
        m_handle = CreateWindowA(classNameA, title.c_str(), win32Style, left, top, width, height, NULL, NULL, GetModuleHandle(NULL), this);
    }

    // Switch to fullscreen if requested
    if (fullscreen)
        switchToFullscreen(mode);

    // Increment window count
    windowCount++;
}


////////////////////////////////////////////////////////////
WindowImplWin32::~WindowImplWin32()
{
    // Destroy the custom icon, if any
    if (m_icon)
        DestroyIcon(m_icon);

    if (!m_callback)
    {
        // Destroy the window
        if (m_handle)
            DestroyWindow(m_handle);

        // Decrement the window count
        windowCount--;

        // Unregister window class if we were the last window
        if (windowCount == 0)
        {
            if (hasUnicodeSupport())
            {
                UnregisterClassW(classNameW, GetModuleHandle(NULL));
            }
            else
            {
                UnregisterClassA(classNameA, GetModuleHandle(NULL));
            }
        }
    }
    else
    {
        // The window is external : remove the hook on its message callback
        SetWindowLongPtr(m_handle, GWLP_WNDPROC, m_callback);
    }
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplWin32::getSystemHandle() const
{
    return m_handle;
}


////////////////////////////////////////////////////////////
void WindowImplWin32::processEvents()
{
    // We process the window events only if we own it
    if (!m_callback)
    {
        MSG message;
        while (PeekMessage(&message, m_handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
}


////////////////////////////////////////////////////////////
Vector2i WindowImplWin32::getPosition() const
{
    RECT rect;
    GetWindowRect(m_handle, &rect);

    return Vector2i(rect.left, rect.top);
}


////////////////////////////////////////////////////////////
void WindowImplWin32::setPosition(const Vector2i& position)
{
    SetWindowPos(m_handle, NULL, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


////////////////////////////////////////////////////////////
Vector2u WindowImplWin32::getSize() const
{
    RECT rect;
    GetClientRect(m_handle, &rect);

    return Vector2u(rect.right - rect.left, rect.bottom - rect.top);
}


////////////////////////////////////////////////////////////
void WindowImplWin32::setSize(const Vector2u& size)
{
    // SetWindowPos wants the total size of the window (including title bar and borders),
    // so we have to compute it
    RECT rectangle = {0, 0, size.x, size.y};
    AdjustWindowRect(&rectangle, GetWindowLong(m_handle, GWL_STYLE), false);
    int width  = rectangle.right - rectangle.left;
    int height = rectangle.bottom - rectangle.top;

    SetWindowPos(m_handle, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}


////////////////////////////////////////////////////////////
void WindowImplWin32::setTitle(const std::string& title)
{
    SetWindowTextA(m_handle, title.c_str());
}


////////////////////////////////////////////////////////////
void WindowImplWin32::setIcon(unsigned int width, unsigned int height, const Uint8* pixels)
{
    // First destroy the previous one
    if (m_icon)
        DestroyIcon(m_icon);

    // Windows wants BGRA pixels: swap red and blue channels
    std::vector<Uint8> iconPixels(width * height * 4);
    for (std::size_t i = 0; i < iconPixels.size() / 4; ++i)
    {
        iconPixels[i * 4 + 0] = pixels[i * 4 + 2];
        iconPixels[i * 4 + 1] = pixels[i * 4 + 1];
        iconPixels[i * 4 + 2] = pixels[i * 4 + 0];
        iconPixels[i * 4 + 3] = pixels[i * 4 + 3];
    }

    // Create the icon from the pixel array
    m_icon = CreateIcon(GetModuleHandle(NULL), width, height, 1, 32, NULL, &iconPixels[0]);

    // Set it as both big and small icon of the window
    if (m_icon)
    {
        SendMessage(m_handle, WM_SETICON, ICON_BIG,   (LPARAM)m_icon);
        SendMessage(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)m_icon);
    }
    else
    {
        err() << "Failed to set the window's icon" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void WindowImplWin32::setVisible(bool visible)
{
    ShowWindow(m_handle, visible ? SW_SHOW : SW_HIDE);
}


////////////////////////////////////////////////////////////
void WindowImplWin32::registerWindowClass()
{
    if (hasUnicodeSupport())
    {
        WNDCLASSW windowClass;
        windowClass.style         = 0;
        windowClass.lpfnWndProc   = &WindowImplWin32::globalOnEvent;
        windowClass.cbClsExtra    = 0;
        windowClass.cbWndExtra    = 0;
        windowClass.hInstance     = GetModuleHandle(NULL);
        windowClass.hIcon         = NULL;
        windowClass.hbrBackground = 0;
        windowClass.lpszMenuName  = NULL;
        windowClass.lpszClassName = classNameW;
        RegisterClassW(&windowClass);
    }
    else
    {
        WNDCLASSA windowClass;
        windowClass.style         = 0;
        windowClass.lpfnWndProc   = &WindowImplWin32::globalOnEvent;
        windowClass.cbClsExtra    = 0;
        windowClass.cbWndExtra    = 0;
        windowClass.hInstance     = GetModuleHandle(NULL);
        windowClass.hIcon         = NULL;
        windowClass.hbrBackground = 0;
        windowClass.lpszMenuName  = NULL;
        windowClass.lpszClassName = classNameA;
        RegisterClassA(&windowClass);
    }
}


////////////////////////////////////////////////////////////
void WindowImplWin32::switchToFullscreen(const VideoMode& mode)
{
    DEVMODE devMode;
    devMode.dmSize       = sizeof(devMode);
    devMode.dmPelsWidth  = mode.width;
    devMode.dmPelsHeight = mode.height;
    devMode.dmBitsPerPel = mode.bitsPerPixel;
    devMode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

    // Apply fullscreen mode
    if (ChangeDisplaySettings(&devMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
    {
        err() << "Failed to change display mode for fullscreen" << std::endl;
        return;
    }

    // Make the window flags compatible with fullscreen mode
    SetWindowLong(m_handle, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    SetWindowLong(m_handle, GWL_EXSTYLE, WS_EX_APPWINDOW);

    // Resize the window so that it fits the entire screen
    SetWindowPos(m_handle, HWND_TOP, 0, 0, mode.width, mode.height, SWP_FRAMECHANGED);
    ShowWindow(m_handle, SW_SHOW);

    // Set "this" as the current fullscreen window
    fullscreenWindow = this;
}


////////////////////////////////////////////////////////////
void WindowImplWin32::cleanup()
{
    // Restore the previous video mode (in case we were running in fullscreen)
    if (fullscreenWindow == this)
    {
        ChangeDisplaySettings(NULL, 0);
        fullscreenWindow = NULL;
    }
}


////////////////////////////////////////////////////////////
void WindowImplWin32::processEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Don't process any message until window is created
    if (m_handle == NULL)
        return;

    switch (message)
    {
        // Destroy event
        case WM_DESTROY :
        {
            // Here we must cleanup resources !
            cleanup();
            break;
        }

        // Close event
        case WM_CLOSE :
        {
            Event event;
            event.type = Event::Closed;
            pushEvent(event);
            break;
        }

        // Resize event
        case WM_SIZE :
        {
            // Consider only events triggered by a maximize or a un-maximize
            if (wParam != SIZE_MINIMIZED && !m_resizing && m_lastSize != getSize())
            {
                // Update the last handled size
                m_lastSize = getSize();

                // Push a resize event
                Event event;
                event.type        = Event::Resized;
                event.size.width  = m_lastSize.x;
                event.size.height = m_lastSize.y;
                pushEvent(event);
            }
            break;
        }

        // Start resizing
        case WM_ENTERSIZEMOVE:
        {
            m_resizing = true;
            break;
        }

        // Stop resizing
        case WM_EXITSIZEMOVE:
        {
            m_resizing = false;

            // Ignore cases where the window has only been moved
            if(m_lastSize != getSize())
            {
                // Update the last handled size
                m_lastSize = getSize();

                // Push a resize event
                Event event;
                event.type        = Event::Resized;
                event.size.width  = m_lastSize.x;
                event.size.height = m_lastSize.y;
                pushEvent(event);
            }
            break;
        }

        // Gain focus event
        case WM_SETFOCUS :
        {
            Event event;
            event.type = Event::GainedFocus;
            pushEvent(event);
            break;
        }

        // Lost focus event
        case WM_KILLFOCUS :
        {
            Event event;
            event.type = Event::LostFocus;
            pushEvent(event);
            break;
        }
    }
}


////////////////////////////////////////////////////////////
bool WindowImplWin32::hasUnicodeSupport()
{
    OSVERSIONINFO version;
    ZeroMemory(&version, sizeof(version));
    version.dwOSVersionInfoSize = sizeof(version);

    if (GetVersionEx(&version))
    {
        return version.dwPlatformId == VER_PLATFORM_WIN32_NT;
    }
    else
    {
        return false;
    }
}


////////////////////////////////////////////////////////////
LRESULT CALLBACK WindowImplWin32::globalOnEvent(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Associate handle and Window instance when the creation message is received
    if (message == WM_CREATE)
    {
        // Get WindowImplWin32 instance (it was passed as the last argument of CreateWindow)
        LONG_PTR window = (LONG_PTR)reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams;

        // Set as the "user data" parameter of the window
        SetWindowLongPtr(handle, GWLP_USERDATA, window);
    }

    // Get the WindowImpl instance corresponding to the window handle
    WindowImplWin32* window = reinterpret_cast<WindowImplWin32*>(GetWindowLongPtr(handle, GWLP_USERDATA));

    // Forward the event to the appropriate function
    if (window)
    {
        window->processEvent(message, wParam, lParam);

        if (window->m_callback)
            return CallWindowProc(reinterpret_cast<WNDPROC>(window->m_callback), handle, message, wParam, lParam);
    }

    // We don't forward the WM_CLOSE message to prevent the OS from automatically destroying the window
    if (message == WM_CLOSE)
        return 0;

    static const bool hasUnicode = hasUnicodeSupport();
    return hasUnicode ? DefWindowProcW(handle, message, wParam, lParam) :
                        DefWindowProcA(handle, message, wParam, lParam);
}

} // namespace priv

} // namespace sf
