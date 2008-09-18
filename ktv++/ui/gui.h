/*==============================================================================
 *   T h e   K T V L i n u x   P r o j e c t
 *------------------------------------------------------------------------------
 *  Filename   : gui.h
 *  Author(s)  : Silicon
 *  Copyright  : Silicon
 *
 *  实现对数据表的封装，歌曲列表，歌手资料列表都在这个单元中实现
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
// CBaseGui 类
//  1. 界面的显示接口
//  2. 界面主题接口
//  3. 窗口的管理
//==============================================================================
class CBaseGui
{
public:
	struct timeb opttime;
	CBaseGui();
	virtual ~CBaseGui(void){}
	virtual void GuiRest(){}

	virtual void DrawTextOpt(CKtvOption *opt, CKtvFont *font=NULL, bool update=false);
	                                           // 输出Option操作项
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
	virtual void DrawSoundBar(int volume) = 0;                    // 显示音量

	virtual void DrawRectangle(RECT rect,TColor color) = 0;       // 画框，color框的颜色
	virtual void DrawFillRect(RECT rect, TColor color) = 0;       // 画填充框, color为填充色
	virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;    // 画线
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0) = 0;
	                                                              // 返回字符串的长度
	virtual void SetFont(CKtvFont *font) = 0;                     // 设置新的字体
	virtual void DrawText(const char *value, RECT rect,TAlign align, bool unicode=true) = 0;  // 显字字符串
	virtual void Flip(RECT* rect=NULL) = 0;                       // 将缓冲区的显示提交到显示
	virtual void RestoreFade(RECT rect) = 0;
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2) = 0;
protected:
	CKtvFont *CurFont;         // 当前的字体指针
	RECT *currect;             // 当前提示框
	RECT volrect;              // 音量的显示范围

	CKtvOption promptopt;      // 走字操作项
	pthread_mutex_t CS;        // 同步互斥,实现界面同步
	void GetXyRect(RECT rect, TAlign align, char *text, int len, int *x, int *y);
private:
};

//==============================================================================
#ifdef DIRECTFB

class CDirectFBGui: public CBaseGui    // 基于 DirectFB 图形接口
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

	virtual void DrawSoundBar(int volume);                            // 显示音量
	virtual void DrawRectangle(RECT rect,TColor color);               // 画框，color框的颜色
	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // 返回Unicode字符串的长度
	virtual void SetFont(CKtvFont *font);                             // 设置新的字体
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);          // 显字字符串
	virtual void Flip(RECT* rect=NULL);                               // 将缓冲区的显示提交到显示
	virtual void RestoreFade(RECT rect) {}
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2){}
private:
	IDirectFB             *MtvDFB         ; //
	IDirectFBWindow       *MtvWindow      ; // 窗体
	IDirectFBSurface      *MtvSurface     ; // 表面
	IDirectFBSurface      *MtvBakSurface  ; // 备份表面
	RECT                   BakFace_rect   ; // 备份表面描述

	IDirectFBDisplayLayer *MtvLayer       ; // 显示层
	IDirectFBEventBuffer  *MtvEventBuffer ; // 事件缓冲
	IDirectFBFont         *MtvFont        ; // 字体
	DFBWindowEvent         MtvWindowEvent ; // 窗体输入事件
	IDirectFBSurface      *tmpFace        ; // 临时表面

	DFBSurfaceDescription  surface_dsc    ;

	typedef struct tagSurfaceSize{
		int width;
		int height;
	} SurfaceSize;
	SurfaceSize tmpFace_dsc; // 量表檬

	void *CreateImageSurface(void *data, size_t length, RECT ct, void* oldface=NULL);
};
#endif
//==============================================================================

//==============================================================================
#ifdef SDLGUI
class CSDLGui: public CBaseGui    // 基于 SDL 图形接口
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

	virtual void DrawSoundBar(int volume);                            // 显示音量
	virtual void DrawRectangle(RECT rect, TColor color) ;              // 画框，color框的颜色

	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // 返回Unicode字符串的长度
	virtual void SetFont(CKtvFont *font);                             // 设置新的字体
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);
	                                                                  // 显字字符串
	virtual void Flip(RECT* rect=NULL);                               // 将缓冲区的显示提交到显示
	virtual void RestoreFade(RECT rect);
	virtual void DrawHandWrite(int x1, int y1, int x2, int y2);
private:
	SDL_Surface *screen;       // 屏幕显示层
	SDL_Surface *shadow;       // 影子层
	SDL_Surface *background;   // 背景层
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

class CSpheGui: public CBaseGui    // 基于 SDL 图形接口
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

	virtual void DrawSoundBar(int volume);                            // 显示音量
	virtual void DrawRectangle(RECT rect, TColor color) ;              // 画框，color框的颜色

	virtual void DrawFillRect(RECT rect, TColor color);
	virtual void DrawLine(int x1, int y1, int x2, int y2);
	virtual bool UnTextExtent(char *text,int len, int *w, int *h, int extwidth=0);
	                                                                  // 返回Unicode字符串的长度
	virtual void SetFont(CKtvFont *font);                             // 设置新的字体
	virtual void DrawText(const char *value,RECT rect,TAlign align, bool unicode=true);
	                                                                  // 显字字符串
	virtual void Flip(RECT* rect=NULL);                               // 将缓冲区的显示提交到显示
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

