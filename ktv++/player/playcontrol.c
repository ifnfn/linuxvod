#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "playcontrol.h"
#include "config.h"
#include "showjpeg.h"
#include "selected.h"
#include "songdata.h"
#include "strext.h"

#include "callbacktable.h"

#ifdef OSDMENU
#include "osd.h"
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0100000
#endif

#define PAL_SCREEN_WIDTH   720
#define PAL_SCREEN_HEIGHT  576
#define NTSC_SCREEN_WIDTH  720
#define NTSC_SCREEN_HEIGHT 480

#define HISTO_COLOR_RED  5
#define HISTO_COLOR_BLUE 6

#define MAX_BOARDS 4

#define BUFCOUNT 40
#define BUFSIZE  8192
#define TOTAL_MEMORY_NEEDED (8*1024*1024)

#define RM_MAX_STRING 1024

static const char *PUSHURL = "[%d]PUSH@%s://#buffercount=%d,buffersize=%d,bufferallocation=external/";
static const char *FILEURL = "[%d]FILE@%s://%s";

static void callback(RMTcontrolInterface ctrl, void *userData, RMmessage message, RMuint32 val);

static int ReadSongTrack(INFO *pInfo);
static jmp_buf g_env;

static int DiscIsReady(INFO *pInfo)
{
	RMstatus status;
	RMbool isOpen, isDiscInDrive, isReady;

	if (pInfo->Disc == 0) return 1;
	status = RMFDiscIsTrayOpen(pInfo->Disc, &isOpen);
	if (status != RM_OK) {
		printf ("Cannot check Tray status.\n");
		return 0;
	}
	if (isOpen) {
		printf("Tray is Open");
		return 0;
	} else {
		status = RMFDiscIsDiscPresent (pInfo->Disc, &isDiscInDrive);
		if (status != RM_OK) {
			printf ("Cannot check if disc is present.\n");
			return 0;
		}
		if (!isDiscInDrive) {
			printf("Disc is not present");
			return 0;
		} else {
			status = RMFDiscIsReady (pInfo->Disc, &isReady);
			if (status != RM_OK) {
				printf ("Cannot check is disc is ready.\n");
				return 0;
			}
			if (!isReady) {
				printf("Disc is not ready.");
				return 0;
			} else {
				printf("Disc is ready.");
				return 1;
			}
		}

	}

	return 0;
}

static RMFDVDUserSettings g_user_settings = {
	videoMode                          : RMF_DVD_VIDEO_PANSCAN,
	countryCodeForParentalLevel        : RM_COUNTRY_UNITED_STATES,
	parentalLevel                      : RM_DVD_PARENTAL_LEVEL_DISABLED,
	menuDescriptionLanguageCode        : RM_LANGUAGE_ENGLISH,
	initialLanguageForAudioStream      : RM_LANGUAGE_ENGLISH,
	initialLanguageForSubPictureStream : RM_LANGUAGE_ENGLISH,
	playerRegionCode                   : RM_DVD_REGION_0,
	dirtyDiscSensitivity               : 0x1E,
	spdifOutputConfig                  : RMF_DVD_SPDIF_DTSCOMPRESSED_ENABLE,
};

static void PrepareStartPlay(INFO *pInfo)
{
	int is_ready;
	RMstatus status;

	is_ready = DiscIsReady(pInfo);
	if (!is_ready) {
		return;
	}

	status = RMFDVDConstruct (pInfo->DvdCtrl,
			info_get_table(),
			NULL,
			&g_user_settings);

	if (status == RM_ERROR_RPC2) {
		if (g_user_settings.playerRegionCode == RM_DVD_REGION_FREE) {
			printf ("You have an RPC2 DVD drive. It is not possible to "
				"set region free on an RPC2 drive.\n");
		} else {
			printf ("Invalid Region RPC2\n");
		}
	} else if (status == RM_ERROR_RPC2_NOT_SET) {
		printf ("Your RPC2 Drive does not have a region set. This is bad. "
			"Please, set a region in your drive.\n");
	} else if (status == RM_ERROR_REGION) {
		printf ("Invalid Region\n");
	} else if (status != RM_OK) {
		printf ("Could not initialize DVD (rc=%d).\n",status);
	}
}

static void RMFDVDPlay(INFO *pInfo)
{
	RMstatus status;

	PrepareStartPlay(pInfo);
	status = RMFDVDTitle_Play(pInfo->DvdCtrl, 0);
	if (status == RM_OK) {
		printf("Playing DVD From start");
		info_set_state ( PLAY_STATE_PLAY);
	} 
	else {
		RMdvdTitleSRPTI tt_srpti;
		status = RMFDVDQueryTT_SRPT (pInfo->DvdCtrl, &tt_srpti);
		if (status != RM_OK || tt_srpti.numberOfTitles == 0) {
			printf("DVD Broken: no titles.");
			return;
		}
		status = RMFDVDTitle_Play (pInfo->DvdCtrl, 1);
		if (status != RM_OK) {
			printf("DVD Broken: cannot play title 1");
			return;
		}
		printf("DVD Broken: playing title 1");
	}
}

