#ifndef SDL_RWNET_H
#define SDL_RWNET_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif
	
SDL_RWops * SDLCALL SDL_RWFromUrl(const char *url, int mode);
#ifdef __cplusplus
}
#endif
	

#endif

