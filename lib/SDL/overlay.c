/********************************************************************************
 *                                                                              *
 * Test of the overlay used for moved pictures, test more closed to real life.  *
 * Running trojan moose :) Coded by Mike Gorchak.                               *
 *                                                                              *
 ********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>

#include "overlay.h"

void RGBtoYUV(Uint8 *rgb, int *yuv, int monochrome, int luminance)
{
	if (monochrome)
	{
#if 1 /* these are the two formulas that I found on the FourCC site... */
		yuv[0] = (int) (0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2]);
		yuv[1] = 128;
		yuv[2] = 128;
#else
		yuv[0] = (int) ((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
		yuv[1] = 128;
		yuv[2] = 128;
#endif
	}
	else
	{
#if 1 /* these are the two formulas that I found on the FourCC site... */
		yuv[0] = (int) (0.299*rgb[0] + 0.587*rgb[1] + 0.114*rgb[2]);
		yuv[1] = (int) ((rgb[2]-yuv[0])*0.565 + 128);
		yuv[2] = (int) ((rgb[0]-yuv[0])*0.713 + 128);
#else
		yuv[0] = (int) ((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
		yuv[1] = (int) (128 - (0.148 * rgb[0]) - (0.291 * rgb[1]) + (0.439 * rgb[2]));
		yuv[2] = (int) (128 + (0.439 * rgb[0]) - (0.368 * rgb[1]) - (0.071 * rgb[2]));
#endif
	}

	if (luminance!=100)
	{
		yuv[0]=yuv[0]*luminance/100;
		if (yuv[0]>255)
			yuv[0]=255;
	}
}

void ConvertRGBtoYV12(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance)
{
	int x,y;
	int yuv[3];
	Uint8 *p,*op[3];

	SDL_LockSurface(s);
	SDL_LockYUVOverlay(o);

	/* Convert */
	for(y=0; y<s->h && y<o->h; y++)
	{
		p=((Uint8 *) s->pixels)+s->pitch*y;
		op[0]=o->pixels[0]+o->pitches[0]*y;
		op[1]=o->pixels[1]+o->pitches[1]*(y/2);
		op[2]=o->pixels[2]+o->pitches[2]*(y/2);
		for(x=0; x<s->w && x<o->w; x++)
		{
			RGBtoYUV(p, yuv, monochrome, luminance);
			*(op[0]++)=yuv[0];
			if(x%2==0 && y%2==0)
			{
				*(op[1]++)=yuv[2];
				*(op[2]++)=yuv[1];
			}
			p+=s->format->BytesPerPixel;
		}
	}

	SDL_UnlockYUVOverlay(o);
	SDL_UnlockSurface(s);
}

void ConvertRGBtoIYUV(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance)
{
	int x,y;
	int yuv[3];
	Uint8 *p,*op[3];

	SDL_LockSurface(s);
	SDL_LockYUVOverlay(o);

	/* Convert */
	for(y=0; y<s->h && y<o->h; y++)
	{
		p=((Uint8 *) s->pixels)+s->pitch*y;
		op[0]=o->pixels[0]+o->pitches[0]*y;
		op[1]=o->pixels[1]+o->pitches[1]*(y/2);
		op[2]=o->pixels[2]+o->pitches[2]*(y/2);
		for(x=0; x<s->w && x<o->w; x++)
		{
			RGBtoYUV(p,yuv, monochrome, luminance);
			*(op[0]++)=yuv[0];
			if(x%2==0 && y%2==0)
			{
				*(op[1]++)=yuv[1];
				*(op[2]++)=yuv[2];
			}
			p+=s->format->BytesPerPixel;
		}
	}

	SDL_UnlockYUVOverlay(o);
	SDL_UnlockSurface(s);
}

void ConvertRGBtoUYVY(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance)
{
	int x,y;
	int yuv[3];
	Uint8 *p,*op;

	SDL_LockSurface(s);
	SDL_LockYUVOverlay(o);

	for(y=0; y<s->h && y<o->h; y++)
	{
		p=((Uint8 *) s->pixels)+s->pitch*y;
		op=o->pixels[0]+o->pitches[0]*y;
		for(x=0; x<s->w && x<o->w; x++)
		{
			RGBtoYUV(p, yuv, monochrome, luminance);
			if(x%2==0)
			{
				*(op++)=yuv[1];
				*(op++)=yuv[0];
				*(op++)=yuv[2];
			}
			else
				*(op++)=yuv[0];

			p+=s->format->BytesPerPixel;
		}
	}

	SDL_UnlockYUVOverlay(o);
	SDL_UnlockSurface(s);
}

void ConvertRGBtoYVYU(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance)
{
	int x,y;
	int yuv[3];
	Uint8 *p,*op;

	SDL_LockSurface(s);
	SDL_LockYUVOverlay(o);

	for(y=0; y<s->h && y<o->h; y++)
	{
		p=((Uint8 *) s->pixels)+s->pitch*y;
		op=o->pixels[0]+o->pitches[0]*y;
		for(x=0; x<s->w && x<o->w; x++)
		{
			RGBtoYUV(p,yuv, monochrome, luminance);
			if(x%2==0)
			{
				*(op++)=yuv[0];
				*(op++)=yuv[2];
				op[1]=yuv[1];
			}
			else
			{
				*op=yuv[0];
				op+=2;
			}

			p+=s->format->BytesPerPixel;
		}
	}

	SDL_UnlockYUVOverlay(o);
	SDL_UnlockSurface(s);
}

void ConvertRGBtoYUY2(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance)
{
	int x,y;
	int yuv[3];
	Uint8 *p,*op;

	SDL_LockSurface(s);
	SDL_LockYUVOverlay(o);

	for(y=0; y<s->h && y<o->h; y++)
	{
		p=((Uint8 *) s->pixels)+s->pitch*y;
		op=o->pixels[0]+o->pitches[0]*y;
		for(x=0; x<s->w && x<o->w; x++)
		{
			RGBtoYUV(p,yuv, monochrome, luminance);
			if(x%2==0)
			{
				*(op++)=yuv[0];
				*(op++)=yuv[1];
				op[1]=yuv[2];
			}
			else
			{
				*op=yuv[0];
				op+=2;
			}

			p+=s->format->BytesPerPixel;
		}
	}

	SDL_UnlockYUVOverlay(o);
	SDL_UnlockSurface(s);
}