void ConvertToTime(RMuint32 totalSeconds, RMuint32 *hours, RMuint32 *minutes, RMuint32 *seconds)
{
	*seconds = (RMuint32) (totalSeconds % 60);
	*minutes = (RMuint32) (totalSeconds/60);
	*hours   = (RMuint32) (*minutes/60);
	*minutes = (RMuint32) (*minutes%60);
}

void DeleteFirstSongList(void)
{
	if (SelectedList.count > 0){
		BroadcastSongRec(DELSONG, SelectedList.items + 0);
		DelSongIndex(0);
	}
}

int GetRealVolume(INFO *pInfo)
{
	int EquVolume, tmp = 0;

	EquVolume = pInfo->CurTrack == MUSICTRACK ? pInfo->PlayingSong.VolumeK : pInfo->PlayingSong.VolumeS;
	if (EquVolume != 0) {
		tmp = pInfo->volume * (1 + (EquVolume - 50) / 50.0);
		if (tmp > pInfo->maxvolume)
			tmp = pInfo->maxvolume;
		if (tmp < pInfo->minvolume)
			tmp = pInfo->minvolume;
	}

	return tmp;
}

bool SongListFirstPlay(INFO *pInfo) // 播放已点歌曲列表中第一首
{
	if (pInfo->lock == true) return false;
	if ( (pInfo->PlaySelect == psSelected) || (pInfo->PlaySelect == psDefault) ){
		if (SelectedList.count > 0){ // 如果点歌列表中有歌曲，打开数据通道
			pInfo->PlaySelect  = psSelected;
			pInfo->PlayingSong = SelectedList.items[0];
			PRINTTRACK(SongStartAudioTrack);
			if (SongStartAudioTrack == trMusic)
				pInfo->CurTrack = MUSICTRACK;
			else if (SongStartAudioTrack == trSong)
				pInfo->CurTrack = SOUNDTRACK;
//			else if (SongStartAudioTrack == trDefault)
//				pInfo->CurTrack = pInfo->CurTrack==MUSICTRACK ? SOUNDTRACK : MUSICTRACK;
			// Show Next song OSD.
			if (SelectedList.count > 1) {
//				char tmp[512] = "speed=1&x=100&y=30&size=36&Foreground=255,0,0,255&text=";
				char tmp[512] = "text=";
				strcat(tmp, "下一首：");
				strcat(tmp, SelectedList.items[1].SongName);
//				printf("tmp=%s\n", tmp);
#ifdef OSDMENU
				CleanScrollTextList(0);
				CreateScrollTextStr(0, tmp);
#endif
			}
		}
		else{
			pInfo->PlaySelect = psDefault;
			memset(&pInfo->PlayingSong, 0, sizeof(SelectSongNode));
			char *p = GetRandomDefaultSong();
			if (p){ // 如果没有默认歌曲
//				StrToSelectSongNode(p, &pInfo->PlayingSong);
				char tmp[100];
				strcpy(tmp, p);
				char *code= strtok(tmp, ",");
				char *ext = strtok(NULL, ",");
				if (ext == NULL) ext = "VOB";

				strcpy(pInfo->PlayingSong.SongCode  , code);
				strcpy(pInfo->PlayingSong.StreamType, ext );
				pInfo->PlayingSong.VolumeK = pInfo->PlayingSong.VolumeS = 50;
				pInfo->PlayingSong.Klok = 'L';
				pInfo->PlayingSong.Sound= 'R';
				PRINTTRACK(SongStartAudioTrack);
				if (DefaultAudioTrack == trMusic)
					pInfo->CurTrack = MUSICTRACK;
				else if (DefaultAudioTrack == trSong)
					pInfo->CurTrack = SOUNDTRACK;
				else
					pInfo->CurTrack = MUTE;
				pInfo->PlayingSong.SoundMode = DefaultSoundMode; // 灯光控制模式
			}
			else {
				DEBUG_OUT("ShowJpeg %s\n", Background);
				if (ShowJpeg(pInfo->rua, Background) == false)
					SETBLACKFRAME(pInfo->PropCtrl);
				NoSongLock();
				return false;
			}
		}
	}
	else if (pInfo->PlaySelect == psHiSong){
		memset(&pInfo->PlayingSong, 0, sizeof(SelectSongNode));
		char *p = GetRandomHiSong();
		if (p) {
			char tmp[100];
			strcpy(tmp, p);
			char *code=strtok(tmp, ",");
			char *ext =strtok(NULL, ",");
			if (ext == NULL) ext = "VOB";

			strcpy(pInfo->PlayingSong.SongCode  , code);
			strcpy(pInfo->PlayingSong.StreamType, ext );
			pInfo->PlayingSong.VolumeK = pInfo->PlayingSong.VolumeS = 50;
//			StrToSelectSongNode(p, &pInfo->PlayingSong);
//			pInfo->PlayingSong.SoundMode = HiSoundMode; // 灯光控制模式
		}
		else
			return false;
	}
	else if (pInfo->PlaySelect == ps119){
		char tmp[50];
		strcpy(tmp, Fire119);
		char *code = strtok(tmp, ",");
		char *ext  = strtok(NULL, ",");
		if (ext == NULL) ext ="M1S";
		strcpy(pInfo->PlayingSong.SongCode  , code);
		strcpy(pInfo->PlayingSong.StreamType, ext );
		pInfo->PlayingSong.VolumeK = pInfo->PlayingSong.VolumeS = 50;
//		StrToSelectSongNode(Fire119, &pInfo->PlayingSong);		
//		pInfo->PlayingSong.SoundMode = Fire119SoundMode; // 灯光控制模式
	}

	if (strcmp( pInfo->PlayingSong.SongCode, "0") == 0)
	{
		strcpy(pInfo->VideoFile, "");
		pInfo->MediaType = mtDISC;
	}
	else {
//		char *tmpfile = GetLocalFile(pInfo->PlayingSong.SongCode, NULL); // 读取本地文件
		char *tmpfile = GetHttpURL(pInfo->PlayingSong.SongCode, pInfo->PlayingSong.Password); // 读取本地文件
		printf("tmpfile=%s\n", tmpfile);
		if (tmpfile) {// 如果本地文件存在
			strcpy(pInfo->VideoFile, tmpfile);
			pInfo->MediaType= mtFILE;
		}
		else {
			sprintf(pInfo->VideoFile, "code=%s", pInfo->PlayingSong.SongCode);
			pInfo->MediaType= mtPUSH;
		}
	}
//	printf("pInfo->VideoFile = %s\n", pInfo->VideoFile);
	BroadcastSongRec(PLAYSONG, &pInfo->PlayingSong);
	return StartPlayer(pInfo);
}

