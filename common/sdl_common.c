/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "SDL.h"

#include "console.h"
#include "qtypes.h"
#include "sdl_common.h"
#include "sys.h"
#include "vid.h"
#include "zone.h"

SDL_Window *sdl_window = NULL;

#include "SDL.h"

// The icon header is generated by ImageMagick
#define MagickImage tyrquake_icon_128
#include "tyrquake_icon_128.h"
#undef MagickImage

SDL_PixelFormat *sdl_desktop_format = NULL;

void
VID_SDL_SetIcon()
{
    SDL_Surface *surface;

    surface = SDL_CreateRGBSurfaceWithFormatFrom((void *)tyrquake_icon_128,
                                                 128, 128, 32, 128 * 4, SDL_PIXELFORMAT_ABGR8888);
    if (!surface)
        return;

    SDL_SetWindowIcon(sdl_window, surface);
    SDL_FreeSurface(surface);
}

void
VID_SDL_InitModeList(void)
{
    int i, err;
    int displays, sdlmodes;
    SDL_DisplayMode sdlmode;
    qvidmode_t *mode;
    qvidformat_t *format;

    /* Query the desktop mode's pixel format */
    err = SDL_GetDesktopDisplayMode(0, &sdlmode);
    if (err)
	Sys_Error("%s: Unable to query desktop display mode (%s)",
		  __func__, SDL_GetError());
    sdl_desktop_format = SDL_AllocFormat(sdlmode.format);
    if (!sdl_desktop_format)
	Sys_Error("%s: Unable to allocate desktop pixel format (%s)",
		  __func__, SDL_GetError());

    /* Setup the default windowed mode */
    mode = &vid_windowed_mode;
    mode->bpp = sdl_desktop_format->BitsPerPixel;
    format = (qvidformat_t *)mode->driverdata;
    format->format = sdl_desktop_format->format;
    mode->refresh = sdlmode.refresh_rate;
    mode->width = 640;
    mode->height = 480;

    displays = SDL_GetNumVideoDisplays();
    if (displays < 1)
	Sys_Error("%s: no displays found (%s)", __func__, SDL_GetError());

    /* FIXME - allow use of more than one display */
    sdlmodes = SDL_GetNumDisplayModes(0);
    if (sdlmodes < 0)
	Con_SafePrintf("%s: error enumerating SDL display modes (%s)\n",
		       __func__, SDL_GetError());

    vid_modelist = Hunk_AllocName(sdlmodes * sizeof(qvidmode_t), "vidmodes");

    /*
     * Check availability of fullscreen modes
     * (default to display 0 for now)
     */
    mode = vid_modelist;
    vid_nummodes = 0;
    for (i = 0; i < sdlmodes; i++) {
	err = SDL_GetDisplayMode(0, i, &sdlmode);
	if (err)
	    Sys_Error("%s: couldn't get mode %d info (%s)",
		      __func__, i, SDL_GetError());

	Sys_Printf("%s: checking mode %i: %dx%d, %s\n", __func__,
		   i, sdlmode.w, sdlmode.h, SDL_GetPixelFormatName(sdlmode.format));

	if (SDL_PIXELTYPE(sdlmode.format) == SDL_PIXELTYPE_PACKED32)
	    vid_modelist[vid_nummodes].bpp = 32;
	else if (SDL_PIXELTYPE(sdlmode.format) == SDL_PIXELTYPE_PACKED16)
	    vid_modelist[vid_nummodes].bpp = 16;
	else
	    continue;

	mode->width = sdlmode.w;
	mode->height = sdlmode.h;
	mode->refresh = sdlmode.refresh_rate;
	format = (qvidformat_t *)mode->driverdata;
	format->format = sdlmode.format;
	vid_nummodes++;
	mode++;
    }

    VID_SortModeList(vid_modelist, vid_nummodes);
}

void
Q_SDL_InitOnce(void)
{
    static qboolean init_done = false;

    if (init_done)
	return;

    if (SDL_Init(0) < 0)
	Sys_Error("SDL_Init(0) failed: %s", SDL_GetError());

    init_done = true;
}
