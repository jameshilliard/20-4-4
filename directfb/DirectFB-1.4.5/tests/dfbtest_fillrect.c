/*
   (c) Copyright 2008  Denis Oliver Kropp

   All rights reserved.

   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <direct/messages.h>

#include <directfb.h>
#include <directfb_strings.h>
#include <directfb_util.h>

#ifdef TIVO
#include <misc/conf.h>
#endif

static const DirectFBPixelFormatNames( format_names );

/**********************************************************************************************************************/

#ifdef TIVO

static inline void dfb_rectangle_set( DFBRectangle *rect,
    int x, int y, int w, int h )
{
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
}

static inline void dfb_rectangle_inset( DFBRectangle *rect,
    int dx, int dy )
{
    rect->x += dx;
    rect->y += dy;
    rect->w -= dx * 2;
    rect->h -= dy * 2;
}

#endif

/**********************************************************************************************************************/

static DFBBoolean
parse_format( const char *arg, DFBSurfacePixelFormat *_f )
{
     int i = 0;

     while (format_names[i].format != DSPF_UNKNOWN) {
          if (!strcasecmp( arg, format_names[i].name )) {
               *_f = format_names[i].format;
               return DFB_TRUE;
          }

          ++i;
     }

     fprintf (stderr, "\nInvalid format specified!\n\n" );

     return DFB_FALSE;
}

static int
print_usage( const char *prg )
{
     int i = 0;

     fprintf (stderr, "\n");
     fprintf (stderr, "== DirectFB Fill Rectangle Test (version %s) ==\n", DIRECTFB_VERSION);
     fprintf (stderr, "\n");
     fprintf (stderr, "Known pixel formats:\n");

     while (format_names[i].format != DSPF_UNKNOWN) {
          DFBSurfacePixelFormat format = format_names[i].format;

          fprintf (stderr, "   %-10s %2d bits, %d bytes",
                   format_names[i].name, DFB_BITS_PER_PIXEL(format),
                   DFB_BYTES_PER_PIXEL(format));

          if (DFB_PIXELFORMAT_HAS_ALPHA(format))
               fprintf (stderr, "   ALPHA");

          if (DFB_PIXELFORMAT_IS_INDEXED(format))
               fprintf (stderr, "   INDEXED");

          if (DFB_PLANAR_PIXELFORMAT(format)) {
               int planes = DFB_PLANE_MULTIPLY(format, 1000);

               fprintf (stderr, "   PLANAR (x%d.%03d)",
                        planes / 1000, planes % 1000);
          }

          fprintf (stderr, "\n");

          ++i;
     }

     fprintf (stderr, "\n");

     fprintf (stderr, "\n");
     fprintf (stderr, "Usage: %s [options]\n", prg);
     fprintf (stderr, "\n");
     fprintf (stderr, "Options:\n");
     fprintf (stderr, "  -h, --help                        Show this help message\n");
     fprintf (stderr, "  -v, --version                     Print version information\n");
     fprintf (stderr, "  -d, --dest      <pixelformat>     Destination pixel format\n");

     return -1;
}

/**********************************************************************************************************************/