bool MuteSwitchPlayer(INFO *pInfo)
{
	if (pInfo->Mute)
		PlayerResumeMute(pInfo);
	else
		PlayerMute(pInfo);

	return pInfo->Mute;
}

void PlayerMute(INFO *pInfo)
{
	pInfo->Mute = true;
	if (pInfo->rua) {
		RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeRight", 0);
		RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeLeft",  0);
	}
}

void PlayerResumeMute(INFO *pInfo)
{
	pInfo->Mute = false;
	AddVolume(pInfo, 0);
}

void AddVolume(INFO *pInfo, int v)
{
	if (!pInfo->Mute) {
		pInfo->volume += v;
		if (pInfo->volume < pInfo->minvolume)
			pInfo->volume = pInfo->minvolume;
		if (pInfo->volume > pInfo->maxvolume)
			pInfo->volume = pInfo->maxvolume;
		int tmpvol = GetRealVolume(pInfo);
		if (pInfo->rua) {
			RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeRight", tmpvol);
			RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeLeft",  tmpvol);
		}
	}
}

static int ReadSongTrack(INFO *pInfo)
{
	RMuint32 numStreams = 0;
	RMdemuxStreamType demuxStreamType;
	RMuint32 i;

	pInfo->MaxVideoStreamNum = pInfo->MaxAudioStreamNum = 0;
	RMFGetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL,"DEMUX","NumberOfStreams",&numStreams); // 得到流数量
	for (i=0; i<numStreams; i++){
		demuxStreamType.streamNumber = i;
		RMFGetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL, "DEMUX", "StreamType", \
			&demuxStreamType, sizeof(RMdemuxStreamType));
		switch (demuxStreamType.demuxStreamType){
			case STREAM_TYPE_MPEG1_VIDEO:
			case STREAM_TYPE_MPEG2_VIDEO:
			case STREAM_TYPE_MPEG4_VIDEO:
				if (pInfo->MaxVideoStreamNum < MAXVIDEONUM){
					pInfo->videoStreams[pInfo->MaxVideoStreamNum].id    = demuxStreamType.streamPID;
					pInfo->videoStreams[pInfo->MaxVideoStreamNum].subId = demuxStreamType.streamSubID;
					pInfo->MaxVideoStreamNum++;
				}
				break;
			case STREAM_TYPE_MPEG1_AUDIO:
			case STREAM_TYPE_MPEG2_AUDIO:
			case STREAM_TYPE_AC3:
			case STREAM_TYPE_PCM:
			case STREAM_TYPE_AAC_ADTS:
			case STREAM_TYPE_MPEG4_AUDIO:
				if (pInfo->MaxAudioStreamNum < MAXAUDIONUM){
					pInfo->audioStreams[pInfo->MaxAudioStreamNum].id    = demuxStreamType.streamPID;
					pInfo->audioStreams[pInfo->MaxAudioStreamNum].subId = demuxStreamType.streamSubID;
					pInfo->audioType[pInfo->MaxAudioStreamNum] = demuxStreamType.demuxStreamType;
					pInfo->MaxAudioStreamNum++;
				}
				break;
			case STREAM_TYPE_RESERVED:
			case STREAM_TYPE_MPEG2_PRIVATE:
			case STREAM_TYPE_MPEG2_PES_PRIVATE:
			case STREAM_TYPE_MHEG:
			case STREAM_TYPE_DSMCC:
			case STREAM_TYPE_H222:
			case STREAM_TYPE_DSMCC_A:
			case STREAM_TYPE_DSMCC_B:
			case STREAM_TYPE_DSMCC_C:
			case STREAM_TYPE_DSMCC_D:
			case STREAM_TYPE_MPEG1_AUX:
			case STREAM_TYPE_FLEXMUX_PES:
			case STREAM_TYPE_FLEXMUX_SYSTEM:
			case STREAM_TYPE_DSMCC_SDP:
			case STREAM_TYPE_SUBPICTURE:
			case STREAM_TYPE_NAVIGATION:
//			case STREAM_TYPE_H264_VIDEO:
//			case STREAM_TYPE_AOB_PCM:
//			case STREAM_TYPE_MLP:
//			case STREAM_TYPE_DTS:
//				break;
			default:
				break;
		}
	}

	return numStreams;
}

