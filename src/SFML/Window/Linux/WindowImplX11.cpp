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
#include <SFML/Window/WindowStyle.hpp> // important to be included first (conflict with None)
#include <SFML/Window/Linux/WindowImplX11.hpp>
#include <SFML/Window/Linux/Display.hpp>
#include <SFML/System/Utf.hpp>
#include <SFML/System/Err.hpp>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <sstream>
#include <vector>


////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////
namespace
{
    sf::priv::WindowImplX11* fullscreenWindow = NULL;
    unsigned long            eventMask        = FocusChangeMask | StructureNotifyMask;

    /// Filter the events received by windows
    /// (only allow those matching a specific window)
    Bool checkEvent(::Display*, XEvent* event, XPointer userData)
    {
        // Just check if the event matches the window
        return event->xany.window == reinterpret_cast< ::Window >(userData);
    }
}


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
WindowImplX11::WindowImplX11(WindowHandle handle) :
m_window      (0),
m_isExternal  (true),
m_atomClose   (0),
m_oldVideoMode(-1)
{
    // Open a connection with the X server
    m_display = OpenDisplay();
    m_screen  = DefaultScreen(m_display);

    // Save the window handle
    m_window = handle;

    if (m_window)
    {
        // Make sure the window is listening to all the requiered events
        XSelectInput(m_display, m_window, eventMask);

        // Do some common initializations
        initialize();
    }
}


////////////////////////////////////////////////////////////
WindowImplX11::WindowImplX11(VideoMode mode, const std::string& title, unsigned long style) :
m_window      (0),
m_isExternal  (false),
m_atomClose   (0),
m_oldVideoMode(-1)
{
    // Open a connection with the X server
    m_display = OpenDisplay();
    m_screen  = DefaultScreen(m_display);

    // Compute position and size
    int left, top;
    bool fullscreen = (style & Style::Fullscreen) != 0;
    if (!fullscreen)
    {
        left = (DisplayWidth(m_display, m_screen)  - mode.width)  / 2;
        top  = (DisplayHeight(m_display, m_screen) - mode.height) / 2;
    }
    else
    {
        left = 0;
        top  = 0;
    }
    int width  = mode.width;
    int height = mode.height;

    // Switch to fullscreen if necessary
    if (fullscreen)
        switchToFullscreen(mode);

    // Define the window attributes
    XSetWindowAttributes attributes;
    attributes.event_mask        = eventMask;
    attributes.override_redirect = fullscreen;

    // Create the window
    m_window = XCreateWindow(m_display,
                             RootWindow(m_display, m_screen),
                             left, top,
                             width, height,
                             0,
                             DefaultDepth(m_display, m_screen),
                             InputOutput,
                             DefaultVisual(m_display, m_screen),
                             CWEventMask | CWOverrideRedirect, &attributes);
    if (!m_window)
    {
        err() << "Failed to create window" << std::endl;
        return;
    }

    // Set the window's name
    setTitle(title);

    // Set the window's style (tell the windows manager to change our window's decorations and functions according to the requested style)
    if (!fullscreen)
    {
        Atom WMHintsAtom = XInternAtom(m_display, "_MOTIF_WM_HINTS", false);
        if (WMHintsAtom)
        {
            static const unsigned long MWM_HINTS_FUNCTIONS   = 1 << 0;
            static const unsigned long MWM_HINTS_DECORATIONS = 1 << 1;
    
            //static const unsigned long MWM_DECOR_ALL         = 1 << 0;
            static const unsigned long MWM_DECOR_BORDER      = 1 << 1;
            static const unsigned long MWM_DECOR_RESIZEH     = 1 << 2;
            static const unsigned long MWM_DECOR_TITLE       = 1 << 3;
            static const unsigned long MWM_DECOR_MENU        = 1 << 4;
            static const unsigned long MWM_DECOR_MINIMIZE    = 1 << 5;
            static const unsigned long MWM_DECOR_MAXIMIZE    = 1 << 6;

            //static const unsigned long MWM_FUNC_ALL          = 1 << 0;
            static const unsigned long MWM_FUNC_RESIZE       = 1 << 1;
            static const unsigned long MWM_FUNC_MOVE         = 1 << 2;
            static const unsigned long MWM_FUNC_MINIMIZE     = 1 << 3;
            static const unsigned long MWM_FUNC_MAXIMIZE     = 1 << 4;
            static const unsigned long MWM_FUNC_CLOSE        = 1 << 5;
    
            struct WMHints
            {
                unsigned long Flags;
                unsigned long Functions;
                unsigned long Decorations;
                long          InputMode;
                unsigned long State;
            };
    
            WMHints hints;
            hints.Flags       = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
            hints.Decorations = 0;
            hints.Functions   = 0;

            if (style & Style::Titlebar)
            {
                hints.Decorations |= MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MINIMIZE | MWM_DECOR_MENU;
                hints.Functions   |= MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE;
            }
            if (style & Style::Resize)
            {
                hints.Decorations |= MWM_DECOR_MAXIMIZE | MWM_DECOR_RESIZEH;
                hints.Functions   |= MWM_FUNC_MAXIMIZE | MWM_FUNC_RESIZE;
            }
            if (style & Style::Close)
            {
                hints.Decorations |= 0;
                hints.Functions   |= MWM_FUNC_CLOSE;
            }

            const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&hints);
            XChangeProperty(m_display, m_window, WMHintsAtom, WMHintsAtom, 32, PropModeReplace, ptr, 5);
        }

        // This is a hack to force some windows managers to disable resizing
        if (!(style & Style::Resize))
        {
            XSizeHints sizeHints;
            sizeHints.flags      = PMinSize | PMaxSize;
            sizeHints.min_width  = sizeHints.max_width  = width;
            sizeHints.min_height = sizeHints.max_height = height;
            XSetWMNormalHints(m_display, m_window, &sizeHints); 
        }
    }

    // Do some common initializations
    initialize();
}


