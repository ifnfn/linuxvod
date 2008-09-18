#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "SDL_image.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Table of image detection and loading functions */
static struct {
	char *type;
	int (SDLCALL *is)(SDL_RWops *src);
	SDL_Surface *(SDLCALL *load)(SDL_RWops *src);
} supported[] = {
	/* keep magicless formats first */
#ifdef BMP
	{ "BMP", IMG_isBMP, IMG_LoadBMP_RW },
#endif
#ifdef PNM
	{ "PNM", IMG_isPNM, IMG_LoadPNM_RW }, /* P[BGP]M share code */
#endif
#ifdef XPM
	{ "XPM", IMG_isXPM, IMG_LoadXPM_RW },
#endif
#ifdef XCF
	{ "XCF", IMG_isXCF, IMG_LoadXCF_RW },
#endif
#ifdef PCX
	{ "PCX", IMG_isPCX, IMG_LoadPCX_RW },
#endif
#ifdef GIF
	{ "GIF", IMG_isGIF, IMG_LoadGIF_RW },
#endif
#ifdef JPG
	{ "JPG", IMG_isJPG, IMG_LoadJPG_RW },
#endif
#ifdef TIF
	{ "TIF", IMG_isTIF, IMG_LoadTIF_RW },
#endif
#ifdef LBM
	{ "LBM", IMG_isLBM, IMG_LoadLBM_RW },
#endif
#ifdef PNG
	{ "PNG", IMG_isPNG, IMG_LoadPNG_RW }
#endif
};

const SDL_version *IMG_Linked_Version(void)
{
	static SDL_version linked_version;
	SDL_IMAGE_VERSION(&linked_version);
	return(&linked_version);
}

/* Load an image from a file */
SDL_Surface *IMG_Load(const char *file)
{
    SDL_RWops *src = SDL_RWFromFile(file, "rb");
    char *ext = strrchr(file, '.');
    if(ext) {
        ext++;
    }
    if(!src) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }
    return IMG_LoadTyped_RW(src, 1, ext);
}

/* Load an image from an SDL datasource (for compatibility) */
SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc)
{
    return IMG_LoadTyped_RW(src, freesrc, NULL);
}

/* Portable case-insensitive string compare function */
static int IMG_string_equals(const char *str1, const char *str2)
{
	while ( *str1 && *str2 ) {
		if ( toupper((unsigned char)*str1) !=
		     toupper((unsigned char)*str2) )
			break;
		++str1;
		++str2;
	}
	return (!*str1 && !*str2);
}

/* Load an image from an SDL datasource, optionally specifying the type */
SDL_Surface *IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, char *type)
{
	int i, start;
	SDL_Surface *image;

	/* Make sure there is something to do.. */
	if ( src == NULL ) {
		IMG_SetError("Passed a NULL data source");
		return(NULL);
	}

	/* See whether or not this data source can handle seeking */
	if ( SDL_RWseek(src, 0, SEEK_CUR) < 0 ) {
		IMG_SetError("Can't seek in this data source");
		if(freesrc)
			SDL_RWclose(src);
		return(NULL);
	}

	/* Detect the type of image being loaded */
	start = SDL_RWtell(src);
	image = NULL;
	for ( i=0; i < ARRAYSIZE(supported); ++i ) {
		if(supported[i].is) {
			SDL_RWseek(src, start, SEEK_SET);
			if(!supported[i].is(src))
				continue;
		} else {
			/* magicless format */
			if(!type
			   || !IMG_string_equals(type, supported[i].type))
				continue;
		}
#ifdef DEBUG_IMGLIB
		fprintf(stderr, "IMGLIB: Loading image as %s\n", supported[i].type);
#endif
		SDL_RWseek(src, start, SEEK_SET);
		image = supported[i].load(src);
		if(freesrc)
			SDL_RWclose(src);
		return image;
	}

	if ( freesrc ) {
		SDL_RWclose(src);
	}
	IMG_SetError("Unsupported image format");
	return NULL;
}

/* Invert the alpha of a surface for use with OpenGL
   This function is a no-op and only kept for backwards compatibility.
 */
int IMG_InvertAlpha(int on)
{
    return 1;
}