void AudioSwitchPlayer(INFO *pInfo)
{
	pInfo->CurTrack = pInfo->CurTrack == MUSICTRACK ? SOUNDTRACK : MUSICTRACK; // 反一下。
	SetAudioChannel(pInfo);
}

void SetAudioChannel(INFO *pInfo)
{
	eAudioMode_type audiomode;
	RMint32 tmpstream;
	PRINTTRACK(pInfo->CurTrack);

	if (pInfo->CurTrack == MUSICTRACK) {      // 原唱
//		printf("pInfo->CurTrack = MUSICTRACK\n");
		if ((pInfo->PlayingSong.Klok == 'L') || (pInfo->PlayingSong.Klok == 'R')) { // 如果是左右声道
			if (pInfo->PlayingSong.Klok == 'L')
				audiomode = eAudioMode_MonoLeft;
			else if (pInfo->PlayingSong.Klok == 'R')
				audiomode = eAudioMode_MonoRight;
			RUA_DECODER_SET_PROPERTY(pInfo->rua, 
					AUDIO_SET, eAudioMode, 
					sizeof(audiomode), &audiomode);
		}
		else {
			tmpstream = pInfo->PlayingSong.Klok>='0' ? 
				pInfo->PlayingSong.Klok - '0' : pInfo->PlayingSong.Klok;

			if (strcasecmp(pInfo->PlayingSong.StreamType, "DIVX") == 0) {
				RMFSetGenericProperty(pInfo->PropCtrl, 
						RM_PROPERTY_EXTERNAL,
						"FILE", "SELECT_AUDIO_STREAM", 
						&tmpstream, sizeof(RMint32));
			}
			else if (tmpstream < pInfo->MaxAudioStreamNum) {
				RMFSetGenericProperty(pInfo->PropCtrl,
						RM_PROPERTY_EXTERNAL, 
						"DEMUX", "AudioPID",
						&pInfo->audioStreams[tmpstream], 
						sizeof(RMstreamId));
			}
		}
	}
	else if (pInfo->CurTrack == SOUNDTRACK) { // 伴唱
//		printf("pInfo->CurTrack = SOUNDRACK\n");
		if ((pInfo->PlayingSong.Sound == 'L') || (pInfo->PlayingSong.Sound == 'R')) { // 如果是左右声道
			if (pInfo->PlayingSong.Sound == 'L')
				audiomode = eAudioMode_MonoLeft;
			else if (pInfo->PlayingSong.Sound == 'R')
				audiomode = eAudioMode_MonoRight;
			RUA_DECODER_SET_PROPERTY(pInfo->rua, AUDIO_SET, eAudioMode, sizeof(audiomode), &audiomode);
		}
		else {
			tmpstream = pInfo->PlayingSong.Sound>='0'?
				pInfo->PlayingSong.Sound - '0' : pInfo->PlayingSong.Sound;
			if (strcasecmp(pInfo->PlayingSong.StreamType, "DIVX") == 0) {
				RMFSetGenericProperty(pInfo->PropCtrl, 
						RM_PROPERTY_EXTERNAL, 
						"FILE", "SELECT_AUDIO_STREAM", 
						&tmpstream, 
						sizeof(RMint32));
			}
			else if (tmpstream < pInfo->MaxAudioStreamNum) {
				RMFSetGenericProperty(pInfo->PropCtrl, 
						RM_PROPERTY_EXTERNAL, 
						"DEMUX", "AudioPID",
						&pInfo->audioStreams[tmpstream],
						sizeof(RMstreamId));
			}
		}
	}
	else if (pInfo->CurTrack == MUTE) {
		PlayerMute(pInfo);
		return;
	}
	AddVolume(pInfo, 0);
}

