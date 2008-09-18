#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "memtest.h"

#include "gui.h"
#include "win.h"
#include "player.h"
#include "xmltheme.h"

#define ConvertFSizeToPixer(Pixer) (size_t)(Pixer*1.0 * 4) / 3

#ifndef UNICODE_BOM_NATIVE

#define UNICODE_BOM_NATIVE  0xFEFF
#define UNICODE_BOM_SWAPPED 0xFFFE

#endif

CScreen* CScreen::screen = NULL;

//==============================================================================
//	Class CBaseGui 图形界面接口基类
//==============================================================================
CBaseGui::CBaseGui()
{
//	printf("CBaseGui::CBaseGui()\n");
	ftime(&opttime);

	volrect.left   = SOUND_WINDOW_LEFT;
	volrect.top    = SOUND_WINDOW_TOP;
	volrect.right  = volrect.left + SOUND_WINDOW_WIDTH;
	volrect.bottom = volrect.top  + SOUND_WINDOW_HEIGHT;

	pthread_mutex_init(&CS, NULL);
}

void CBaseGui::GetXyRect(RECT rect, TAlign align, char *text, int len, int *x, int *y)
{
	int cx = 0, cy = 0;
	switch (align.valign)
	{
		case taBottom:
		case taCenter:
			UnTextExtent(text, len, &cx, &cy, 0);
			if (align.valign == taCenter)
				*y = (rect.bottom + rect.top - cy)/2;
			else
				*y = (rect.bottom  - cy)+1;
			break;
		case taTop:
		default:
			*y = rect.top+1;
	}

	switch (align.align)
	{
		case taRightJustify:
		case taCenter:
			if ((cx == 0) || (cy==0))
				UnTextExtent(text, len, &cx, &cy, 0);
			if (align.align == taCenter)
				*x = (rect.right + rect.left - cx)/2 + 1;
			else
				*x = (rect.right - cx) - 1;
			break;
		case taLeftJustify:
		default:
			*x = rect.left+1;
	}
}

void CBaseGui::DrawTextOpt(CKtvOption *opt, CKtvFont *font, bool update)
{
	if (opt == NULL) return;
	pthread_mutex_lock(&CS);
	if (font)
		SetFont(font);
	else
		SetFont(opt->font);
	DrawText(opt->title.data(), opt->rect, opt->align);
	if (update) Flip(&opt->rect);
	pthread_mutex_unlock(&CS);
}

//==============================================================================
//	Class CDirectFBGui DirectFB 图形界面接口
//==============================================================================
#ifdef DIRECTFB

#include "directfb.h"
#include "directfb_keyboard.h"

static __inline__ uint16_t Swap16(uint16_t x)
{
#if defined(mipsel) || defined(arm)
	return (x >> 8) + (x<<8);
#else
	__asm__("xchgb %b0,%h0" : "=q" (x) :  "0" (x));
	return x;
#endif
}

#define DFBCHECK(x...)                                                \
{                                                                     \
	DFBResult err;                                                \
	err = x;                                                      \
	if (err != DFB_OK) {                                          \
		fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );\
		DirectFBErrorFatal( #x, err );                        \
	}                                                             \
}

bool ConRectToRgn(DFBRectangle *rect, DFBRegion *region)
{
	if (rect == NULL || region == NULL)
		return false;
	region->x1 = rect->x;
	region->y1 = rect->y;
	region->x2 = rect->x + rect->w;
	region->y2 = rect->y + rect->h;
	return true;
}

bool ConRgnToRect(DFBRegion *region, DFBRectangle *rect)
{
	if (rect == NULL || region == NULL)
		return false;
	rect->x = region->x1;
	rect->y = region->y1;
	rect->w = region->x2 - region->x1;
	rect->h = region->y2 - region->y1;
	return true;
}

CDirectFBGui::CDirectFBGui():CBaseGui(), MtvDFB(NULL), MtvWindow(NULL), MtvSurface(NULL), MtvBakSurface(NULL), \
	MtvLayer(NULL), MtvEventBuffer(NULL), tmpFace(NULL)
{
	tmpFace_dsc.width = tmpFace_dsc.height = 0;
}

bool CDirectFBGui::GraphicInit(int argc, char **argv)
{
	DFBCHECK(DirectFBInit(&argc, &argv));

	DFBCHECK(DirectFBCreate(&MtvDFB));
	DFBWindowDescription win_dsc;
	DFBDisplayLayerConfig layer_config;
	// Create main surface
	surface_dsc.flags  = (DFBSurfaceDescriptionFlags) (DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS);
	surface_dsc.width  = SCREEN_WIDTH;
	surface_dsc.height = SCREEN_HEIGHT;
	surface_dsc.caps   = DSCAPS_SYSTEMONLY;
//	surface_dsc.caps   = DSCAPS_VIDEOONLY;
//	surface_dsc.caps   = DSCAPS_DOUBLE;

	DFBCHECK( MtvDFB->GetDisplayLayer(MtvDFB, DLID_PRIMARY, &MtvLayer) );

	layer_config.flags      = DLCONF_BUFFERMODE;
//	layer_config.buffermode = DLBM_BACKSYSTEM;
	layer_config.buffermode = DLBM_BACKVIDEO;
	MtvLayer->SetConfiguration(MtvLayer, &layer_config);

	win_dsc.flags = (DFBWindowDescriptionFlags) (DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT);
	win_dsc.posx  = 0;
	win_dsc.posy  = 0;
	win_dsc.width = SCREEN_WIDTH;
	win_dsc.height= SCREEN_HEIGHT;
	win_dsc.caps  = DWCAPS_ALPHACHANNEL;
//	win_dsc.caps  = DWCAPS_DOUBLEBUFFER;

	MtvLayer->CreateWindow(MtvLayer, &win_dsc, &MtvWindow);
	MtvWindow->CreateEventBuffer(MtvWindow, &MtvEventBuffer);
	MtvWindow->SetOptions(MtvWindow, (DFBWindowOptions) (DWOP_ALPHACHANNEL | DWOP_OPAQUE_REGION));
	MtvWindow->SetOpacity(MtvWindow, 0xff);
	MtvWindow->PutAtop(MtvWindow, MtvWindow);
	MtvWindow->RequestFocus(MtvWindow);
	MtvWindow->GetSurface(MtvWindow, &MtvSurface);
	return true;
}

