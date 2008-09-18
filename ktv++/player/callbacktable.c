/*****************************************
 Copyright © 2001-2003
 Sigma Designs, Inc. All Rights Reserved
 Proprietary and Confidential
 *****************************************/

/**
  @file informationtable.c
  @brief Sample DVD application using RMF API

  @date   2002-11-07
*/
#include "callbacktable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

static RMuint8 g_ASTN = 0;
static RMuint8 g_SPSTN = 0;
static RMbool g_display = 0;
static RMuint8 g_future_SPSTN = 0;
static RMbool g_future_display = 0;
static RMuint8 g_nangles = 0;
static RMuint8 g_angle = 0;
static RMbool g_pause = FALSE;
static RMuint8 g_nbutton = 0;
static RMuint32 g_chapterElapsed = 0;
static RMuint32 g_chapterLeft = 0;
static RMuint64 g_prev_spstn_set;
static RMdvdDomainType g_domain = RM_DVD_DOMAIN_FPPGC;
static RMbool g_dirty_enable = TRUE;

static RMuint64 RMlocalGetTimeInMicroSeconds(void)
{
	static RMbool firsttimehere=TRUE;
	static struct timeval timeorigin;

	struct timeval now;

	// the first time, set the origin.
	if (firsttimehere) {
		gettimeofday(&timeorigin,NULL);
		firsttimehere=FALSE;
	}

	gettimeofday(&now,NULL);

	return (RMuint64)( ( (RMint64)now.tv_sec
		-(RMint64)timeorigin.tv_sec )*1000000LL
		+((RMint64)now.tv_usec
		-(RMint64)timeorigin.tv_usec));
}

static const char *time_to_string (RMuint32 time)
{
	static char buffer[40];
	RMuint8 hours, minutes, seconds;

	// get rid of tenths of seconds
	time /= 10;

	hours = time / 3600;
	minutes = (time / 60) - (hours * 60);
	seconds = time % 60;

	sprintf (buffer, "%.2u:%.2u:%.2u", hours, minutes, seconds);

	return buffer;
}

static void display_title(const char *str)
{
}

static void display_time(const char *str)
{
}

static void display_chapter(const char *str)
{
}

static void display_domain(const char *str)
{
}

static void display_message (const char *str)
{
}

static void display_play_state(const char *str)
{
}

static void my_streaming_status (RMuint32 UOPs, RMdvdDomainType domainType,
				RMuint32 chapterElapsed, RMuint32 titleElapsed,
				RMuint32 chapterLeft, RMuint32 titleLeft,
				RMuint16 PTTN, RMuint8 TTN,
				RMuint8 AGLN, RMuint8 AGLTot,
				void *userData)
{
	static char buffer[10];

	g_nangles = AGLTot;

	display_time (time_to_string (titleElapsed));
	sprintf (buffer, "%.2u", TTN);
	display_title (buffer);
	sprintf (buffer, "%.3u", PTTN);
	display_chapter (buffer);

	g_chapterElapsed = chapterElapsed;
	g_chapterLeft = chapterLeft;

	g_domain = domainType;

	//g_angle = AGLN;
	/*g_uops = UOPs;
	g_domain = domainType;
	g_angle = AGLN - 1;
	g_nangles = AGLTot;
	if (g_nangles != 0) {
		RMDBGPRINT((ENABLE,"Angles: %u, Angle: %u | ", AGLTot, AGLN));
	}

	RMDBGPRINT((ENABLE,"Time: %u | ", titleElapsed));
	*/

	{
		RMascii *domainName = "FIXME";
		switch (domainType) {
			case RM_DVD_DOMAIN_FPPGC:
				domainName = "FPPGC";
				break;
			case RM_DVD_DOMAIN_VMGM:
				domainName = "VMGM";
				break;
			case RM_DVD_DOMAIN_VTSM:
				domainName = "VTSM";
				break;
			case RM_DVD_DOMAIN_VTSTT:
				domainName = "VTSTT";
				break;
		}
		display_domain (domainName);
	}
}

