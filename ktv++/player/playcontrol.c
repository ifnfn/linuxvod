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
#include "songdata.h"
#include "selected.h"
#include "strext.h"

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

const char *PUSHURL = "[%d]PUSH@%s://#buffercount=%d,buffersize=%d,bufferallocation=external/";
const char *FILEURL = "[%d]FILE@%s://%s";

static void callback(RMTcontrolInterface ctrl, void *userData, RMmessage message, RMuint32 val);

static int ReadSongTrack(INFO *pInfo);
static jmp_buf g_env;

void convertToTime(RMuint32 totalSeconds, RMuint32 *hours, RMuint32 *minutes, RMuint32 *seconds)
{
	*seconds = (RMuint32) (totalSeconds % 60);
	*minutes = (RMuint32) (totalSeconds/60);
	*hours   = (RMuint32) (*minutes/60);
	*minutes = (RMuint32) (*minutes%60);
}

void DeleteFirstSongList(void)
{
	if (SelectedList.count > 0){
		BroadcastSongRec("delsong=", SelectedList.items + 0);
		DelSongIndex(0);
	}
}

int GetRealVolume(INFO *pInfo)
{
	int EquVolume;
	return  pInfo->volume;
	if (pInfo->CurTrack == MUSICTRACK)
		EquVolume = pInfo->PlayingSong.VolumeK;
	else
		EquVolume = pInfo->PlayingSong.VolumeS;

//	DEBUG_OUT("EquVolume=%d\n", EquVolume);
	int tmp = pInfo->volume * (1 - (EquVolume - pInfo->volume) / 100.0);
	if (tmp > pInfo->maxvolume)
		tmp = pInfo->maxvolume;
	else if (tmp < pInfo->minvolume)
		tmp = pInfo->minvolume;
//	DEBUG_OUT("vol =%d, pInfo->volume=%d, RealVolume=%d\n", EquVolume, pInfo->volume, tmp);
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
				char tmp[100];
				strcpy(tmp, p);
				char *code= strtok(tmp, ",");
				char *ext = strtok(NULL, ",");
				if (ext == NULL) ext = "VOB";

				strcpy(pInfo->PlayingSong.SongCode  , code);
				strcpy(pInfo->PlayingSong.StreamType, ext );
			}else {
				DEBUG_OUT("ShowJpeg %s\n", Background);
				ShowJpeg(pInfo->rua, Background);
				NoSongLock();
				return false;
			}
			PRINTTRACK(SongStartAudioTrack);
			if (DefaultAudioTrack == trMusic)
				pInfo->CurTrack = MUSICTRACK;
			else if (DefaultAudioTrack == trSong)
				pInfo->CurTrack = SOUNDTRACK;
			pInfo->PlayingSong.SoundMode = DefaultSoundMode; // 灯光控制模式
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
			pInfo->PlayingSong.SoundMode = HiSoundMode; // 灯光控制模式
		}else
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
		pInfo->PlayingSong.SoundMode = Fire119SoundMode; // 灯光控制模式
	}

	char *tmpfile = GetLocalFile(pInfo->PlayingSong.SongCode, NULL); // 读取本地文件
	if (tmpfile) // 如果本地文件存在
		strcpy(pInfo->VideoFile, tmpfile);
	else
		sprintf(pInfo->VideoFile, "code=%s", pInfo->PlayingSong.SongCode);
//	printf("pInfo->VideoFile = %s\n", pInfo->VideoFile);
	pInfo->LocalFile = tmpfile != NULL;
	BroadcastSongRec("playsong=", &pInfo->PlayingSong);
	return StartPlayer(pInfo);
}

void MuteSwitchPlayer(INFO *pInfo)
{
	if (pInfo->Mute) {
		pInfo->Mute = false;
		AddVolume(pInfo, 0);
	}
	else {
		pInfo->Mute = true;
		if (pInfo->rua) {
			RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeRight", 0);
			RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "AUDIO_SET", "eaVolumeLeft",  0);
		}
	}
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
			RUA_DECODER_SET_PROPERTY(pInfo->rua, AUDIO_SET, eAudioMode, sizeof(audiomode), &audiomode);
		}
		else {
			tmpstream = pInfo->PlayingSong.Klok>='0' ? pInfo->PlayingSong.Klok - '0' : pInfo->PlayingSong.Klok;

			if (strcasecmp(pInfo->PlayingSong.StreamType, "DIVX") == 0) {
				RMFSetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL,
					 		"FILE", "SELECT_AUDIO_STREAM", &tmpstream, sizeof(RMint32));
//				DEBUG_OUT("MUSICTRACK DIVX =%ld\n", tmpstream);
			}
			else {
				if (tmpstream < pInfo->MaxAudioStreamNum)
					RMFSetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL, "DEMUX", "AudioPID",
							&pInfo->audioStreams[tmpstream], sizeof(RMstreamId));
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
			tmpstream = pInfo->PlayingSong.Sound>='0'?pInfo->PlayingSong.Sound - '0':pInfo->PlayingSong.Sound;
			if (strcasecmp(pInfo->PlayingSong.StreamType, "DIVX") == 0) {
				RMFSetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL,
					"FILE", "SELECT_AUDIO_STREAM", &tmpstream, sizeof(RMint32));
			}
			else {
				if (tmpstream < pInfo->MaxAudioStreamNum)
					RMFSetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_EXTERNAL, "DEMUX", "AudioPID",
						&pInfo->audioStreams[tmpstream], sizeof(RMstreamId));
			}
		}
	}
	AddVolume(pInfo, 0);
}

