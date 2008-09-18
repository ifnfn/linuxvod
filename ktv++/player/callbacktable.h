/*****************************************
 Copyright © 2001-2003
 Sigma Designs, Inc. All Rights Reserved
 Proprietary and Confidential
 *****************************************/
/**
  @file   informationtable.h
  @brief

  <long description>

  @author Emmanuel Michon
  @date   2002-08-22
*/

#ifndef __CALLBACKTABLE_H__
#define __CALLBACKTABLE_H__

#ifndef ALLOW_OS_CODE
#define ALLOW_OS_CODE 1
#endif

#include "rmexternalapi.h"
#include "callbacktable.h"

//#include "common.h"

const RMFDVDCallbackTable *info_get_table (void);

// audio
RMuint8 info_get_ASTN (void);
void info_set_ASTN (RMuint8 ASTN);

// subpicture
RMuint8 info_get_SPSTN (void);
RMbool info_get_display (void);
void info_set_SPSTN (RMuint8 SPSTN);
void info_set_display (RMbool display);
void info_set_future_SPSTN_display (RMuint8 SPSTN, RMbool display);
RMbool info_get_future_SPSTN_display (RMuint8 *SPSTN, RMbool  *display);

// angle
void info_set_angle (RMuint8 AGLN);
RMuint8 info_get_nangles (void);
RMuint8 info_get_angle (void);

RMuint8 info_get_nbutton (void);

typedef enum {
	PLAY_STATE_NO_DISC_IN_DRIVE,
	PLAY_STATE_STOPPED,
	PLAY_STATE_STOP_AND_BOOKMARK,
	PLAY_STATE_PLAY,
	PLAY_STATE_PAUSE,
	APP_TRICK_SF1,
	APP_TRICK_SF2,
	APP_TRICK_FF1,
	APP_TRICK_FF2,
	APP_TRICK_FF3,
	APP_TRICK_SB1,
	APP_TRICK_SB2,
	APP_TRICK_FB1,
	APP_TRICK_FB2,
	APP_TRICK_FB3,
	// GUI SEPCIFIC
	SETUP_STATE
	///////////////
} PlayState;

void info_set_state (PlayState state);
PlayState info_get_state (void);

RMuint32 info_get_chapter_elapsed_time (void);
RMuint32 info_get_chapter_left_time (void);

RMdvdDomainType info_get_domain_type (void);

void info_enable_dirty_detection (RMbool enable);

#endif // __CALLBACKTABLE_H__