static void my_stn_status (RMuint8 ASTN,
			RMuint8 SPSTN, RMbool display,
			void *userData)
{
	if (g_future_SPSTN != SPSTN ||
		g_future_display != display) {
		RMuint64 now;
		// This is a timeout on the content of SPSTN and future_SPSTN.
		//printf ("stn status with potential timeout: %u %u\n", SPSTN, display);
		now = RMlocalGetTimeInMicroSeconds ();
		if ((now - g_prev_spstn_set) >= 5000000) {
			g_future_SPSTN = SPSTN;
			g_future_display = display;
			g_prev_spstn_set = now;
		}
	} else {
		//printf ("stn status no difference: %u %u\n", SPSTN, display);
		g_SPSTN = SPSTN;
		g_display = display;
	}
}


static void my_start_button (RMuint8 BTNN, RMdvdButtonsInformation buttons[MAX_BTN], void *data)
{
	g_nbutton = BTNN;
	/*	g_nbutton = BTNN;
	RMDBGPRINT((ENABLE,"Buttons: %u\n", g_nbutton));
	if (g_nbutton != 0) {
		RMdvdButtonsInformation buttons;
		RMstatus status = RMdvdQueryButtons (g_pdvd, &buttons);
		if (status != RM_OK) {
			RMDBGPRINT((ENABLE,"Could not get Button information.\n"));
		} else {
			for (RMuint8 i = 0; i < buttons.n; i++) {
			}
		}
		}*/
}

static void my_end_button (void *data)
{
	g_nbutton = 0;
	display_message ("No buttons anymore");
}

static void my_selected_button (RMuint8 BTNN, void *data)
{
	/*g_button = BTNN;
	if (g_nbutton != 0) {
		RMDBGPRINT((ENABLE,"Button: %u", g_button));
		}*/
}

static void my_still_on (void *userData)
{
	g_pause = TRUE;
	info_set_state (PLAY_STATE_PAUSE);
	display_message ("DVD Pause...");
}

static void my_still_off (void *userData)
{
	g_pause = FALSE;
	info_set_state (PLAY_STATE_PLAY);
	display_message ("DVD Play...");
}


static void my_end_playback (void *userData)
{
	info_set_state (PLAY_STATE_STOPPED);
	display_message ("DVD Playback Finished.");
}

static void my_scan_end (void *userData)
{
	display_message("End of Scan");
	info_set_state (PLAY_STATE_PLAY);
}


static void my_get_next_playlist_item (RMdvdPlaylistItem *const item, void *userData)
{
/*
	RMdvdPlaylistItem *tmp;
	RMstatus status = shell_thread_get_next_playlist (&tmp);
	if (status != RM_OK) {
		item->type = RM_DVD_PLAYLIST_END;
	} else {
		item->type = tmp->type;
		item->PTTN = tmp->PTTN;
		item->TTN = tmp->TTN;
	}
*/
}

static void my_change_parental_level (
		RMuint8 requestedLevel,
		RMbool *const allowed,
		void *userData)
{
	printf("Allowed parental level change to level %u\n", requestedLevel);
	*allowed = TRUE;

}

static void my_eject (void *userData)
{
	info_set_state (PLAY_STATE_NO_DISC_IN_DRIVE);
	display_message ("User ejected drive himself. Bad.");
}

static RMbool my_disc_dirty (void *userData)
{
	if (g_dirty_enable) {
		info_set_state (PLAY_STATE_STOPPED);
		display_message ("Disc is Dirty. Stop playback");
		return FALSE;
	} else {
		display_message ("Disc is Dirty. Do not stop playback");
		return TRUE;
	}
}

static void my_fatal (void *userData)
{
	info_set_state (PLAY_STATE_STOPPED);
	display_message ("Disc is truly deadly dirty");
}

static void my_macrovision_level(
		RMdvdMacrovisionLevel macrovisionLevel,
		void *userData)
{
	//printf ("macrovision level: %u\n", macrovisionLevel);
}


const RMFDVDCallbackTable *info_get_table (void)
{
	static RMFDVDCallbackTable callbackTable = {
		streamingStatus:my_streaming_status,
		STNStatus:my_stn_status,
		startButtons:my_start_button,
		endButtons:my_end_button,
		selectedButton:my_selected_button,
		stillOn:my_still_on,
		stillOff:my_still_off,
		endPlayback:my_end_playback,
		scanEnd:my_scan_end,
		getNextPlaylistItem:my_get_next_playlist_item,
		changeParentalLevel:my_change_parental_level,
		eject: my_eject,
		macrovisionLevel: my_macrovision_level,
		fatalErrorReadingDevice: my_fatal,
		discIsDirty: my_disc_dirty,
	};
	RMuint64 now;
	now = RMlocalGetTimeInMicroSeconds ();
	g_prev_spstn_set = now;

	return &callbackTable;
}


