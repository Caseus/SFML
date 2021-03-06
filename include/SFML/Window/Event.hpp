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

#ifndef SFML_EVENT_HPP
#define SFML_EVENT_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Config.hpp>


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Defines a system event and its parameters
///
////////////////////////////////////////////////////////////
class Event
{
public :

    ////////////////////////////////////////////////////////////
    /// \brief Size events parameters (Resized)
    ///
    ////////////////////////////////////////////////////////////
    struct SizeEvent
    {
        unsigned int width;  ///< New width, in pixels
        unsigned int height; ///< New height, in pixels
    };

    ////////////////////////////////////////////////////////////
    /// \brief Enumeration of the different types of events
    ///
    ////////////////////////////////////////////////////////////
    enum EventType
    {
        Closed,                 ///< The window requested to be closed (no data)
        Resized,                ///< The window was resized (data in event.size)
        LostFocus,              ///< The window lost the focus (no data)
        GainedFocus,            ///< The window gained the focus (no data)

        Count                   ///< Keep last -- the total number of event types
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    EventType type; ///< Type of the event

    union
    {
        SizeEvent            size;            ///< Size event parameters (Event::Resized)
    };
};

} // namespace sf


#endif // SFML_EVENT_HPP


////////////////////////////////////////////////////////////
/// \class sf::Event
/// \ingroup window
///
/// sf::Event holds all the informations about a system event
/// that just happened. Events are retrieved using the
/// sf::Window::pollEvent and sf::Window::waitEvent functions.
///
/// A sf::Event instance contains the type of the event
/// (window closed, ...) as well
/// as the details about this particular event. Please note that
/// the event parameters are defined in a union, which means that
/// only the member matching the type of the event will be properly
/// filled; all other members will have undefined values and must not
/// be read if the type of the event doesn't match.
///
/// Usage example:
/// \code
/// sf::Event event;
/// while (window.pollEvent(event))
/// {
///     // Request for closing the window
///     if (event.type == sf::Event::Closed)
///         window.close();
///
///     // The window was resized
///     if (event.type == sf::Event::Resized)
///         doSomethingWithTheNewSize(event.size.width, event.size.height);
///
///     // etc ...
/// }
/// \endcode
///
////////////////////////////////////////////////////////////
