#include <stdio.h>

#include "SDL_image.h"

/* See if an image is contained in a data source */
int IMG_isXXX(SDL_RWops *src)
{
	int is_XXX;

	is_XXX = 0;

	return(is_XXX);
}

/* Load a XXX type image from an SDL datasource */
SDL_Surface *IMG_LoadXXX_RW(SDL_RWops *src)
{
	if ( !src ) {
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	return(NULL);
}