int PlayAudio(INFO *pInfo, char *type, 
		RMuint32 samplerate, 
		RMuint32 numberofchannels, 
		RMuint32 numberofbitspersample, 
		const char *file)
{
#define CHUNKSIZE 5000
#define COPYANDQUEUEDATA_TIMEOUT_MICROSEC 400000
#define HISTSIZE 5

	int fd,actuallyread;
	char buf[CHUNKSIZE];
	eAudioFormat_type audioformat;

	if (strcmp(type,"mpeg") == 0)
		audioformat=eAudioFormat_MPEG2;
	else if (strcmp(type,"ac3") == 0)
		audioformat=eAudioFormat_AC3;
	else if (strcmp(type,"pcm") == 0)
		audioformat=eAudioFormat_PCM;
	else if (strcmp(type,"revpcm") == 0)
		audioformat=eAudioFormat_REVERSE_PCM;
	else
		return -1;

	if (pInfo->rua==NULL)
		return -1;

	fd=open(file,O_RDONLY|O_LARGEFILE);
	if (fd == -1)
		return -1;

	RUA_DECODER_STREAMSTOP(pInfo->rua,REALMAGICHWL_AUDIO);
	RUA_DECODER_SET_PROPERTY(pInfo->rua,AUDIO_SET,eAudioFormat,sizeof(eAudioFormat_type),&audioformat);
	RUA_DECODER_SET_PROPERTY(pInfo->rua,AUDIO_SET,eAudioSampleRate,sizeof(RMuint32),&samplerate);
	RUA_DECODER_SET_PROPERTY(pInfo->rua,AUDIO_SET,eAudioNumberOfChannels,sizeof(RMuint32),&numberofchannels);
	RUA_DECODER_SET_PROPERTY(pInfo->rua,AUDIO_SET,eAudioNumberOfBitsPerSample,sizeof(RMuint32),&numberofbitspersample);
	RUA_DECODER_STREAMPLAY(pInfo->rua,REALMAGICHWL_AUDIO,0,0);

	while (1) {
		my_MPEG_WRITE_DATA Q={0,};
		actuallyread=read(fd,buf,CHUNKSIZE);
		if(actuallyread<=0) {
			close(fd);
			break;
		}

		Q.pData=(unsigned char *)buf;
		Q.DataLeft = actuallyread;
		while (RUA_DECODER_FEEDME(pInfo->rua, REALMAGICHWL_AUDIO, 0, &Q)!=0) {
			RMuint32 X = REALMAGICHWL_HAPPENING_DECODER_AUDIODMA_HALF;
			{
				// Audio decoder watchdog
				static int histpos=0;
				static int RdPtr[HISTSIZE]={-1,};
				int i,allthesame=1;

				edecPacketsFifo_type pf;
				pf.Type=1; // hack: say AUDIO
				RUA_DECODER_GET_PROPERTY(pInfo->rua, DECODER_SET,edecPacketsFifo,sizeof(edecPacketsFifo_type),&pf);

				histpos=(histpos+1)%HISTSIZE;
				RdPtr[histpos]=pf.Info.RdPtr;

				for (i=0;i<HISTSIZE;i++) {
					if (RdPtr[i]!=RdPtr[0]) allthesame=0;
				}
				if (allthesame) {
					RUA_DECODER_STREAMSTOP(pInfo->rua,REALMAGICHWL_AUDIO);
					RUA_DECODER_STREAMPLAY(pInfo->rua,REALMAGICHWL_AUDIO,0,0);
				}
			}
			RUA_DECODER_WAIT(pInfo->rua,COPYANDQUEUEDATA_TIMEOUT_MICROSEC,&X);
		}
	}

	return 0;
}

static void ApiDemoPanic(RMuint32 reason)
{
	DEBUG_OUT("ApiDemoPanic\n");
	longjmp(g_env,(int)reason);
}

bool GetDiscDetect(const char* device, char *url)
{
	if (!url) return false;
	RMTDisc *disc = (RMTDisc *)NULL;
	RMstatus status = RMFDiscOpen((RMuint8 *) device, &disc);
	strcpy(url, "");
	if(status != RM_OK) {
		printf("Can not open device \"%s\"\n", device);
		return false;
	}
	RMdiscType type = RMFDiscDetect(disc);
	char disc_type[6] = "";
	switch (type){
	case RM_DISC_TYPE_DVD:
		strcpy(disc_type, "DVD");
		break;
	case RM_DISC_TYPE_VCD:
		strcpy(disc_type, "VCD");
		break;
	case RM_DISC_TYPE_SVCD:
		strcpy(disc_type, "SVCD");
		break;
	case RM_DISC_TYPE_CDDA:
		strcpy(disc_type, "CDDA");
		break;
	case RM_DISC_TYPE_ISO9660:
		printf("DISC == ISO 9660\n");
		disc_type[0] = '\0';
		break;
	case RM_DISC_TYPE_UNKNOWN:
		printf("DISC == UNKNOWN\n");
		disc_type[0] = '\0';
		break;
	case RM_NO_DISC:
		printf("NO DISC\n");
		disc_type[0] = '\0';
		break;
	default:
		printf("Wrong returned value\n");
		disc_type[0] = '\0';
		break;
	}
	printf("disc_type=%s\n", disc_type);
	if(disc != (RMTDisc *)NULL)
		RMFDiscClose(disc);
	if(disc_type[0] != '\0') {
		sprintf(url, "DISC@%s://%s", disc_type, device);
		printf("url=%s\n", url);
		return true;
	}
	return false;
}

