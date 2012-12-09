////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Marco Antognini (antognini.marco@gmail.com), 
//                         Laurent Gomila (laurent.gom@gmail.com), 
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
#import <AppKit/AppKit.h>

namespace sf {
    namespace priv {
        class WindowImplCocoa;
    }
}

////////////////////////////////////////////////////////////
/// \brief Spesialized NSOpenGLView
///
/// Handle event and send them back to the requester.
///
////////////////////////////////////////////////////////////
@interface SFOpenGLView : NSOpenGLView {
    sf::priv::WindowImplCocoa*    m_requester;
    NSTrackingRectTag             m_trackingTag;
    NSSize                        m_realSize;
}

////////////////////////////////////////////////////////////
/// Create the SFML opengl view to fit the given area.
/// 
////////////////////////////////////////////////////////////
-(id)initWithFrame:(NSRect)frameRect;

////////////////////////////////////////////////////////////
/// Apply the given resquester to the view.
/// 
////////////////////////////////////////////////////////////
-(void)setRequesterTo:(sf::priv::WindowImplCocoa *)requester;

////////////////////////////////////////////////////////////
/// Set the real size of view (it should be the back buffer size).
/// If not set, or set to its default value NSZeroSize
/// 
////////////////////////////////////////////////////////////
-(void)setRealSize:(NSSize)newSize;

@end