void CDirectFBGui::GuiEnd(void)
{
	if (MtvFont      != NULL) MtvFont->Release   (MtvFont   );
	if (MtvWindow    != NULL) MtvWindow->Release (MtvWindow );
	if (MtvSurface   != NULL) MtvSurface->Release(MtvSurface);
	if (MtvBakSurface!= NULL) MtvBakSurface->Release(MtvSurface);
	if (MtvLayer     != NULL) MtvLayer->Release  (MtvLayer  );
	if (MtvDFB       != NULL) MtvDFB->Release    (MtvDFB    );
}

void CDirectFBGui::DrawFillRect(RECT rect, TColor color)
{
	MtvSurface->SetDrawingFlags(MtvSurface, DSDRAW_NOFX);
	MtvSurface->SetColor(MtvSurface, color.r, color.g, color.b, color.a);
	MtvSurface->FillRectangle(MtvSurface, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top);
	color = argb2color(255, 0, 255, 0);
	DrawRectangle(rect, color);
}

bool CDirectFBGui::UnTextExtent(char *text,int len, int *w, int *h, int extwidth)
{
	const uint16_t *text16 = (uint16_t*)text;
	const uint16_t *ch = text16;
	int swapped = 0;

	*w = *h = 0;
	DFBRectangle GlyphRect;
	for (; *ch; ++ch )
	{
		uint16_t c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text16 == ch ) {
				++text;
			}
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text16 == ch ) {
				++text;
			}
			continue;
		}
		if ( swapped ) {
			c = Swap16(c);
		}
		uint16_t OldIndex = c;
		if ((OldIndex >= 'A' && OldIndex <= 'Z') ||
			(OldIndex >= 'a' && OldIndex <= 'z') ||
			OldIndex == 0x20 ||
			(OldIndex >= '0' && OldIndex <= '9') || OldIndex == 0x20)
			OldIndex = 'O';
		else if(OldIndex & 0x8080)
			OldIndex = 0x9896;

		MtvFont->GetGlyphExtents(MtvFont, OldIndex, &GlyphRect, NULL);
		*w += GlyphRect.w + extwidth;
		*h  = (*h < GlyphRect.h ? GlyphRect.h : *h);
	}
	return true;
}

void* CDirectFBGui::CreateBackground(void *data, size_t length, RECT ct)  /* 建立背景图 */
{
	IDirectFBSurface* tmpsurface = (IDirectFBSurface*)CreateImageSurface(data, length, ct, NULL);
	return tmpsurface;
}

void CDirectFBGui::FreeBackground(void *background)
{
	if (background) {
		IDirectFBSurface* tmpsurface = (IDirectFBSurface*)background;
		tmpsurface->Release(tmpsurface);
	}
}

void CDirectFBGui::SetShadow(void *EmptyShadow)
{
	MtvBakSurface = (IDirectFBSurface*)EmptyShadow;
}

void CDirectFBGui::UpdateShadow(RECT *rect)
{
	int x=0, y= 0;
	if (rect) {
		x = rect->left;
		y = rect->top;
	}

	if (MtvBakSurface)
		MtvSurface->Blit(MtvSurface, MtvBakSurface, NULL, x, y);
}

void CDirectFBGui::Flip(RECT* rect)
{
	MtvSurface->Flip(MtvSurface, NULL, DSFLIP_ONSYNC);
}

void *CDirectFBGui::CreateImageSurface(void *data, size_t length, RECT ct, void *oldface)
{
	IDirectFBSurface        *surface     = NULL;
	IDirectFBDataBuffer     *data_buffer = NULL;
	IDirectFBImageProvider  *image	     = NULL;
	DFBDataBufferDescription data_dsc;
	data_dsc.flags                       = DBDESC_MEMORY;
	data_dsc.memory.data                 = data;
	data_dsc.memory.length               = length;
	DFBRectangle src;
	DFBRegion reg;

	src.x = ct.left;
	src.y = ct.top;
	src.w = ct.right - ct.left;
	src.h = ct.bottom - ct.top;
	reg.x1 = ct.left;
	reg.y1 = ct.top;
	reg.x2 = ct.right;
	reg.y2 = ct.bottom;

	surface_dsc.width = src.w;
	surface_dsc.height= src.h;
	if (oldface)
		surface=(IDirectFBSurface*)oldface;
	else
		MtvDFB->CreateSurface(MtvDFB, &surface_dsc, &surface);

	/**************************************************************************/
	MtvDFB->CreateDataBuffer(MtvDFB, &data_dsc, &data_buffer);
	if (data_buffer) {
		data_buffer->CreateImageProvider(data_buffer, &image);
		data_buffer->Release(data_buffer);
		if (image) {
			image->RenderTo(image, surface, NULL);
			image->Release(image);
			return surface;
		}
	}
	return NULL;
	/**************************************************************************/
}

static int DirectFB_Numpadascii(int key)
{
	switch (key) {
		case DIKS_PAGE_UP  : return 280;
		case DIKS_PAGE_DOWN: return 281;
	}
	return key;
}