////////////////////////////////////////////////////////////
WindowImplX11::~WindowImplX11()
{
    // Cleanup graphical resources
    cleanup();

    // Destroy the window
    if (m_window && !m_isExternal)
    {
        XDestroyWindow(m_display, m_window);
        XFlush(m_display);
    }

    // Close the connection with the X server
    CloseDisplay(m_display);
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplX11::getSystemHandle() const
{
    return m_window;
}


////////////////////////////////////////////////////////////
void WindowImplX11::processEvents()
{
    XEvent event;
    while (XCheckIfEvent(m_display, &event, &checkEvent, reinterpret_cast<XPointer>(m_window)))
    {
        processEvent(event);
    }
}


////////////////////////////////////////////////////////////
Vector2i WindowImplX11::getPosition() const
{
    XWindowAttributes attributes;
    XGetWindowAttributes(m_display, m_window, &attributes);
    return Vector2i(attributes.x, attributes.y);
}


////////////////////////////////////////////////////////////
void WindowImplX11::setPosition(const Vector2i& position)
{
    XMoveWindow(m_display, m_window, position.x, position.y);
    XFlush(m_display);
}


////////////////////////////////////////////////////////////
Vector2u WindowImplX11::getSize() const
{
    XWindowAttributes attributes;
    XGetWindowAttributes(m_display, m_window, &attributes);
    return Vector2u(attributes.width, attributes.height);
}


////////////////////////////////////////////////////////////
void WindowImplX11::setSize(const Vector2u& size)
{
    XResizeWindow(m_display, m_window, size.x, size.y);
    XFlush(m_display);
}


////////////////////////////////////////////////////////////
void WindowImplX11::setTitle(const std::string& title)
{
    XStoreName(m_display, m_window, title.c_str());
}


////////////////////////////////////////////////////////////
void WindowImplX11::setIcon(unsigned int width, unsigned int height, const Uint8* pixels)
{
    // X11 wants BGRA pixels : swap red and blue channels
    // Note: this memory will be freed by XDestroyImage
    Uint8* iconPixels = static_cast<Uint8*>(std::malloc(width * height * 4));
    for (std::size_t i = 0; i < width * height; ++i)
    {
        iconPixels[i * 4 + 0] = pixels[i * 4 + 2];
        iconPixels[i * 4 + 1] = pixels[i * 4 + 1];
        iconPixels[i * 4 + 2] = pixels[i * 4 + 0];
        iconPixels[i * 4 + 3] = pixels[i * 4 + 3];
    }

    // Create the icon pixmap
    Visual*      defVisual = DefaultVisual(m_display, m_screen);
    unsigned int defDepth  = DefaultDepth(m_display, m_screen);
    XImage* iconImage = XCreateImage(m_display, defVisual, defDepth, ZPixmap, 0, (char*)iconPixels, width, height, 32, 0);
    if (!iconImage)
    {
        err() << "Failed to set the window's icon" << std::endl;
        return;
    }
    Pixmap iconPixmap = XCreatePixmap(m_display, RootWindow(m_display, m_screen), width, height, defDepth);
    XGCValues values;
    GC iconGC = XCreateGC(m_display, iconPixmap, 0, &values);
    XPutImage(m_display, iconPixmap, iconGC, iconImage, 0, 0, 0, 0, width, height);
    XFreeGC(m_display, iconGC);
    XDestroyImage(iconImage);

    // Create the mask pixmap (must have 1 bit depth)
    std::size_t pitch = (width + 7) / 8;
    static std::vector<Uint8> maskPixels(pitch * height, 0);
    for (std::size_t j = 0; j < height; ++j)
    {
        for (std::size_t i = 0; i < pitch; ++i)
        {
            for (std::size_t k = 0; k < 8; ++k)
            {
                if (i * 8 + k < width)
                {
                    Uint8 opacity = (pixels[(i * 8 + k + j * width) * 4 + 3] > 0) ? 1 : 0;
                    maskPixels[i + j * pitch] |= (opacity << k);                    
                }
            }
        }
    }
    Pixmap maskPixmap = XCreatePixmapFromBitmapData(m_display, m_window, (char*)&maskPixels[0], width, height, 1, 0, 1);

    // Send our new icon to the window through the WMHints
    XWMHints* hints = XAllocWMHints();
    hints->flags       = IconPixmapHint | IconMaskHint;
    hints->icon_pixmap = iconPixmap;
    hints->icon_mask   = maskPixmap;
    XSetWMHints(m_display, m_window, hints);
    XFree(hints);

    XFlush(m_display);
}


////////////////////////////////////////////////////////////
void WindowImplX11::setVisible(bool visible)
{
    if (visible)
        XMapWindow(m_display, m_window);
    else
        XUnmapWindow(m_display, m_window);

    XFlush(m_display);
}


////////////////////////////////////////////////////////////
void WindowImplX11::switchToFullscreen(const VideoMode& mode)
{
    // Check if the XRandR extension is present
    int version;
    if (XQueryExtension(m_display, "RANDR", &version, &version, &version))
    {
        // Get the current configuration
        XRRScreenConfiguration* config = XRRGetScreenInfo(m_display, RootWindow(m_display, m_screen));
        if (config)
        {
            // Get the current rotation
            Rotation currentRotation;
            m_oldVideoMode = XRRConfigCurrentConfiguration(config, &currentRotation);

            // Get the available screen sizes
            int nbSizes;
            XRRScreenSize* sizes = XRRConfigSizes(config, &nbSizes);
            if (sizes && (nbSizes > 0))
            {
                // Search a matching size
                for (int i = 0; i < nbSizes; ++i)
                {
                    if ((sizes[i].width == static_cast<int>(mode.width)) && (sizes[i].height == static_cast<int>(mode.height)))
                    {
                        // Switch to fullscreen mode
                        XRRSetScreenConfig(m_display, config, RootWindow(m_display, m_screen), i, currentRotation, CurrentTime);

                        // Set "this" as the current fullscreen window
                        fullscreenWindow = this;
                        break;
                    }
                }
            }

            // Free the configuration instance
            XRRFreeScreenConfigInfo(config);
        }
        else
        {
            // Failed to get the screen configuration
            err() << "Failed to get the current screen configuration for fullscreen mode, switching to window mode" << std::endl;
        }
    }
    else
    {
        // XRandr extension is not supported : we cannot use fullscreen mode
        err() << "Fullscreen is not supported, switching to window mode" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::initialize()
{
    // Get the atom defining the close event
    m_atomClose = XInternAtom(m_display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(m_display, m_window, &m_atomClose, 1);

    // Show the window
    XMapWindow(m_display, m_window);
    XFlush(m_display);

    // Flush the commands queue
    XFlush(m_display);
}


////////////////////////////////////////////////////////////
void WindowImplX11::cleanup()
{
    // Restore the previous video mode (in case we were running in fullscreen)
    if (fullscreenWindow == this)
    {
        // Get current screen info
        XRRScreenConfiguration* config = XRRGetScreenInfo(m_display, RootWindow(m_display, m_screen));
        if (config) 
        {
            // Get the current rotation
            Rotation currentRotation;
            XRRConfigCurrentConfiguration(config, &currentRotation);

            // Reset the video mode
            XRRSetScreenConfig(m_display, config, RootWindow(m_display, m_screen), m_oldVideoMode, currentRotation, CurrentTime);

            // Free the configuration instance
            XRRFreeScreenConfigInfo(config);
        } 

        // Reset the fullscreen window
        fullscreenWindow = NULL;
    }
}


////////////////////////////////////////////////////////////
bool WindowImplX11::processEvent(XEvent windowEvent)
{
    // Convert the X11 event to a sf::Event
    switch (windowEvent.type)
    {
        // Destroy event
        case DestroyNotify :
        {
            // The window is about to be destroyed : we must cleanup resources
            cleanup();
            break;
        }

        // Gain focus event
        case FocusIn :
        {
            Event event;
            event.type = Event::GainedFocus;
            pushEvent(event);
            break;
        }

        // Lost focus event
        case FocusOut :
        {
            Event event;
            event.type = Event::LostFocus;
            pushEvent(event);
            break;
        }

        // Resize event
        case ConfigureNotify :
        {
            Event event;
            event.type        = Event::Resized;
            event.size.width  = windowEvent.xconfigure.width;
            event.size.height = windowEvent.xconfigure.height;
            pushEvent(event);
            break;
        }

        // Close event
        case ClientMessage :
        {
            if ((windowEvent.xclient.format == 32) && (windowEvent.xclient.data.l[0]) == static_cast<long>(m_atomClose))  
            {
                Event event;
                event.type = Event::Closed;
                pushEvent(event);
            }
            break;
        }
    }

    return true;
}

} // namespace priv

} // namespace sf
