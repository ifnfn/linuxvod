/*==============================================================================
 *   T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *  Filename   : gui.h
 *  Author(s)  : Silicon
 *  Copyright  : Silicon
 *
 *  ʵ�ֶ����ݱ�ķ�װ�������б����������б��������Ԫ��ʵ��
 *============================================================================*/
#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/timeb.h>
#ifdef WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif

#ifdef DIRECTFB
	#include "directfb.h"
#endif

#if defined(SDLGUI) || defined(SPHEGUI)
	#include "SDL.h"
	#include "SDL/SDL_ttf.h"
	#include "SDL_image/SDL_image.h"
	#include "SDL/SDL_drawlibs.h"
	#include "SDL/SDL_ttf.h"
	#ifdef NEWMOUSE
		#include "mousecursor.h"
	#endif
#endif

#ifdef SPHEGUI
	#include <termios.h>
	#include <sys/ioctl.h>
	#include <linux/keyboard.h>
	#include <linux/kd.h>
#endif
#include "config.h"
#include "windowmanage.h"
#include "font.h"
#include "keybuf.h"

//==============================================================================
// CBaseGui ��
//  1. �������ʾ�ӿ�
//  2. ��������ӿ�
//  3. ���ڵĹ���
//==============================================================================
class CBaseGui
{
public:
	struct timeb opttime;
	CBaseGui();
	virtual ~CBaseGui(void){}
	virtual void GuiRest(){}

	virtual void DrawTextOpt(CKtvOption *opt, CKtvFont *font=NULL, bool update=false);
	                                           // ���Option������
//==============================================================================
	virtual unsigned char GetMouseState(int *x, int *y) = 0;
	virtual bool GraphicInit(int argc, char **argv) = 0;
	virtual void GuiEnd(void) = 0;
	virtual void *CreateBackground(void *data, size_t length, RECT ct) = 0;
	virtual void FreeBackground(void *background) = 0;
	virtual void SetShadow(void *EmptyShadow) = 0;
	virtual void UpdateShadow(RECT *rect=NULL) = 0;
	virtual void DrawImage(void *data, size_t length, RECT ct) = 0;
	virtual bool WaitInputEvent(InputEvent *event, int timeout) = 0;
	virtual void DrawSoundBar(int volume) = 0;                    // ��ʾ����

	virtual void DrawRectangle(RECT rect,TColor color) = 0;       // ����color�����ɫ
	virtual void DrawFillRect(RECT rect, TColor color) = 0;       // ������, colorΪ���ɫ
	virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;    // ����
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0) = 0;
	                                                              // �����ַ����ĳ���
	virtual void SetFont(CKtvFont *font) = 0;                     // �����µ�����
	virtual void DrawText(const char *value, RECT rect,TAlign align, bool unicode=true) = 0;  // �����ַ���
	virtual void Flip(RECT* rect=NULL) = 0;                       // ������������ʾ�ύ����ʾ
	virtual void RestoreFade(RECT rect) = 0;
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2) = 0;
protected:
	CKtvFont *CurFont;         // ��ǰ������ָ��
	RECT *currect;             // ��ǰ��ʾ��
	RECT volrect;              // ��������ʾ��Χ

	CKtvOption promptopt;      // ���ֲ�����
	pthread_mutex_t CS;        // ͬ������,ʵ�ֽ���ͬ��
	void GetXyRect(RECT rect, TAlign align, char *text, int len, int *x, int *y);
private:
};

//==============================================================================
#ifdef DIRECTFB

class CDirectFBGui: public CBaseGui    // ���� DirectFB ͼ�νӿ�
{
public:
	CDirectFBGui ();
	virtual ~CDirectFBGui(void){}
	virtual void GuiRest();
	virtual unsigned char GetMouseState(int *x, int *y) {return 0;}
	virtual bool GraphicInit(int argc, char **argv);
	virtual void GuiEnd(void);
	virtual void *CreateBackground(void *data, size_t length, RECT ct);
	virtual void FreeBackground(void *background);
	virtual void SetShadow(void *EmptyShadow);
	virtual void UpdateShadow(RECT *rect=NULL);
	virtual void DrawImage(void *data, size_t length, RECT ct);
	virtual bool WaitInputEvent(InputEvent *event, int timeout);

	virtual void DrawSoundBar(int volume);                            // ��ʾ����
	virtual void DrawRectangle(RECT rect,TColor color);               // ����color�����ɫ
	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // ����Unicode�ַ����ĳ���
	virtual void SetFont(CKtvFont *font);                             // �����µ�����
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);          // �����ַ���
	virtual void Flip(RECT* rect=NULL);                               // ������������ʾ�ύ����ʾ
	virtual void RestoreFade(RECT rect) {}
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2){}
private:
	IDirectFB             *MtvDFB         ; //
	IDirectFBWindow       *MtvWindow      ; // ����
	IDirectFBSurface      *MtvSurface     ; // ����
	IDirectFBSurface      *MtvBakSurface  ; // ���ݱ���
	RECT                   BakFace_rect   ; // ���ݱ�������

	IDirectFBDisplayLayer *MtvLayer       ; // ��ʾ��
	IDirectFBEventBuffer  *MtvEventBuffer ; // �¼�����
	IDirectFBFont         *MtvFont        ; // ����
	DFBWindowEvent         MtvWindowEvent ; // ���������¼�
	IDirectFBSurface      *tmpFace        ; // ��ʱ����

	DFBSurfaceDescription  surface_dsc    ;

	typedef struct tagSurfaceSize{
		int width;
		int height;
	} SurfaceSize;
	SurfaceSize tmpFace_dsc; // ������

	void *CreateImageSurface(void *data, size_t length, RECT ct, void* oldface=NULL);
};
#endif
//==============================================================================