bool CDirectFBGui::WaitInputEvent(InputEvent *event, int timeout)
{
	MtvEventBuffer->WaitForEventWithTimeout(MtvEventBuffer, 0, timeout);
	if ( MtvEventBuffer->GetEvent(MtvEventBuffer, DFB_EVENT(&MtvWindowEvent)) == DFB_OK ){
//		DEBUG_OUT("%d\n", MtvWindowEvent.key_symbol);
		if (MtvWindowEvent.key_symbol==']') DEBUG_OUT("\n");
		fflush(stdout);
		if (MtvWindowEvent.type == DWET_KEYDOWN){
			event->type = IT_KEY_DOWN;
			event->k    = DirectFB_Numpadascii(MtvWindowEvent.key_symbol);
			return true;
		}
		if (MtvWindowEvent.type == DWET_KEYUP){
			event->type = IT_KEY_UP;
			event->k    = DirectFB_Numpadascii(MtvWindowEvent.key_symbol);
			return true;
		}
		else if(MtvWindowEvent.type == DWET_BUTTONDOWN){
			if (MtvWindowEvent.button == DIBI_RIGHT)
				event->type = IT_MOUSERIGHT_DOWN;
			else if(MtvWindowEvent.button == DIBI_LEFT)
				event->type = IT_MOUSELEFT_DOWN;
			event->x = MtvWindowEvent.cx;
			event->y = MtvWindowEvent.cy;
			return true;
		}
		else if(MtvWindowEvent.type == DWET_BUTTONUP){
			if (MtvWindowEvent.button == DIBI_RIGHT)
				event->type = IT_MOUSERIGHT_UP;
			else if(MtvWindowEvent.button == DIBI_LEFT)
				event->type = IT_MOUSELEFT_UP;
			event->x = MtvWindowEvent.cx;
			event->y = MtvWindowEvent.cy;
			return true;
		}
		else if (MtvWindowEvent.type == DWET_MOTION) {
			event->type = IT_MOUSEMOVE;
			event->x    = MtvWindowEvent.cx;
			event->y    = MtvWindowEvent.cy;
			return true;
		}
	}
	return false;
}

void CDirectFBGui::GuiRest()
{
	MtvEventBuffer->Reset(MtvEventBuffer);
}

void CDirectFBGui::SetFont(CKtvFont *font)
{
	if (!font->GetHandle()) {
		IDirectFBFont *tmpFont = NULL;
		DFBFontDescription MtvFontDesc;
		MtvFontDesc.flags = (DFBFontDescriptionFlags)(DFDESC_WIDTH | DFDESC_HEIGHT);
		MtvFontDesc.width = (font->size() > 0 ? ConvertFSizeToPixer(font->size()) : FONT_SIZE);
		MtvFontDesc.height= (font->size() > 0 ? ConvertFSizeToPixer(font->size()) : FONT_SIZE);
		if (MtvDFB->CreateFont(MtvDFB, font->GetFontFile(), &MtvFontDesc, &tmpFont)!= DFB_OK) {
			DEBUG_OUT("failed opening %s\n", font->GetFontFile());
			return;
		}
		font->SetHandle(tmpFont);
	}

	if (CurFont != font)
	{
		MtvFont = (IDirectFBFont *)(font->GetHandle() );
		if (MtvFont != NULL)
			DFBCHECK(MtvSurface->SetFont(MtvSurface, MtvFont));
		CurFont = font;
	}
	DFBCHECK(MtvSurface->SetColor(MtvSurface,font->color.r,font->color.g,font->color.b,font->color.a));
}


void CDirectFBGui::DrawText(const char *value, RECT rect, TAlign align, bool unicode)
{
	if (strlen(value) == 0) return;
	char cBuf[BUF_LEN];
	memset(cBuf, '\0', BUF_LEN);
	size_t OutByteSize = BUF_LEN;
	int CurX;
	int CurY;
	if (unicode)
		OutByteSize = Unicode(CurFont->charset(), value, cBuf, BUF_LEN);
	else{
	}
	if (OutByteSize <= 0) return;

	int x=0,y=0;
	GetXyRect(rect, align, cBuf, OutByteSize, &x, &y);
	DFBRectangle GlyphRect;
	MtvSurface->SetColor(MtvSurface, 0x00, 0x00, 0x00, 0x00);

	CurX = x;
	CurY = y;
	const uint16_t *text = (uint16_t*)cBuf;
	const uint16_t *ch = text;
	int swapped = 0;
	for (; *ch; ++ch )
	{
		uint16_t c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch )
				++text;
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch )
				++text;
			continue;
		}
		if ( swapped )
			c = Swap16(c);
		MtvSurface->DrawGlyph(MtvSurface, c, CurX-1, CurY-1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX+1, CurY+1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX-1, CurY+1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX+1, CurY-1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX-1, CurY  , (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX+1, CurY  , (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX  , CurY+1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		MtvSurface->DrawGlyph(MtvSurface, c, CurX  , CurY-1, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));

		uint16_t OldIndex = c;
		if ((OldIndex >= 'A' && OldIndex <= 'Z') ||
			(OldIndex >= 'a' && OldIndex <= 'z') ||
			 OldIndex == 0x20 ||
			(OldIndex >= '0' && OldIndex <= '9') || OldIndex == 0x20)
		{
			OldIndex = 'O';
		}
		else if(OldIndex & 0x8080)
			OldIndex = 0x9896;
		MtvFont->GetGlyphExtents(MtvFont, OldIndex, &GlyphRect, NULL);
		CurX += GlyphRect.w + 2;
	}
	DFBCHECK(MtvSurface->SetColor(MtvSurface, CurFont->color.r, CurFont->color.g, CurFont->color.b, CurFont->color.a));

	CurX = x;
	CurY = y;

	for(ch = text; *ch; ++ch ) {
		uint16_t c = *ch;
		if ( c == UNICODE_BOM_NATIVE ) {
			swapped = 0;
			if ( text == ch )
				++text;
			continue;
		}
		if ( c == UNICODE_BOM_SWAPPED ) {
			swapped = 1;
			if ( text == ch )
				++text;
			continue;
		}
		if ( swapped )
			c = Swap16(c);
		MtvSurface->DrawGlyph(MtvSurface, c, CurX, CurY, (DFBSurfaceTextFlags)(DSTF_TOPLEFT));
		uint16_t OldIndex = c;
		if ((OldIndex >= 'A' && OldIndex <= 'Z') ||
			(OldIndex >= 'a' && OldIndex <= 'z') ||
			OldIndex == 0x20 ||
			(OldIndex >= '0' && OldIndex <= '9') || OldIndex == 0x20)
		{
			OldIndex = 'O';
		}
		else if(OldIndex & 0x8080)
			OldIndex = 0x9896;
		MtvFont->GetGlyphExtents(MtvFont, OldIndex, &GlyphRect, NULL);
		CurX += GlyphRect.w + 2;
	}
}