RMuint8 info_get_ASTN (void)
{
	return g_ASTN;
}
void info_set_ASTN (RMuint8 ASTN)
{
	g_ASTN = ASTN;
}
RMuint8 info_get_SPSTN (void)
{
	return g_SPSTN;
}
void info_set_SPSTN (RMuint8 SPSTN)
{
	g_SPSTN = SPSTN;
}
RMbool info_get_display (void)
{
	return g_display;
}
void info_set_display (RMbool display)
{
	g_display = display;
}
void info_set_future_SPSTN_display (RMuint8 SPSTN, RMbool display)
{
	RMuint64 now;
	now = RMlocalGetTimeInMicroSeconds ();
	g_prev_spstn_set = now;

	g_future_SPSTN = SPSTN;
	g_future_display = display;
}
RMbool info_get_future_SPSTN_display (RMuint8 *SPSTN, RMbool  *display)
{
	RMuint64 now;
	now = RMlocalGetTimeInMicroSeconds ();

	if ((g_future_SPSTN != g_SPSTN || g_future_display != g_display) &&
		(now - g_prev_spstn_set) >= 5000000) {
		//printf ("stn timeout in get: %u %u\n", g_future_SPSTN, g_future_display);
		g_future_SPSTN = g_SPSTN;
		g_future_display = g_display;
		g_prev_spstn_set = now;
	} else {
		//printf ("no timeout in get: %u %u\n", g_future_SPSTN, g_future_display);
	}

	*SPSTN = g_future_SPSTN;
	*display = g_future_display;
	if (g_future_SPSTN != g_SPSTN || g_future_display != g_display)
		return TRUE;
	else
		return FALSE;
}
void info_set_angle (RMuint8 AGLN)
{
	g_angle = AGLN;
}
RMuint8 info_get_nangles (void)
{
	return g_nangles;
}
RMuint8 info_get_angle (void)
{
	return g_angle;
}
RMuint8 info_get_nbutton (void)
{
	return g_nbutton;
}


static PlayState g_state = PLAY_STATE_NO_DISC_IN_DRIVE;

static const char *state_to_string (PlayState state)
{
	switch (state) {
	case PLAY_STATE_NO_DISC_IN_DRIVE:
		return "NO_DISC";
		break;
	case PLAY_STATE_STOPPED:
		return "STOP";
		break;
	case PLAY_STATE_STOP_AND_BOOKMARK:
		return "STOP_AND_BOOKMARK";
		break;
	case PLAY_STATE_PLAY:
		return "PLAY";
		break;
	case PLAY_STATE_PAUSE:
		return "PAUSE";
		break;
	case APP_TRICK_SF1:
		return "SF1";
		break;
	case APP_TRICK_SF2:
		return "SF2";
		break;
	case APP_TRICK_FF1:
		return "FF1";
		break;
	case APP_TRICK_FF2:
		return "FF2";
		break;
	case APP_TRICK_FF3:
		return "FF3";
		break;
	case APP_TRICK_SB1:
		return "SB1";
		break;
	case APP_TRICK_SB2:
		return "SB2";
		break;
	case APP_TRICK_FB1:
		return "FB1";
		break;
	case APP_TRICK_FB2:
		return "FB2";
		break;
	case APP_TRICK_FB3:
		return "FB3";
		break;
	// GUI SPECIFIC
	case SETUP_STATE:
		return "SETUP";
		break;
	////////////////
	}
	return "";
}

void info_set_state (PlayState state)
{
	g_state = state;
	display_play_state (state_to_string (state));
}
PlayState info_get_state (void)
{
	return g_state;
}

RMuint32 info_get_chapter_elapsed_time (void)
{
	return g_chapterElapsed;
}

RMuint32 info_get_chapter_left_time (void)
{
	return g_chapterLeft;
}

RMdvdDomainType info_get_domain_type (void)
{
	return g_domain;
}

void info_enable_dirty_detection (RMbool enable)
{
	g_dirty_enable = enable;
}