int
main( int argc, char *argv[] )
{
     DFBResult               ret;
     int                     i;
#ifdef TIVO
     int                     testNum;
#endif
     DFBSurfaceDescription   desc;
     IDirectFB              *dfb;
     IDirectFBSurface       *dest          = NULL;
     DFBSurfacePixelFormat   dest_format   = DSPF_UNKNOWN;

     /* Initialize DirectFB. */
     ret = DirectFBInit( &argc, &argv );
     if (ret) {
          D_DERROR( ret, "DFBTest/FillRectangle: DirectFBInit() failed!\n" );
          return ret;
     }

#ifdef TIVO
     testNum = 0;
#endif

     /* Parse arguments. */
     for (i=1; i<argc; i++) {
          const char *arg = argv[i];

          if (strcmp( arg, "-h" ) == 0 || strcmp (arg, "--help") == 0)
               return print_usage( argv[0] );
          else if (strcmp (arg, "-v") == 0 || strcmp (arg, "--version") == 0) {
               fprintf (stderr, "dfbtest_blit version %s\n", DIRECTFB_VERSION);
               return false;
          }
          else if (strcmp (arg, "-d") == 0 || strcmp (arg, "--dest") == 0) {
               if (++i == argc) {
                    print_usage (argv[0]);
                    return false;
               }

               if (!parse_format( argv[i], &dest_format ))
                    return false;
          }
#ifdef TIVO
          else if (strcmp (arg, "-t") == 0 || strcmp (arg, "--test") == 0) {
               if (++i == argc) {
                    print_usage (argv[0]);
                    return false;
               }

               testNum = atoi( argv[i] );
          }
#endif
          else
               return print_usage( argv[0] );
     }

     /* Create super interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          D_DERROR( ret, "DFBTest/FillRectangle: DirectFBCreate() failed!\n" );
          return ret;
     }

     /* Fill description for a primary surface. */
     desc.flags = DSDESC_CAPS;
     desc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

     if (dest_format != DSPF_UNKNOWN) {
          desc.flags       |= DSDESC_PIXELFORMAT;
          desc.pixelformat  = dest_format;
     }

     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* Create a primary surface. */
     ret = dfb->CreateSurface( dfb, &desc, &dest );
     if (ret) {
          D_DERROR( ret, "DFBTest/FillRectangle: IDirectFB::CreateSurface() failed!\n" );
          goto out;
     }

     dest->GetSize( dest, &desc.width, &desc.height );
     dest->GetPixelFormat( dest, &desc.pixelformat );

     D_INFO( "DFBTest/FillRectangle: Destination is %dx%d using %s\n",
             desc.width, desc.height, dfb_pixelformat_name(desc.pixelformat) );

#ifdef TIVO
     if (testNum == 1)
     {
         int testWidth  = 1280; // left half of screen
         int testHeight = 720;

         int horzMargin = testWidth  / 10;
         int vertMargin = testHeight / 10;

         int borderSize = 16;

         DFBRectangle surfaceBounds;
         DFBRectangle testBounds;
         DFBRectangle contentBounds;
         DFBRectangle barBounds;
         DFBResult dfbResult;

         dfb_rectangle_set( &surfaceBounds, 0, 0, testWidth, testHeight );

         testBounds = surfaceBounds;
         dfb_rectangle_inset( &testBounds, horzMargin, vertMargin );

         contentBounds = testBounds;
         dfb_rectangle_inset( &contentBounds, borderSize, borderSize );

         barBounds = contentBounds;
         dfb_rectangle_inset( &barBounds, 8, 8 );

         barBounds.w = 16;

         while (barBounds.x + barBounds.w < contentBounds.x + contentBounds.w )
         {
             // green
             dest->SetColor( dest, 0x00 /* r */, 0xFF /* g */, 0x00 /* b */, 0xFF /* a */ );
             dest->FillRectangle( dest, surfaceBounds.x, surfaceBounds.y, surfaceBounds.w, surfaceBounds.h );

             // orange
             dest->SetColor( dest, 0xFF /* r */, 0x66 /* g */, 0x00 /* b */, 0xFF /* a */ );
             dest->FillRectangle( dest, testBounds.x, testBounds.y, testBounds.w, testBounds.h );

             // yellow
             dest->SetColor( dest, 0xFF /* r */, 0xFF /* g */, 0x00 /* b */, 0xFF /* a */ );
             dest->FillRectangle( dest, contentBounds.x, contentBounds.y, contentBounds.w, contentBounds.h );

             // violet
             dest->SetColor( dest, 0xCC /* r */, 0x33 /* g */, 0xFF /* b */, 0xFF /* a */ );
             dest->FillRectangle( dest, barBounds.x, barBounds.y, barBounds.w, barBounds.h );

             // red
             dest->SetColor( dest, 0xFF /* r */, 0x00 /* g */, 0x00 /* b */, 0xFF /* a */ );

             dfbResult = dest->DrawLine( dest, contentBounds.x, contentBounds.y,
                 contentBounds.x + contentBounds.w, contentBounds.y + contentBounds.h );

             dfbResult = dest->DrawLine( dest, contentBounds.x + contentBounds.w, contentBounds.y,
                 contentBounds.x, contentBounds.y + contentBounds.h );

             dest->Flip( dest, NULL, DSFLIP_NONE );

             // testing: see if we can capture the surface image
             //dest->Dump( dest, dfb_config->screenshot_dir, "dfbtest_fillrect" );

             // delay to under 20 fps
             usleep( 50 * 1000 );

             barBounds.x += 16 + 8;
         }
     }
     else
     {
         int    j;
         int    top;
         int    left;

         top = 60;
         left = 80;

         // exit the test after running a few iterations
         for (j=0; j<3; j++)
         {
             // only draw a fairly small number of rectangles so
             // the diagnostic logging is fairly small
             for (i=0; i<3; i++)
             {
                 int rgb = 0xFF << ((2 - i) * 8);
                 int r = (rgb >> 16) & 0xFF;
                 int g = (rgb >>  8) & 0xFF;
                 int b = (rgb      ) & 0xFF;
                 int a = 0x80;

                 dest->SetColor( dest, r, g, b, a );
                 dest->FillRectangle( dest, left + i * (120 * 2) /* x */,
                     top /* y */, 160 /* w */, 120 /* h */ );
             }

             dest->Flip( dest, NULL, DSFLIP_NONE );

             // testing: see if we can capture the surface image
             dest->Dump( dest, dfb_config->screenshot_dir, "dfbtest_fillrect" );

             sleep( 1 );

             top += 24;
             left += 32;
         }
     }
#else
      while (true) {
          for (i=0; i<100000; i++) {
               dest->SetColor( dest, rand()%256, rand()%256, rand()%256, rand()%256 );
               dest->FillRectangle( dest, rand()%100, rand()%100, rand()%100, rand()%100 );
          }

          dest->Flip( dest, NULL, DSFLIP_NONE );
     }
#endif

out:
     if (dest)
          dest->Release( dest );

     /* Shutdown DirectFB. */
     dfb->Release( dfb );

     return ret;
}