int PlayAudio(INFO *pInfo, char *type, RMuint32 samplerate, RMuint32 numberofchannels, RMuint32 numberofbitspersample, char *file)
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

		Q.pData=buf;
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

bool InitInfo(INFO *pInfo)
{
	memset(pInfo, 0, sizeof(INFO));
	pInfo->PlayStatus    = stStop;
	pInfo->InterFaceCtrl = 0;

	pInfo->volume        = VolumeValue;  // 当前音量,初始音量
	pInfo->maxvolume     = MaxVolume;    // 最大音量
	pInfo->minvolume     = MinVolume;    // 最小音量
	pInfo->Mute          = false;
	pInfo->PlayVstp      = CreateVstp(3000);
	pInfo->quit          = false;
	pInfo->lock          = false;
	pInfo->PlaySelect    = psSelected;
	pInfo->KeepSongList  = false;
	pInfo->lampid        = 0;
	pInfo->PlayCancel    = false;
	memset(pInfo->ServerIP, 0, IPLEN);

	if (PlayerStartAudioTrack == trMusic) // 播放器开始的音轨
		pInfo->CurTrack  = trMusic;
	else
		pInfo->CurTrack  = trSong;

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
		FILE *fp = fopen("/var/run/player", "w");
		if (fp) {
			fputs("没有找到解压卡", fp);
			fclose(fp);
		}
		fprintf(stderr, "can't AccessDevice\n");
		return false;
	}
	EnterCaribbean(pInfo->pStart, TOTAL_MEMORY_NEEDED, RM_MACHINEALIGNMENT, ApiDemoPanic);
	pInfo->flags = 0;
	return true;
}