void CDirectFBGui::DrawRectangle(RECT rect,TColor color) /* 画框，color框的颜色 */
{
	int w = rect.right - rect.left;
	int h = rect.bottom- rect.top;
	MtvSurface->SetColor(MtvSurface, color.r,color.g,color.b,color.a);
	MtvSurface->DrawRectangle(MtvSurface, rect.left, rect.top, w, h);
	MtvSurface->DrawRectangle(MtvSurface, rect.left+1, rect.top+1, w-2, h-2);
}

void CDirectFBGui::DrawLine(int x1, int y1, int x2, int y2)
{
	MtvSurface->DrawLine(MtvSurface, x1, y1, x2, y2);
}

void CDirectFBGui::DrawImage(void *data, size_t length, RECT ct)
{
	if (length <= 0) return;
	int w = ct.right - ct.left, h=ct.bottom - ct.top;
	if ((tmpFace_dsc.width != w) || (tmpFace_dsc.height != h)){
		if (tmpFace != NULL) {
			tmpFace->Release(tmpFace);
			tmpFace = NULL;
		}
		tmpFace_dsc.width = w;
		tmpFace_dsc.height= h;
	}
	tmpFace = (IDirectFBSurface*)CreateImageSurface(data, length, ct, tmpFace);
	if (tmpFace)
		MtvSurface->Blit(MtvSurface,tmpFace, NULL, ct.left, ct.top);
}

void CDirectFBGui::DrawSoundBar(int volume)
{
	if (volume < 0) return;
	int sprite = 0;
	int i;
	int r,g,b;
	if (currect) {
		DFBRectangle source_rect;
		source_rect.x = currect->left - BakFace_rect.left;
		source_rect.y = currect->top - BakFace_rect.top;
		source_rect.w = currect->right - currect->left;
		source_rect.h = currect->bottom - currect->top;
		MtvSurface->Blit(MtvSurface, MtvBakSurface, &source_rect, currect->left, currect->top);
	} else
		MtvSurface->Blit(MtvSurface, MtvBakSurface, NULL, 0, 0);


	currect = &volrect;
	MtvSurface->SetDrawingFlags(MtvSurface, DSDRAW_NOFX);
	r=0; g=255; b=0;
	for (i=0; i<20; i++)
	{
		if (i<10) r =255/10*i; else g=255/10*(20-i);
		MtvSurface->SetColor(MtvSurface,r,g,b,0x00);
		if (i == 0)
			sprite = 0;
		else
			sprite = i * SOUND_SPRITE;
		if (i < volume / 5)
			MtvSurface->FillRectangle(MtvSurface,
					SOUND_WINDOW_LEFT + sprite + i *SOUND_RECT_WIDTH,
					SOUND_WINDOW_TOP,
					SOUND_RECT_WIDTH,
					SOUND_RECT_HEIGHT);
		else
			MtvSurface->DrawRectangle(MtvSurface,
					SOUND_WINDOW_LEFT + sprite + i *SOUND_RECT_WIDTH,
					SOUND_WINDOW_TOP,
					SOUND_RECT_WIDTH,
					SOUND_RECT_HEIGHT);
	}
	Flip();
}
#endif

#ifdef SDLGUI

//#include "SDL.h"
//#include "SDL_image.h"
//#include "SDL_drawlibs.h"
//#include "SDL_ttf.h"

#define DESIRED_BPP 16
//#define VIDEO_FLAGS SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ANYFORMAT
#define VIDEO_FLAGS SDL_HWSURFACE | SDL_ANYFORMAT

#define RECT2SDLRECT(r1, r2) {r1.x=r2.left;r1.y=r2.top;r1.w=r2.right-r2.left;r1.h=r2.bottom-r2.top;}

/* Create a "light" -- a yellowish surface with variable alpha */
#ifdef ENABLE_HANDWRITE
SDL_Surface *CreateLight(Uint32 flags, int radius)
{
	Uint8  trans;
	int    range, addition;
	int    xdist, ydist;
	Uint16 x, y;
	Uint16 skip;
	Uint32 pixel;
	SDL_Surface *light;

	Uint16 *buf;

	Uint32 amask = 0x000000FF;


	/* Create a 16 (4/4/4/4) bpp square with a full 4-bit alpha channel */
	/* Note: this isn't any faster than a 32 bit alpha surface */
	light = CreateSurface(flags, 2*radius, 2*radius, DESIRED_BPP);
	/* Fill with a light yellow-orange color */
	skip = light->pitch-(light->w*light->format->BytesPerPixel);
	buf = (Uint16 *)light->pixels;
	/* Get a tranparent pixel value - we'll add alpha later */
	pixel = PEN_COLOR;
	for ( y=0; y<light->h; ++y ) {
		for ( x=0; x<light->w; ++x ) {
			*buf++ = pixel;
		}
		buf += skip;	/* Almost always 0, but just in case... */
	}
	return light;
	SDL_SaveBMP(light, "/b.bmp");
	/* Calculate alpha values for the surface. */
	buf = (Uint16 *)light->pixels;
	for ( y=0; y<light->h; ++y ) {
		for ( x=0; x<light->w; ++x ) {
			/* Slow distance formula (from center of light) */
			xdist = x-(light->w/2);
			ydist = y-(light->h/2);
			range = (int)sqrt(xdist * xdist + ydist * ydist);

			/* Scale distance to range of transparency (0-255) */
			if ( range > radius ) {
				trans = amask;
			} else {
				/* Increasing transparency with distance */
				trans = (Uint8)((range * amask)/radius);

				/* Lights are very transparent */
				addition = (amask+1)/8;
				if ( (int)trans + addition > (int)amask ) {
					trans = amask;
				} else {
					trans += addition;
				}
			}
			/* We set the alpha component as the right N bits */
			*buf++ |= (255-trans);
		}
		buf += skip;	/* Almost always 0, but just in case... */
	}
	/* Enable RLE acceleration of this alpha surface */
	SDL_SetAlpha(light, SDL_SRCALPHA|SDL_RLEACCEL, 0);
	SDL_SaveBMP(light, "/a.bmp");
	/* We're done! */
	return(light);
}
#endif

