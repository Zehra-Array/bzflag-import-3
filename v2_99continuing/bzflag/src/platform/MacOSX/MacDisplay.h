/* bzflag
 * Copyright (c) 1993-2010 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef BZF_MACDISPLAY_H
#define BZF_MACDISPLAY_H

#include "common.h"
#include "platform/BzfDisplay.h"
#include "BzfEvent.h"
#include "bzfgl.h"

#include <Carbon/Carbon.h>

#define MAC_FG_SLEEP 3 /* ticks to sleep when in foreground */
#define MAC_BG_SLEEP 4 /* ticks to sleep when in background */

class MacDisplay : public BzfDisplay {
  public:
    MacDisplay();
    MacDisplay(const char* name, const char* videoFormat);
    ~MacDisplay() {};

    bool isValid() const { return is_valid;   }
    bool isEventPending() const;

    bool getEvent(BzfEvent&) const;
    bool peekEvent(BzfEvent&) const;
    //void    setPending (bool val) const { pending = val; }

    int getWidth() const;
    int getHeight() const;

    int getPassthroughWidth() const;
    int getPassthroughHeight() const;

    //GDHandle getDevice () const { return screen_device; }
    //CGrafPtr getWindow () const { return window; }
    void setWindow(CGrafPtr win) const { window = win; }

    //AGLContext getContext () const { return context; }
    void setContext(CGLContextObj ctx) const { context = ctx;  }

  private:

    bool doSetResolution(int) { return false; }

    void getKey(BzfKeyEvent& bzf_event, char key, ::UInt32 code) const;

    //ResInfo *res_info;
    int screen_width;
    int screen_height;
    //int     screen_depth;

    bool is_valid;

    static bool pending;

    static CGLContextObj  context;
    static CGrafPtr    window;

    CGMutablePathRef   cursor_region;

    //GDHandle    screen_device;
};

#endif // BZF_MACDISPLAY_H
//BZF_DEFINE_ALIST(MacDisplayResList, MacDisplayRes);

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8 expandtab
