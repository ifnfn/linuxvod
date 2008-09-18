#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include "mousecursor.h"
#include "SDL/SDL_drawlibs.h"
#include "SDL_image/SDL_image.h"

#define USE_ALPHA 0
#define IGNORE_ALPHA 1
#define UPDATE true

SDL_Surface* sdl_surface_from_file(const char *file, int use_alpha)
{
	SDL_Surface* sdl_surface;
	SDL_Surface* temp;

	temp = IMG_Load(file);

	if (temp == NULL)
		abort();

	if(use_alpha == IGNORE_ALPHA)
		sdl_surface = SDL_DisplayFormat(temp);
	else
		sdl_surface = SDL_DisplayFormatAlpha(temp);

	if (sdl_surface == NULL)
		abort();

	if (use_alpha == IGNORE_ALPHA)
		SDL_SetAlpha(sdl_surface, 0, 0);
	SDL_FreeSurface(temp);

	return sdl_surface;
}

int MouseCursor::draw_part(float sx, float sy, float x, float y, float w, float h, Uint8 alpha, bool update)
{
	SDL_Rect src, dest;

	src.x = (int)sx;
	src.y = (int)sy;
	src.w = (int)w;
	src.h = (int)h;

	dest.x = (int)x;
	dest.y = (int)y;
	dest.w = (int)w;
	dest.h = (int)h;

	if(alpha != 255) {
		/* Create a Surface, make it using colorkey, blit surface into temp, apply alpha
		to temp sur, blit the temp into the screen */
		/* Note: this has to be done, since SDL doesn't allow to set alpha to surfaces that
		already have an alpha mask yet... */

		SDL_Surface* sdl_surface_copy = SDL_CreateRGBSurface (cursor->flags,
			cursor->w, cursor->h, cursor->format->BitsPerPixel,
			cursor->format->Rmask, cursor->format->Gmask,
			cursor->format->Bmask,
			0);
		int colorkey = SDL_MapRGB(sdl_surface_copy->format, 255, 0, 255);
		SDL_FillRect(sdl_surface_copy, NULL, colorkey);
		SDL_SetColorKey(sdl_surface_copy, SDL_SRCCOLORKEY, colorkey);

		SDL_BlitSurface(cursor, NULL, sdl_surface_copy, NULL);
		SDL_SetAlpha(sdl_surface_copy ,SDL_SRCALPHA,alpha);

		int ret = SDL_BlitSurface(sdl_surface_copy, NULL, screen, &dest);

		if (update == UPDATE)
			SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

		SDL_FreeSurface (sdl_surface_copy);
		return ret;
	}

	int ret = SDL_BlitSurface(cursor, &src, screen, &dest);

	if (update == UPDATE)
		SDL_UpdateRect(screen, dest.x, dest.y, dest.w, dest.h);

	return ret;
}

MouseCursor::MouseCursor(SDL_Surface** back,SDL_Surface *mainscreen, const char *cursor_file, int frames):mid_x(0), mid_y(0)
{
	screen = mainscreen;
	shadow = back;
	cursor = sdl_surface_from_file(cursor_file, USE_ALPHA);

	int x, y;
	x = screen->w/2;
	y = screen->h/2;
#if 0
	SDL_WarpMouse(x, y);
	SDL_GetMouseState(&x,&y);
	postions[1].x = x-mid_x;
	postions[1].y = y-mid_y;
	postions[1].w = cursor->w / tot_frames;
	postions[1].h = cursor->h / MC_STATES_NB;
	postions[0] = postions[1];
#endif
	cur_frame = 0;
	tot_frames = frames;
	SetMouseXy(x, y);
	cur_state = MC_NORMAL;
	SDL_ShowCursor(SDL_DISABLE);
}

MouseCursor::~MouseCursor()
{
	delete cursor;
	SDL_ShowCursor(SDL_ENABLE);
}

void MouseCursor::SetMouseXy(int x, int y)
{
	SDL_WarpMouse(x, y);
	SDL_GetMouseState(&x,&y);
	postions[1].x = x-mid_x;
	postions[1].y = y-mid_y;
	if (tot_frames)
		postions[1].w = cursor->w / tot_frames;
	postions[1].h = cursor->h / MC_STATES_NB;
	postions[0] = postions[1];
}

int MouseCursor::state()
{
	return cur_state;
}

void MouseCursor::set_state(int nstate)
{
	cur_state = nstate;
}

void MouseCursor::set_mid(int x, int y)
{
	mid_x = x;
	mid_y = y;
}

void MouseCursor::draw()
{
	int x,y,w,h;
	Uint8 ispressed = SDL_GetMouseState(&x,&y);
	w = cursor->w / tot_frames;
	h = cursor->h / MC_STATES_NB;
	if(ispressed &SDL_BUTTON(1) || ispressed &SDL_BUTTON(2) || ispressed & SDL_BUTTON(3))
	{
		if(cur_state != MC_CLICK) {
			state_before_click = cur_state;
			cur_state = MC_CLICK;
		}
	}
	else {
		if(cur_state == MC_CLICK)
			cur_state = state_before_click;
	}

	SDL_BlitSurface(*shadow, &postions[0], screen, &postions [0]);
	SDL_UpdateRects(screen, 1, &postions[0]);
	SDL_PumpEvents();
	postions[1].x=x-mid_x;
	postions[1].y=y-mid_y;
	postions[1].w=w;
	postions[1].h=h;
	postions[0] = postions[1];
	draw_part(w*cur_frame, h*cur_state , postions[1].x, postions[1].y, w, h, 255, true);
}