CSDLGui::CSDLGui():CBaseGui(),screen(NULL), shadow(NULL), background(NULL)
#ifdef NEWMOUSE
	, mouse_cursor(NULL)
#endif
#ifdef ENABLE_HANDWRITE
	, handwrite(NULL)
#endif
{
	BG_Rect.left   = 0;
	BG_Rect.top    = 0;
	BG_Rect.right  = SCREEN_WIDTH;
	BG_Rect.bottom = SCREEN_HEIGHT;
}

CSDLGui::~CSDLGui(void)
{
	GuiEnd();
#ifdef NEWMOUSE
	delete mouse_cursor;
#endif
}

void CSDLGui::GuiRest(){}

bool CSDLGui::GraphicInit(int argc, char **argv)
{
	/* Initialize SDL */
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return false;
	}
	atexit(SDL_Quit);
	Uint32 video_flags = FastestFlags(VIDEO_FLAGS, SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP, video_flags);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n",
			SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP, SDL_GetError());
		return false;
	}

//	shadow = SDL_CreateRGBSurface(video_flags, SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP, 0, 0, 0, 0);
	shadow = CreateSurface(video_flags, SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP);
	if (shadow== NULL) {
		fprintf(stderr, "Couldn't set %dx%dx%d video mode, %s\n",
			SCREEN_WIDTH, SCREEN_HEIGHT, DESIRED_BPP, SDL_GetError());
		return false;
	}
#ifdef ENABLE_HANDWRITE
	handwrite = CreateLight(video_flags, PEN_WIDTH);
#endif

	/* Initialize the TTF library */
	if ( TTF_Init() < 0 ) {
		fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
		return(2);
	}
	atexit(TTF_Quit);

#ifdef NEWMOUSE
#define MOUSEFILE "/usr/local/share/mousecursor.png"
	mouse_cursor = new MouseCursor(&shadow, screen, MOUSEFILE, 1);
	if (!mouse_cursor)
		printf("No found "MOUSEFILE"\n");
#endif
	return true;
}

void CSDLGui::GuiEnd(void)
{
	if (screen){
		SDL_FreeSurface(screen);
		screen = NULL;
	}
	if (background) {
		SDL_FreeSurface(background);
		screen = NULL;
	}
	if (shadow) {
		SDL_FreeSurface(shadow);
		shadow = NULL;
	}
#ifdef ENABLE_HANDWRITE
	if (handwrite) {
		SDL_FreeSurface(handwrite);
		shadow = NULL;
	}
#endif
	SDL_Quit();
}

SDL_Surface *CSDLGui::CreateImageSurface(void *data, size_t length, RECT ct)
{
	if (length <= 0) return NULL;

	SDL_RWops *ctx = SDL_RWFromMem(data, length);
	SDL_Surface *image = IMG_Load_RW(ctx, false);

	if ( image == NULL ) {
		fprintf(stderr, "Couldn't load Image Data: %s\n", SDL_GetError());
		return NULL;
	}
	SDL_Rect dest;
	RECT2SDLRECT(dest,ct)
	if ( (image->w != dest.w) || (image->h != dest.h) )
		SDL_SoftStretch(image, NULL, image, &dest);
	return image;
}

unsigned char CSDLGui::GetMouseState(int *x, int *y)
{
	return SDL_GetMouseState(x, y);
}

void* CSDLGui::CreateBackground(void *data, size_t length, RECT ct)
{
	return CreateImageSurface(data, length, ct);
}

void CSDLGui::FreeBackground(void *background)
{
	SDL_FreeSurface((SDL_Surface*)background);
}

void CSDLGui::DrawImage(void *data, size_t length, RECT ct)
{
	SDL_Surface *image = CreateImageSurface(data, length, ct);
	if (image) {
		if ( image->format->palette )
			SDL_SetColors(shadow, image->format->palette->colors, 0, image->format->palette->ncolors);
		SDL_Rect rect,srcrect;
		RECT2SDLRECT(rect, ct)
		srcrect.x = srcrect.y = 0;
		srcrect.w = rect.w;
		srcrect.h = rect.h;
		SDL_BlitSurface(image, &srcrect, shadow, &rect);
		SDL_FreeSurface(image);
	}
}

static int SDL_WaitEventTimeout(SDL_Event *event, Uint32 timeout)
{
	Uint32 t1 = SDL_GetTicks();

	while ( 1 ) {
		SDL_PumpEvents();
		switch(SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) {
			case -1: return 0;
			case 1 : return 1;
			case 0 : SDL_Delay(10);
		}
		if (SDL_GetTicks() - t1 > timeout)
			return 0;
	}
}

