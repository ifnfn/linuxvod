#ifndef SUPERTUX_MOUSECURSOR_H
#define SUPERTUX_MOUSECURSOR_H

#include <SDL.h>

#define MC_STATES_NB 3
enum {
	MC_NORMAL,
	MC_CLICK,
	MC_LINK
};

class MouseCursor
{
public:
	MouseCursor(SDL_Surface** back, SDL_Surface *mainscreen, const char *cursor_file, int frames);
	~MouseCursor();
	int state();
	void set_state(int nstate);
	void set_mid(int x, int y);
	void draw();
	void SetMouseXy(int x, int y);
private:
	int mid_x, mid_y;
	int state_before_click;
	int cur_state;
	int cur_frame, tot_frames;
	SDL_Surface *cursor;
private:
	SDL_Surface *screen;
	SDL_Surface **shadow;
	SDL_Rect postions[2];
	int draw_part(float sx, float sy, float x, float y, float w, float h,  Uint8 alpha=255, bool update=false);
};

#endif /*SUPERTUX_MOUSECURSOR_H*/
