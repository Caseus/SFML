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
#include <SFML/Window/OSX/WindowImplCocoa.hpp>
#include <SFML/System/Err.hpp>

#import <SFML/Window/OSX/SFOpenGLView.h>

////////////////////////////////////////////////////////////
/// Erase (replace with 0) the given bits mask from the given data bits.
///
////////////////////////////////////////////////////////////
NSUInteger eraseMaskFromData(NSUInteger data, NSUInteger mask);

////////////////////////////////////////////////////////////
/// Erase (replace with 0) everything execept the given bits mask from the given data bits.
///
////////////////////////////////////////////////////////////
NSUInteger keepOnlyMaskFromData(NSUInteger data, NSUInteger mask);

////////////////////////////////////////////////////////////
/// SFOpenGLView class : Privates Methods Declaration
///
////////////////////////////////////////////////////////////
@interface SFOpenGLView ()

////////////////////////////////////////////////////////////
/// Handle view resized event.
/// 
////////////////////////////////////////////////////////////
-(void)frameDidChange:(NSNotification *)notification;

@end

@implementation SFOpenGLView

#pragma mark
#pragma mark SFOpenGLView's methods

////////////////////////////////////////////////////////
-(id)initWithFrame:(NSRect)frameRect
{
    if ((self = [super initWithFrame:frameRect])) {
        [self setRequesterTo:0];
        m_realSize = NSZeroSize;
        [self initModifiersState];
        
        // Register for resize event
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center addObserver:self
                   selector:@selector(frameDidChange:)
                       name:NSViewFrameDidChangeNotification
                     object:self];
    }

    return self;
}


////////////////////////////////////////////////////////
-(void)setRequesterTo:(sf::priv::WindowImplCocoa *)requester
{
    m_requester = requester;
}


////////////////////////////////////////////////////////
-(void)setRealSize:(NSSize)newSize
{
    m_realSize = newSize;
}

////////////////////////////////////////////////////////
-(void)frameDidChange:(NSNotification *)notification
{
    // Update the OGL view to fit the new size.
    [self update];
    
    // Send an event
    if (m_requester == 0) return;
    
    // The new size
    NSSize newSize = [self frame].size;
    m_requester->windowResized(newSize.width, newSize.height);
}

#pragma mark
#pragma mark Subclassing methods


////////////////////////////////////////////////////////
-(void)dealloc
{
    // Unregister
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self removeTrackingRect:m_trackingTag];
    
    [super dealloc];
}


////////////////////////////////////////////////////////

@end


#pragma mark - C-like functions


////////////////////////////////////////////////////////
NSUInteger eraseMaskFromData(NSUInteger data, NSUInteger mask)
{
    return (data | mask) ^ mask;
}


////////////////////////////////////////////////////////
NSUInteger keepOnlyMaskFromData(NSUInteger data, NSUInteger mask)
{
    NSUInteger negative = NSUIntegerMax ^ mask;
    return eraseMaskFromData(data, negative);
}