static int SDL_Numpadascii(int key)
{
	switch (key) {
		case SDLK_KP0         : return SDLK_0;
		case SDLK_KP1         : return SDLK_1;
		case SDLK_KP2         : return SDLK_2;
		case SDLK_KP3         : return SDLK_3;
		case SDLK_KP4         : return SDLK_4;
		case SDLK_KP5         : return SDLK_5;
		case SDLK_KP6         : return SDLK_6;
		case SDLK_KP7         : return SDLK_7;
		case SDLK_KP8         : return SDLK_8;
		case SDLK_KP9         : return SDLK_9;
		case SDLK_KP_PERIOD   : return SDLK_PERIOD;
		case SDLK_KP_DIVIDE   : return SDLK_SLASH;
		case SDLK_KP_MULTIPLY : return SDLK_ASTERISK;
		case SDLK_KP_MINUS    : return SDLK_MINUS;
		case SDLK_KP_PLUS     : return SDLK_PLUS;
		case SDLK_KP_ENTER    : return SDLK_RETURN;
		case SDLK_KP_EQUALS   : return SDLK_EQUALS;
	}
	return key;
}

bool CSDLGui::WaitInputEvent(InputEvent *event, int timeout)
{
	SDL_Event sdlevnet;
	if (SDL_WaitEventTimeout(&sdlevnet, timeout)) {
#if NEWMOUSE
		if ( (sdlevnet.type == SDL_MOUSEBUTTONDOWN) ||
		     (sdlevnet.type == SDL_MOUSEBUTTONUP ) ||
		     (sdlevnet.type == SDL_MOUSEMOTION) )
			if(mouse_cursor) mouse_cursor->draw();
#endif
		switch (sdlevnet.type) {
			case SDL_MOUSEBUTTONDOWN:
				if (sdlevnet.button.button == SDL_BUTTON_LEFT)
					event->type = IT_MOUSELEFT_DOWN;
				else if(sdlevnet.button.button == SDL_BUTTON_RIGHT)
					event->type = IT_MOUSERIGHT_DOWN;
				event->x = sdlevnet.button.x;
				event->y = sdlevnet.button.y;
				return true;
			case SDL_MOUSEBUTTONUP:
				if (sdlevnet.button.button == SDL_BUTTON_LEFT)
					event->type = IT_MOUSELEFT_UP;
				else if(sdlevnet.button.button == SDL_BUTTON_RIGHT)
					event->type = IT_MOUSERIGHT_UP;
				event->x = sdlevnet.button.x;
				event->y = sdlevnet.button.y;
				return true;
			case SDL_MOUSEMOTION:
				event->type = IT_MOUSEMOVE;
				event->x    = sdlevnet.button.x;
				event->y    = sdlevnet.button.y;
				return true;
			case SDL_KEYDOWN:
				event->type = IT_KEY_DOWN;
				event->k = sdlevnet.key.keysym.sym;
				if (event->k >=SDLK_KP0 && event->k <= SDLK_KP_EQUALS)
					event->k = SDL_Numpadascii(event->k);
				return true;
			case SDL_KEYUP:
				event->type = IT_KEY_UP;
				event->k = sdlevnet.key.keysym.sym;
				return true;
			case SDL_QUIT:
				return true;
			default:
				break;
		}
	}
	return false;
}

void CSDLGui::DrawSoundBar(int volume)                            // 显示音量
{
	if (volume < 0) return;
	currect = &volrect;
	SDL_Rect rect;
	SDL_Color c1 = {0, 255,0 }, c2 = {255,0,0};

	rect.x = SOUND_WINDOW_LEFT;
	rect.y = SOUND_WINDOW_TOP;
	rect.w = SOUND_WINDOW_WIDTH;
	rect.h = SOUND_WINDOW_HEIGHT;
	SDL_horizgradient2(shadow, rect, c1, c2, 200);

	rect.x = SOUND_WINDOW_LEFT  + (volume / 5) * SOUND_RECT_WIDTH;
	rect.w = SOUND_WINDOW_WIDTH - (volume / 5) * SOUND_RECT_WIDTH;
	SDL_FillRect(shadow, &rect, 0xB4B4B4);
	SDL_DrawRound(shadow, SOUND_WINDOW_LEFT, SOUND_WINDOW_TOP, SOUND_WINDOW_WIDTH, SOUND_WINDOW_HEIGHT, 0, 0XFF0000);
	Flip();
}

void CSDLGui::DrawRectangle(RECT rect,TColor color)              // 画框，color框的颜色
{
	SDL_DrawRect(shadow, rect.left, rect.top, rect.right,rect.bottom, COLOR2UINT32(color));
}

void CSDLGui::DrawFillRect(RECT rect, TColor color)
{
	SDL_Rect r = {rect.left, rect.top, rect.right-rect.left-1, rect.bottom-rect.top-1};

	SDL_FillRect(shadow, &r, 0xB4B4B4);
	SDL_DrawRect(shadow, rect.left, rect.top, rect.right-1, rect.bottom-1, rgb2uint32(0xFF, 0, 0));
}

void CSDLGui::DrawLine(int x1, int y1, int x2, int y2)
{
	SDL_DrawLine(shadow, x1, y1, x2, y2, 0);
}

bool CSDLGui::UnTextExtent(char *text,int len, int *w, int *h, int extwidth)  // 返回Unicode字符串的长度
{
	TTF_SizeUNICODE((TTF_Font*)CurFont->GetHandle(), (const Uint16 *)text, w, h);
	*w += extwidth;
	return true;
}

#define RENDER_STYLE TTF_STYLE_NORMAL
#define RENDER_TYPE  RENDER_LATIN1
void CSDLGui::SetFont(CKtvFont *font)                                // 设置新的字体
{
	CurFont = font;
	if (!font->GetHandle()) {
		int size = (font->size()>0 ? ConvertFSizeToPixer(font->size()) : FONT_SIZE);
		font->SetHandle( (void *)TTF_OpenFont(font->GetFontFile(), size) );
		if (!font->GetHandle() )
			printf("TTF_OpenFont error.\n");
		TTF_SetFontStyle((TTF_Font*)font->GetHandle(), RENDER_STYLE);
	}
}

