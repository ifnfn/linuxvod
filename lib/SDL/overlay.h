#ifndef OVERLAY_H
#define OVERLAY_H

void ConvertRGBtoYV12(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance);
void ConvertRGBtoIYUV(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance);
void ConvertRGBtoUYVY(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance);
void ConvertRGBtoYVYU(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance);
void ConvertRGBtoYUY2(SDL_Surface *s, SDL_Overlay *o, int monochrome, int luminance);

#endif