//==============================================================================
#ifdef SDLGUI
class CSDLGui: public CBaseGui    // ���� SDL ͼ�νӿ�
{
public:
	CSDLGui ();
	virtual ~CSDLGui(void);

	virtual void GuiRest();
	virtual bool GraphicInit(int argc, char **argv);
	virtual void GuiEnd(void);
	virtual unsigned char GetMouseState(int *x, int *y);
	virtual void *CreateBackground(void *data, size_t length, RECT ct);
	virtual void FreeBackground(void *background);
	virtual void SetShadow(void *EmptyShadow);
	virtual void UpdateShadow(RECT *rect=NULL);
	virtual void DrawImage(void *data, size_t length, RECT ct);
	virtual bool WaitInputEvent(InputEvent *event, int timeout);

	virtual void DrawSoundBar(int volume);                            // ��ʾ����
	virtual void DrawRectangle(RECT rect, TColor color) ;              // ����color�����ɫ

	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // ����Unicode�ַ����ĳ���
	virtual void SetFont(CKtvFont *font);                             // �����µ�����
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);
	                                                                  // �����ַ���
	virtual void Flip(RECT* rect=NULL);                               // ������������ʾ�ύ����ʾ
	virtual void RestoreFade(RECT rect);
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2);
private:
	SDL_Surface *screen;       // ��Ļ��ʾ��
	SDL_Surface *shadow;       // Ӱ�Ӳ�
	SDL_Surface *background;   // ������
#ifdef NEWMOUSE
	MouseCursor *mouse_cursor;
#endif
#ifdef ENABLE_HANDWRITE
	SDL_Surface *handwrite;
#endif
	RECT BG_Rect;
	SDL_Surface *CreateImageSurface(void *data, size_t length, RECT ct);
	void DrawTextSurface(SDL_Surface *surface, const char *value, RECT rect, TAlign align, bool unicode=true);
};
#endif

#ifdef SPHEGUI
#include "SP_Gui.h"

class CSpheGui: public CBaseGui    // ���� SDL ͼ�νӿ�
{
public:
	CSpheGui();
	virtual ~CSpheGui(void);

	virtual void GuiRest();
	virtual bool GraphicInit(int argc, char **argv);
	virtual void GuiEnd(void);
	virtual unsigned char GetMouseState(int *x, int *y);
	virtual void *CreateBackground(void *data, size_t length, RECT ct);
	virtual void FreeBackground(void *background);
	virtual void SetShadow(void *EmptyShadow);
	virtual void UpdateShadow(RECT *rect=NULL);
	virtual void DrawImage(void *data, size_t length, RECT ct);
	virtual bool WaitInputEvent(InputEvent *event, int timeout);

	virtual void DrawSoundBar(int volume);                            // ��ʾ����
	virtual void DrawRectangle(RECT rect, TColor color) ;              // ����color�����ɫ

	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // ����Unicode�ַ����ĳ���
	virtual void SetFont(CKtvFont *font);                             // �����µ�����
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);
	                                                                  // �����ַ���
	virtual void Flip(RECT* rect=NULL);                               // ������������ʾ�ύ����ʾ
	virtual void RestoreFade(RECT rect);
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2);
private:
	int Width, Height;
	HDC hdc;
#define MOUSE           "/dev/psaux"    /* mouse associated with the screen */
#define KEYBOARD        "/dev/tty0"     /* keyboard associated with the screen */
	int     mouse_fd, keyb_fd;
	int    old_kbd_mode;
	struct termios  old_termios;            /* original terminal modes */
	struct termios  new_termios;

#if 0
	SDL_Surface *Shadow;
	SDL_Surface *CreateImageSurface(void *data, size_t length);
	void ShowSDLSurface(SDL_Surface *surface, RECT ct);
	uint32_t GetSurfacePixel(SDL_Surface *surface, int x, int y);
#else
	BITMAP *Shadow;
	BITMAP *CreateImageSurface(void *data, size_t length);
#endif
};
#endif

class CScreen {
public:
	static void FreeScreen();
	static CScreen *CreateScreen();
	CBaseGui *GetGui() { return pGui; }
protected:
	CScreen() {
#ifdef SDLGUI
		pGui = new CSDLGui();
#endif
#ifdef DIRECTFB
		pGui = new CDirectFBGui();
#endif
#ifdef SPHEGUI
		pGui = new CSpheGui();
#endif
	}
	~CScreen();
private:
	CBaseGui *pGui;
	static CScreen *screen;
};

//==============================================================================
#endif