void CSDLGui::DrawText(const char *value, RECT rect,TAlign align, bool unicode)          // 显字字符串
{
	DrawTextSurface(shadow, value, rect, align, unicode);
}

void CSDLGui::DrawTextSurface(SDL_Surface *surface, const char *value, RECT rect, TAlign align, bool unicode)
{
	if (strlen(value) == 0) return;

	char cBuf[BUF_LEN];
	memset(cBuf, '\0', BUF_LEN);

	size_t OutByteSize = BUF_LEN;
	if (unicode)
		OutByteSize = Unicode(CurFont->charset(), value, cBuf, BUF_LEN);
	else{
	}

	if (OutByteSize <= 0) return;
	SDL_Color black = { 0x00, 0x00, 0x00, 0 };
	SDL_Color fontcolor = {CurFont->color.r, CurFont->color.g, CurFont->color.b};

	int x=0,y=0;
	GetXyRect(rect, align, cBuf, OutByteSize, &x, &y);
	SDL_Surface *text = TTF_RenderUNICODE_Blended((TTF_Font*)CurFont->GetHandle(), (Uint16 *)cBuf, black);
	if ( text != NULL ) {
		SDL_Rect dstrect, tmp;
		dstrect.x = x;
		dstrect.y = y;
		dstrect.w = text->w;
		dstrect.h = text->h;
		tmp = dstrect;
		tmp.x=dstrect.x-1; tmp.y=dstrect.y-1;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x+1; tmp.y=dstrect.y+1;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x-1; tmp.y=dstrect.y+1;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x+1; tmp.y=dstrect.y-1;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x-1; tmp.y=dstrect.y;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x+1; tmp.y=dstrect.y;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x; tmp.y=dstrect.y+1;
		SDL_BlitSurface(text, NULL, surface, &tmp);
		tmp.x=dstrect.x; tmp.y=dstrect.y-1;
		SDL_BlitSurface(text, NULL, surface, &tmp);

		SDL_FreeSurface(text);
		text = TTF_RenderUNICODE_Blended((TTF_Font*)CurFont->GetHandle(), (Uint16 *)cBuf, fontcolor);
		if (text != NULL){
			SDL_BlitSurface(text, NULL, surface, &dstrect);
			SDL_FreeSurface(text);
		}
	}
}

void CSDLGui::SetShadow(void *EmptyShadow)
{
	background = (SDL_Surface*)EmptyShadow;
}

void CSDLGui::UpdateShadow(RECT *rect)
{
	if (background) {
		if ( background->format->palette ) {
			SDL_SetColors(shadow, background->format->palette->colors, 0, background->format->palette->ncolors);
		}
		SDL_Rect destrect,srcrect;
		if (rect){
			RECT m = *rect;
			RECT2SDLRECT(destrect, m);

		}
		else
			RECT2SDLRECT(destrect, BG_Rect);
		srcrect = destrect;
		SDL_BlitSurface(background, &srcrect, shadow, &destrect);
	}
}

void CSDLGui::Flip(RECT* rect)  // 将缓冲区的显示提交到显示
{
	if (rect) {
		SDL_Rect r;
		RECT r1 = *rect;
		RECT2SDLRECT(r, r1);
		SDL_BlitSurface(shadow, &r, screen, &r);
		SDL_UpdateRect(screen, r.x, r.y, r.w, r.h);
	}
	else {
		SDL_BlitSurface(shadow, NULL, screen, NULL);
		SDL_Flip(screen);
	}
#ifdef NEWMOUSE
	if(mouse_cursor) mouse_cursor->draw();
#endif
}

void CSDLGui::RestoreFade(RECT rect)
{
#if 0
	SDL_Surface *tmp = CreateSurface(VIDEO_FLAGS, rect.right-rect.left, rect.bottom-rect.top, DESIRED_BPP);
	SDL_Rect dstrect, srcrect;
	RECT2SDLRECT(srcrect, rect);
	dstrect.x = dstrect.y = 0;
	dstrect.w = srcrect.w;
	dstrect.h = dstrect.h;
	SDL_BlitSurface(shadow, &srcrect, tmp, &dstrect);
	SDL_Fade(tmp, screen, 200, true);
	SDL_FreeSurface(tmp);
#else
//	SDL_Fade(shadow, screen, 100, true);
#endif
}

void CSDLGui::DrawHandWrite(int x1, int y1, int x2, int y2)
{
#ifdef ENABLE_HANDWRITE
	RECT r;
	SDL_DrawLineHandle(shadow, x1, y1, x2, y2, handwrite);

	r.left = x1;
	r.top  = y1;
	r.right= x2;
	r.bottom =y2;
	SDL_Rect src;
	RECT2SDLRECT(src, r);
	Flip(&r);
#endif
}

#endif

#ifdef SPHEGUI
CSpheGui::CSpheGui ():CBaseGui()
{
	printf("CSpheGui::CSpheGui()\n");
	keyb_fd = open(KEYBOARD, O_NONBLOCK);   /* the keyboard immediatedly */
//	keyb_fd = open(KEYBOARD, O_NONBLOCK);   /* the keyboard immediatedly */
						/* because they use the same port */
	if(keyb_fd < 0){
		printf("Open %s failed!\n",KEYBOARD);
		exit(-1);
	}

	printf(" keyb_fd = %d.\n", keyb_fd);

	/* Save previous settings*/
	if (ioctl(keyb_fd, KDGKBMODE, &old_kbd_mode) < 0) {
		printf("Get keyboard mode failed!\n");
		exit(-1);
	}
	if (tcgetattr(keyb_fd, &old_termios) < 0){
		printf("Get attr failed\n");
		exit(-1);
	}
	new_termios = old_termios;

	new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_termios.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | BRKINT);
	new_termios.c_cc[VMIN] = 0;
	new_termios.c_cc[VTIME] = 0;
	if (tcsetattr(keyb_fd, TCSAFLUSH, &new_termios) < 0) {
		printf("Set attr failed!\n");
		exit(-1);
	}
	if (ioctl(keyb_fd, KDSKBMODE, K_XLATE /* K_MEDIUMRAW K_RAW*/) < 0) {
		printf("Set Keyboard Mode Failed!\n");
		exit(-1);
	}
}