bool InitInfo(INFO *pInfo)
{
	CHKREG
	memset(pInfo, 0, sizeof(INFO));
	pInfo->PlayStatus    = stStop;
	pInfo->InterFaceCtrl = 0;

	pInfo->volume        = VolumeValue;  // 当前音量,初始音量
	pInfo->maxvolume     = MaxVolume;    // 最大音量
	pInfo->minvolume     = MinVolume;    // 最小音量
	pInfo->Mute          = false;
#ifdef NETPLAYER
	pInfo->PlayVstp      = CreateVstp(3000);
#endif
	pInfo->quit          = false;
	pInfo->lock          = false;
	pInfo->PlaySelect    = psSelected;
	pInfo->KeepSongList  = false;
	pInfo->lampid        = 0;
	pInfo->PlayCancel    = false;
	pInfo->Disc          = NULL;
	pInfo->HaveHW        = 0;
	memset(pInfo->ServerIP, 0, IPLEN);

	if (PlayerStartAudioTrack == trMusic) // 播放器开始的音轨
		pInfo->CurTrack = trMusic;
	else
		pInfo->CurTrack = trSong;

	GetLocalIP(pInfo->IP);
	GetMacAddr(pInfo->MAC);

// 初始化播放信息
	pInfo->pStart = (RMuint8 *)malloc(TOTAL_MEMORY_NEEDED);
	if (pInfo->pStart == NULL) {
		DEBUG_OUT("Cannot allocate %d bytes.\n", TOTAL_MEMORY_NEEDED);
		return false;
	}
	if (setjmp(g_env) != 0) {
		DEBUG_OUT("setjmp error.\n");
		return false;
	}
	pInfo->rua = RUA_OpenDevice(0, TRUE);
	if (pInfo->rua == NULL) {
		fprintf(stderr, "can't AccessDevice\n");
		return false;
	}
	EnterCaribbean(pInfo->pStart, TOTAL_MEMORY_NEEDED, RM_MACHINEALIGNMENT, ApiDemoPanic);
	pInfo->flags = 0;

	pInfo->HaveHW = 1;
	return true;
}

static bool InitVideo(INFO *pInfo) // 初始化视频设置
{
	char url[RM_MAX_STRING];
	RMuint32 val;
	Wnd_type hwnd;

	if (pInfo->InterFaceCtrl != NULL) {
		printf(".............................RMFCloseControlInterface(pInfo->InterFaceCtrl);\n");
		RMFCloseControlInterface(pInfo->InterFaceCtrl);
		pInfo->InterFaceCtrl = NULL;
	}
	if (pInfo->MediaType == mtFILE){
		snprintf(url, RM_MAX_STRING -1, FILEURL, 0, pInfo->PlayingSong.StreamType, pInfo->VideoFile);
	}
	else if (pInfo->MediaType == mtPUSH) {
		sprintf(url, PUSHURL, 0, pInfo->PlayingSong.StreamType, BUFCOUNT, BUFSIZE);
		char URL[512];
		sprintf(URL, "http://%s/playcount?%s", pInfo->ServerIP, pInfo->VideoFile);
//		printf("URL: %s\n", URL);
		offset_t size;
		char* p = url_readbuf(URL, &size);
		if (p) free(p);
	}
	else if (pInfo->MediaType == mtDISC) {
		GetDiscDetect(Cdrom, url);
	}

	DEBUG_OUT("URL: %s\n", url);
	if (pInfo->rua == NULL) return false;
	if (RMFOpenUrlControlInterface(url, &pInfo->InterFaceCtrl, &callback, pInfo) != RM_OK){
		DEBUG_OUT("RMFOpenUrlControlInterface error.\n");
		return false;
	}

	pInfo->type = RMFGetControlInterfaceInputType(pInfo->InterFaceCtrl);
	RMFGetControlHandle((void **) &pInfo->PropCtrl, RM_CONTROL_PROPERTY, pInfo->InterFaceCtrl);

//	SETBLACKFRAME(pInfo->PropCtrl);
	RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evInAspectRatio"   ,evInAspectRatio_4x3);
	RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evOutDisplayOption",evOutDisplayOption_Normal);

	if (strcasecmp(TvType, "PAL") == 0)
		RMFSetAndSavePropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evTvStandard",evTvStandard_PAL);
	else
		RMFSetAndSavePropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evTvStandard",evTvStandard_NTSC);

	RMFGetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evTvStandard", &val);

	hwnd.x = 0;
	hwnd.y = 0;
	if (val == evTvStandard_PAL) {
		hwnd.h = PAL_SCREEN_HEIGHT;
		hwnd.w = PAL_SCREEN_WIDTH;
	}
	else if (val == evTvStandard_NTSC) {
		hwnd.h = NTSC_SCREEN_HEIGHT;
		hwnd.w = NTSC_SCREEN_WIDTH;
	}
	// Apply the settings
	RMFSetGenericProperty(pInfo->PropCtrl, 
			RM_PROPERTY_HWLIB, 
			"VIDEO_SET", "evDestinationWindow",
			&hwnd, sizeof(Wnd_type));

	pInfo->type = RMFGetControlInterfaceInputType(pInfo->InterFaceCtrl);

	switch (pInfo->type){
#ifdef NETPLAYER
		case RM_INPUT_PUSH: // 如果是 PUSH 模式
			sem_init(&pInfo->g_sem, 0, BUFCOUNT);
			RMFGetControlHandle((void **) &pInfo->PushCtrl, RM_CONTROL_PUSH, pInfo->InterFaceCtrl);
			RMFPlayPUSH(pInfo->PushCtrl);
			break;
#endif
		case RM_INPUT_FILE: // 如果是 文件 模式
			RMFGetControlHandle((void **) &pInfo->FileCtrl, RM_CONTROL_FILE, pInfo->InterFaceCtrl);
			RMFPlayFile(pInfo->FileCtrl);
			break;
		case RM_INPUT_CDDA: // 如果是 CDDA 模式
			RMFGetControlHandle((void **) &pInfo->CddaCtrl, RM_CONTROL_CDDA, pInfo->InterFaceCtrl);
			RMFCDDAPlay(pInfo->CddaCtrl);
			break;
		case RM_INPUT_VCD: // 如果是 VCD 模式
			RMFGetControlHandle((void **) &pInfo->VcdCtrl, RM_CONTROL_VCD, pInfo->InterFaceCtrl);
			RMFVCDPlay(pInfo->VcdCtrl);
			break;
		case RM_INPUT_DVD: // 如果是 DVD 模式
			RMFGetControlHandle((void **) &pInfo->DvdCtrl, RM_CONTROL_DVD, pInfo->InterFaceCtrl);
			RMFDVDPlay(pInfo);
			break;
		case RM_INPUT_DISC:
		default:
			break;
	}

	return true;
}