static bool InitVideo(INFO *pInfo) // 初始化视频设置
{
	char url[RM_MAX_STRING];
	RMuint32 val;
	Wnd_type hwnd;

	if (pInfo->InterFaceCtrl) {
		RMFCloseControlInterface(pInfo->InterFaceCtrl);
		pInfo->InterFaceCtrl = 0;
	}
	if (pInfo->LocalFile)
		sprintf(url, FILEURL, 0, pInfo->PlayingSong.StreamType, pInfo->VideoFile);
	else {
		sprintf(url, PUSHURL, 0, pInfo->PlayingSong.StreamType, BUFCOUNT, BUFSIZE);
		char URL[512];
		sprintf(URL, "http://%s/playcount?%s", pInfo->ServerIP, pInfo->VideoFile);
//		printf("URL: %s\n", URL);
		offset_t size;
		char* p = url_readbuf(URL, &size);
		if (p) free(p);
	}

	DEBUG_OUT("URL: %s\n", url);
	if (pInfo->rua == NULL) return false;
	if (RMFOpenUrlControlInterface(url, &pInfo->InterFaceCtrl, &callback, pInfo) != RM_OK){
		DEBUG_OUT("RMFOpenUrlControlInterface error.\n");
		return false;
	}
	RMFGetControlHandle((void **) &pInfo->PropCtrl, RM_CONTROL_PROPERTY, pInfo->InterFaceCtrl);
//	SETBLACKFRAME(pInfo->PropCtrl);
	RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evInAspectRatio"   ,evInAspectRatio_4x3);
	RMFSetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evOutDisplayOption",evOutDisplayOption_Normal);

	if (strcasecmp(TvType, "PAL") == 0)
		RMFSetAndSavePropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB,"VIDEO_SET", "evTvStandard",evTvStandard_PAL);
	else
		RMFSetAndSavePropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB,"VIDEO_SET", "evTvStandard",evTvStandard_NTSC);


	RMFGetPropertyValue(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET", "evTvStandard", &val);

	hwnd.x=0;
	hwnd.y=0;
	if (val == evTvStandard_PAL) {
		hwnd.h = PAL_SCREEN_HEIGHT;
		hwnd.w = PAL_SCREEN_WIDTH;
	}
	else if (val == evTvStandard_NTSC) {
		hwnd.h = NTSC_SCREEN_HEIGHT;
		hwnd.w = NTSC_SCREEN_WIDTH;
	}
	// Apply the settings
	RMFSetGenericProperty(pInfo->PropCtrl, RM_PROPERTY_HWLIB, "VIDEO_SET",
		"evDestinationWindow", &hwnd, sizeof(Wnd_type));
//	RUA_DECODER_SET_PROPERTY(pInfo->rua,
//		OSD_SET, eOsdDestinationWindow, sizeof(Wnd_type), &hwnd);

	pInfo->type = RMFGetControlInterfaceInputType(pInfo->InterFaceCtrl);

	if (pInfo->type == RM_INPUT_PUSH){       // 如果是 PUSH 模式
		sem_init(&pInfo->g_sem, 0, BUFCOUNT);
		RMFGetControlHandle((void **) &pInfo->PushCtrl, RM_CONTROL_PUSH, pInfo->InterFaceCtrl);
		RMFPlayPUSH(pInfo->PushCtrl);
	}
	else if (pInfo->type == RM_INPUT_FILE) { // 如果是 文件 模式
		RMFGetControlHandle((void **) &pInfo->FileCtrl, RM_CONTROL_FILE, pInfo->InterFaceCtrl);
		RMFPlayFile(pInfo->FileCtrl);
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
	if (pInfo->type == RM_INPUT_FILE) {
		RMFStopFile(pInfo->FileCtrl);
	}
	else if (pInfo->type == RM_INPUT_PUSH){
		RMFStopPUSH(pInfo->PushCtrl);           // 停止播放
		CloseUrl(pInfo->PlayVstp);              // 关闭流媒体通道
		RMFFlushPUSH(pInfo->PushCtrl, TRUE);    // 播放缓冲区刷新
		sem_destroy(&pInfo->g_sem);
	}
	pInfo->PlayStatus = stStop;                 // 将播放设为停止状态
}

#if 1
RMTbuffer *GetPushDataBuf(INFO *pInfo)          // 初始化播放信息
{
	RMTbuffer *nogetBuffer;
	sem_wait(&pInfo->g_sem);
	nogetBuffer = (RMTbuffer *) malloc(sizeof(RMTbuffer));
	nogetBuffer->buffer = (RMuint8 *) malloc(BUFSIZE);
	nogetBuffer->bufferSize = BUFSIZE;
	return nogetBuffer;
}
#endif

inline void ReplayPlayer(INFO *pInfo) // 重唱
{
	pInfo->KeepSongList = true;
	pInfo->PlayStatus   = stStop;
//	StopPlayer(pInfo);
}

void RunSoundMode(INFO *pInfo, char *param)
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
	if (!pInfo->LocalFile) {           // 如果不是本地歌曲
		if (OpenUrl(pInfo->PlayVstp, pInfo->VideoFile)) {
			DEBUG_OUT("OpenUrl Error (%s).\n", pInfo->VideoFile);
			return false;
		}
	}
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
	AddVolume(pInfo, 0);
	return true;
}

PlayState PauseContinuePlayer(INFO *pInfo)
{
	if (pInfo->PlayStatus == stPlaying){
		if (pInfo->type == RM_INPUT_FILE)
			RMFPauseFile(pInfo->FileCtrl);
		else if (pInfo->type == RM_INPUT_PUSH)
			RMFPausePUSH(pInfo->PushCtrl);
		pInfo->PlayStatus = stPause;
	}
	else if (pInfo->PlayStatus == stPause){
		if (pInfo->type == RM_INPUT_FILE)
			RMFPlayFile(pInfo->FileCtrl);
		else if (pInfo->type == RM_INPUT_PUSH)
			RMFPlayPUSH(pInfo->PushCtrl);
		pInfo->PlayStatus = stPlaying;
	}
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
			SETBLACKFRAME(tmpInfo->PropCtrl);
//			ShowJpeg(tmpInfo->rua, Background);
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
		case RM_MESSAGE_DISPLAY_TIME:
		case RM_MESSAGE_AUDIO_RATE:
		case RM_MESSAGE_VIDEO_RATE:
		case RM_MESSAGE_AVG_VIDEO_RATE:
		case RM_MESSAGE_RATE:
		case RM_MESSAGE_ASPECTRATIOCHANGE:
		case RM_MESSAGE_ACMODECHANGE:
			break;
		case RM_MESSAGE_FRAME_INDEX:
			if (tmpInfo->StartGetStream) {
				if (strcasecmp(tmpInfo->PlayingSong.StreamType, "DIVX") == 0)
					tmpInfo->StartGetStream = false;
				else
					tmpInfo->StartGetStream = ReadSongTrack(tmpInfo) < 1;
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