CSpheGui::~CSpheGui(void)
{
}

void CSpheGui::GuiRest()
{
}

bool CSpheGui::GraphicInit(int argc, char **argv)
{
	printf("CSpheGui::GraphicInit()\n");
	SP_OpenDisplay();

	SP_GetScreenParam(&Width, &Height);
//	SP_SetViewOrg(40,10);
//	Width = 720 - 42*2;
//	Height = 480 - 15*2;
	printf("screen:%d x %d\n",Width, Height);

	SP_CreateGlobalDC();
	hdc = SP_GetGlobalDC();

	return true;
}

void CSpheGui::GuiEnd(void)
{
	SP_CloseDisplay();
}

unsigned char CSpheGui::GetMouseState(int *x, int *y)
{
	return 0;
}

void *CSpheGui::CreateBackground(void *data, size_t length, RECT ct)
{
	return CreateImageSurface(data, length);
}

void CSpheGui::FreeBackground(void *background)
{
}

void CSpheGui::SetShadow(void *EmptyShadow)
{
	printf("%s\n", __FUNCTION__);
	Shadow = (BITMAP*)EmptyShadow;
	printf("................................\n");
}

void CSpheGui::UpdateShadow(RECT *rect)
{
	printf("CSpheGui::UpdateShadow\n");
	int x =0, y=0, w=Width, h=Height;
	if (rect) {
		x = rect->left;
		y = rect->top;
		w = rect->right - rect->left;
		h = rect->bottom - rect->top;
	}
	SP_FillRectWithBitmap (hdc, x, y, w, h, Shadow);
//	ShowSDLSurface(Shadow, *rect);
}

void CSpheGui::DrawImage(void *data, size_t length, RECT ct)
{
#if 0
	SDL_Surface *surface = CreateImageSurface(data, length);
	ShowSDLSurface(surface, ct);
#else
	printf("%s\n", __FUNCTION__);
	BITMAP *surface = CreateImageSurface(data, length);
	SP_FillRectWithBitmap (hdc, ct.left, ct.top, ct.right-ct.left, ct.bottom-ct.top, surface);
//	ShowSDLSurface(surface, ct);
#endif
}

bool CSpheGui::WaitInputEvent(InputEvent *event, int timeout)
{
#define MAX_BUF_SIZE    1
	usleep(timeout * 1000);
	printf("CSpheGui::WaitInputEvent\n");
	unsigned char buffer[MAX_BUF_SIZE];
	int keyb_entry;
	memset(buffer,0,MAX_BUF_SIZE);
	keyb_entry = read(keyb_fd, buffer, MAX_BUF_SIZE );
	if( keyb_entry > 0 )
		printf("[%s]\n",buffer);
	return false;
}

void CSpheGui::DrawSoundBar(int volume)
{
}

void CSpheGui::DrawRectangle(RECT rect, TColor color)
{
}

void CSpheGui::DrawFillRect(RECT rect, TColor color)
{
}

void CSpheGui::DrawLine(int x1, int y1, int x2, int y2)
{
}

bool CSpheGui::UnTextExtent(char *text,int len, int *w, int *h, int extwidth)
{
	return false;
}

void CSpheGui::SetFont(CKtvFont *font)
{
}

void CSpheGui::DrawText(const char *value,RECT rect,TAlign align, bool unicode)
{
}

void CSpheGui::Flip(RECT* rect)
{
}

void CSpheGui::RestoreFade(RECT rect)
{
}

void CSpheGui::DrawHandWrite(int x1, int y1, int x2, int y2)
{
}

#if 0
void CSpheGui::ShowSDLSurface(SDL_Surface *surface, RECT ct)
{
	int i,j;
	uint32_t color;
	printf("ShowSDLSurface\n");
	for (i=0;i<surface->w; i++){
		for (j=0; j<surface->h; j++)
		{
			color = GetSurfacePixel(surface, i, j);
			SP_SetPixel(hdc, i, j, color);
		}
	}
}
#endif

BITMAP *CSpheGui::CreateImageSurface(void *data, size_t length)
{
#if 0
	SDL_RWops *ctx = SDL_RWFromMem(data, length);
	return IMG_Load_RW(ctx, false);
#else
	BITMAP *x =SP_LoadBitmapFromBuffer((char*)data);
	printf("%ldx%ld,%d\n", x->width, x->height, x->bpp);
	return x;
#endif
}

uint32_t GetSurfacePixel(SDL_Surface *surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	switch(bpp) {
		case 1:	return *p;
		case 2:	return *(Uint16 *)p;
		case 3: return SDL_BYTEORDER == SDL_BIG_ENDIAN? p[0]<<16|p[1]<<8|p[2]:p[0]|p[1]<<8|p[2]<<16;
		case 4: return *(uint32_t *)p;
		default: return 0;  /* shouldn't happen, but avoids warnings */
	}
}
#endif

void CScreen::FreeScreen()
{
//	if (screen) delete screen;
}

CScreen *CScreen::CreateScreen()
{
	if (!screen) {
		screen = new CScreen();
		atexit(CScreen::FreeScreen);
	}
	return screen;
}

CScreen::~CScreen()
{
	if (pGui) delete pGui;
}