void ExitPlayer(INFO *pInfo) // 关闭播放器
{
	StopPlayer(pInfo);
	if (pInfo->InterFaceCtrl != NULL)
		RMFCloseControlInterface(pInfo->InterFaceCtrl);
	if (ExitCaribbean(NULL)==RM_OK)
		free(pInfo->pStart);
}

void StopPlayer(INFO *pInfo)
{
//	if (pInfo->PlayStatus == stStop) return;
	switch (pInfo->type) {
		case RM_INPUT_FILE:
			RMFStopFile(pInfo->FileCtrl);
			break;
#ifdef NETPLAYER
		case RM_INPUT_PUSH:
			RMFStopPUSH(pInfo->PushCtrl);           // 停止播放
			CloseUrl(pInfo->PlayVstp);              // 关闭流媒体通道
			RMFFlushPUSH(pInfo->PushCtrl, TRUE);    // 播放缓冲区刷新
			sem_destroy(&pInfo->g_sem);
			break;
#endif
		case RM_INPUT_CDDA:
			RMFCDDAStop(pInfo->CddaCtrl);
			RMFCDDAClose(pInfo->CddaCtrl);
			break;
		case RM_INPUT_VCD:
			RMFVCDStop(pInfo->VcdCtrl);
			RMFVCDClose(pInfo->VcdCtrl);
			break;
		case RM_INPUT_DVD:
			RMFDVDStop(pInfo->DvdCtrl);
			break;
		case RM_INPUT_DISC:
		default:
			break;
	}
	if (pInfo->InterFaceCtrl != NULL) {
		RMFCloseControlInterface(pInfo->InterFaceCtrl);
		pInfo->InterFaceCtrl = NULL;
	}
	pInfo->PlayStatus = stStop;                 // 将播放设为停止状态
}

RMTbuffer *GetPushDataBuf(INFO *pInfo)          // 初始化播放信息
{
	RMTbuffer *nogetBuffer;
	sem_wait(&pInfo->g_sem);
	nogetBuffer = (RMTbuffer *) malloc(sizeof(RMTbuffer));
	nogetBuffer->buffer = (RMuint8 *) malloc(BUFSIZE);
	nogetBuffer->bufferSize = BUFSIZE;

	return nogetBuffer;
}

inline void ReplayPlayer(INFO *pInfo) // 重唱
{
	pInfo->KeepSongList = true;
	pInfo->PlayStatus   = stStop;
//	StopPlayer(pInfo);
}

void RunSoundMode(INFO *pInfo, const char *param)
{
	if (!FileExists("/bin/lamp")) return;

	char *cmd = NULL;
	if (param)
		cmd = GetSoundMode(param);
	else {
		char name[20];
		sprintf(name, "%d", pInfo->PlayingSong.SoundMode);
		cmd = GetSoundMode(name);
	}

	if (!cmd) return;

	if (pInfo->lampid > 0) {  // 如果已经创建过进程
		int status;
		if ((waitpid(pInfo->lampid, &status, WNOHANG)) == 0) {
			int retval = kill(pInfo->lampid, SIGTERM);
			if (retval) {
				waitpid(pInfo->lampid,&status, WNOHANG);
			} else
				waitpid(pInfo->lampid,&status, 0);
		}
		pInfo->lampid = -1;
	}
	pInfo->lampid = fork();
	if (pInfo->lampid == 0)
		execlp("/bin/lamp", "lamp", "-c", cmd, NULL);
}

