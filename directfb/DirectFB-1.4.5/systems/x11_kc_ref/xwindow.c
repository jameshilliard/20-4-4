/*
   (c) Copyright 2001-2009  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <directfb_util.h>

#include <direct/mem.h>

#include "x11.h"

D_DEBUG_DOMAIN( X11_Window, "X11/Window", "X11 Window handling" );

static int error_code = 0;

static int
error_handler( Display *display, XErrorEvent *event )
{
     char buf[512];

     D_DEBUG_AT( X11_Window, "%s()\n", __FUNCTION__ );

     XGetErrorText( display, event->error_code, buf, sizeof(buf) );

     D_ERROR( "X11/Window: Error! %s\n", buf );

     error_code = event->error_code;

     return 0;
}

Bool
dfb_x11_open_window( DFBX11 *x11, XWindow** ppXW, int iXPos, int iYPos, int iWidth, int iHeight, DFBSurfacePixelFormat format )
{
     XWindow              *xw;
     XSetWindowAttributes  attr = { .background_pixmap = 0 };
     void                 *old_error_handler = 0;

     D_DEBUG_AT( X11_Window, "Creating %4dx%4d %s window...\n", iWidth, iHeight, dfb_pixelformat_name(format) );

     xw = D_CALLOC( 1, sizeof(XWindow) );
     if (!xw)
          return D_OOM();

     /* We set the structure as needed for our window */
     xw->width   = iWidth;
     xw->height  = iHeight;
     xw->display = x11->display;

     xw->screenptr = DefaultScreenOfDisplay(xw->display);
     xw->screennum = DefaultScreen(xw->display);
     xw->depth     = DefaultDepthOfScreen(xw->screenptr);
     xw->visual    = DefaultVisualOfScreen(xw->screenptr);

     attr.event_mask =
            ButtonPressMask
          | ButtonReleaseMask
          | PointerMotionMask
          | KeyPressMask
          | KeyReleaseMask
          | ExposureMask
          | StructureNotifyMask;

     XLockDisplay( x11->display );

     old_error_handler = XSetErrorHandler( error_handler );

     error_code = 0;

     xw->window = XCreateWindow( xw->display,
                                 RootWindowOfScreen(xw->screenptr),
                                 iXPos, iYPos, iWidth, iHeight, 0, xw->depth, InputOutput,
                                 xw->visual, CWEventMask, &attr );
     XSync( xw->display, False );
     if (!xw->window || error_code) {
          D_FREE( xw );
          XUnlockDisplay( x11->display );
          return False;
     }


     XSizeHints Hints;

     /*
      * Here we inform the function of what we are going to change for the
      * window (there's also PPosition but it's obsolete)
      */
     Hints.flags    =    PSize | PMinSize | PMaxSize;

     /*
      * Now we set the structure to the values we need for width & height.
      * For esthetic reasons we set Width=MinWidth=MaxWidth.
      * The same goes for Height. You can try whith differents values, or
      * let's use Hints.flags=Psize; and resize your window..
      */
     Hints.min_width          =    Hints.max_width          =    Hints.base_width    =    xw->width;
     Hints.min_height    =    Hints.max_height    =    Hints.base_height   =    xw->height;

     /* Now we can set the size hints for the specified window */
     XSetWMNormalHints(xw->display,xw->window,&Hints);

     /* We change the title of the window (default:Untitled) */
     XStoreName(xw->display,xw->window,"DFB X11 system window");

     xw->gc = XCreateGC(xw->display, xw->window, 0, NULL);

     // Create a null cursor
     Pixmap  pixmp1;
     Pixmap  pixmp2;
     XColor  fore;
     XColor  back;
     char    zero = 0;

     pixmp1 = XCreateBitmapFromData( xw->display, xw->window, &zero, 1, 1 );
     pixmp2 = XCreateBitmapFromData( xw->display, xw->window, &zero, 1, 1 );

     xw->NullCursor = XCreatePixmapCursor( xw->display, pixmp1, pixmp2, &fore, &back, 0, 0 );

     XFreePixmap ( xw->display, pixmp1 );
     XFreePixmap ( xw->display, pixmp2 );

     XDefineCursor( xw->display, xw->window, xw->NullCursor );


     /* maps the window and raises it to the top of the stack */
     XMapRaised( xw->display, xw->window );

     {
          int pitch;

          xw->bpp = (xw->depth > 16) ? 4 :
                    (xw->depth >  8) ? 2 : 1;

          pitch = (xw->bpp * xw->width + 3) & ~3;

          /* Use malloc(), not D_MALLOC() here, because XCreateImage()
           * will call free() on this data.
           */
          xw->virtualscreen = malloc ( 2 * xw->height * pitch );

          xw->ximage = XCreateImage( xw->display, xw->visual, xw->depth, ZPixmap, 0,
                                     xw->virtualscreen, xw->width, xw->height * 2, 32, pitch );
          XSync( xw->display, False );
          if (!xw->ximage || error_code) {
               D_ERROR( "X11/Window: XCreateImage( Visual %02lu, depth %d, size %dx%d, buffer %p [%d] ) failed!\n",
                        xw->visual->visualid, xw->depth, xw->width, xw->height * 2, xw->virtualscreen, pitch );
               XFreeGC(xw->display,xw->gc);
               XDestroyWindow(xw->display,xw->window);
               XSetErrorHandler( old_error_handler );
               XUnlockDisplay( x11->display );
               D_FREE( xw );
               return False;
          }
     }

     XSetErrorHandler( old_error_handler );

     XUnlockDisplay( x11->display );

     (*ppXW) = xw;

     return True;
}

void
dfb_x11_close_window( DFBX11 *x11, XWindow* xw )
{
     XDestroyImage( xw->ximage );

     XFreeGC( xw->display, xw->gc );
     XDestroyWindow( xw->display, xw->window );
     XFreeCursor( xw->display, xw->NullCursor );

     D_FREE( xw );
}