bool StartPlayer(INFO *pInfo)
{
	if (pInfo->lock == true)
		return false;
	pInfo->KeepSongList = false;       // 取消重唱状态
#ifdef NETPLAYER
	if (pInfo->MediaType == mtPUSH) {  // 如果不是本地歌曲
		if (OpenUrl(pInfo->PlayVstp, pInfo->VideoFile)) {
			DEBUG_OUT("OpenUrl Error (%s).\n", pInfo->VideoFile);
			return false;
		}
	}
#endif
	if (!InitVideo(pInfo)) {
		DEBUG_OUT("InitVideo Error.\n");
		return false;
	}
	pInfo->PlayCancel = false;
	pInfo->PlayStatus = stPlaying;
	pInfo->flags = 0;
	pInfo->StartGetStream = true;
	if (EnabledSound)
		RunSoundMode(pInfo, NULL);
	SetAudioChannel(pInfo);

	return true;
}

void PausePlayer(INFO *pInfo)
{
	switch (pInfo->type){
		case RM_INPUT_FILE:
			RMFPauseFile(pInfo->FileCtrl);
			break;
#ifdef NETPLAYER
		case RM_INPUT_PUSH:
			RMFPausePUSH(pInfo->PushCtrl);
			break;
#endif
		case RM_INPUT_VCD:
			RMFVCDPause(pInfo->VcdCtrl);
			break;
		case RM_INPUT_CDDA:
			RMFCDDAPause(pInfo->CddaCtrl);
			break;
		case RM_INPUT_DVD:
			RMFDVDPause_Off(pInfo->DvdCtrl);
			break;
		case RM_INPUT_DISC:
		default:
			return;
	}
	pInfo->PlayStatus = stPause;
}

void ContinuePlayer(INFO *pInfo)
{
	switch (pInfo->type) {
		case RM_INPUT_FILE:
			RMFPlayFile(pInfo->FileCtrl);
			break;
#ifdef NETPLAYER		
		case RM_INPUT_PUSH:
			RMFPlayPUSH(pInfo->PushCtrl);
			break;
#endif
		case RM_INPUT_VCD:
			RMFVCDPlay(pInfo->VcdCtrl);
			break;
		case RM_INPUT_CDDA:
			RMFCDDAPlay(pInfo->CddaCtrl);
			break;
		case RM_INPUT_DVD:
			RMFDVDPause_On(pInfo->DvdCtrl);
			break;
		case RM_INPUT_DISC:
		default:
			return;
	}		
	pInfo->PlayStatus = stPlaying;
}

PlayerState PauseContinuePlayer(INFO *pInfo)
{
	if (pInfo->PlayStatus == stPlaying)
		PausePlayer(pInfo);
	else if (pInfo->PlayStatus == stPause)
		ContinuePlayer(pInfo);

	return pInfo->PlayStatus;
}

static void callback(RMTcontrolInterface ctrl, void *userData, RMmessage message, RMuint32 val)
{
	INFO *tmpInfo = userData;
	RMTbuffer *buffer;

	switch (message) {
		case RM_MESSAGE_RMTBUFFER_FREE:    // 释放
			buffer = (RMTbuffer *) val;
			free(buffer->buffer);
			free(buffer);
			sem_post(&tmpInfo->g_sem);
			break;
		case RM_MESSAGE_EOS:               // 播放完毕信号
			if (ShowJpeg(tmpInfo->rua, Background) == false)
				SETBLACKFRAME(tmpInfo->PropCtrl);
#if 0
			if (Advertising){ // 如果两首歌曲显示广告图片
				const char *tmpfile="/ktvdata/adtmp.jpg";
				long len = DownloadToFile("", ADCACHEPATH, tmpfile, true); // 随机下载图片
				if (len > 0)
					ShowJpeg(tmpInfo->rua, tmpfile);
			}
#endif
			tmpInfo->PlayStatus = stStop;
			break;
		case RM_MESSAGE_DEMUX_DROP_DATA:
			RMFFlushPUSH(tmpInfo->PushCtrl, TRUE);
			break;
		case RM_MESSAGE_FRAME_INDEX:
//			printf("RM_MESSAGE_FRAME_INDEX: tmpInfo->StartGetStream=%d\n", tmpInfo->StartGetStream);
			if (tmpInfo->StartGetStream) {
				if (strcasecmp(tmpInfo->PlayingSong.StreamType, "DIVX") == 0)
					tmpInfo->StartGetStream = false;
				else
					tmpInfo->StartGetStream = ReadSongTrack(tmpInfo) < 1;
//				printf("%s: tmpInfo->StartGetStream=%d\n", __FUNCTION__, tmpInfo->StartGetStream);
				if (!tmpInfo->StartGetStream) {
					PRINTTRACK(tmpInfo->CurTrack);
					SetAudioChannel(tmpInfo);
				}
			}
			break;
		default:
//			DEBUG_OUT("message=%d, val=%ld\n", message, val);
			break;
	}
}

