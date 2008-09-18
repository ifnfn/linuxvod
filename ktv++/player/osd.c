#define ALLOW_OS_CODE
#include "osd.h"
#include <string.h>

static RUA_handle rua;
static RMKERNELOSDBUFDimensions_type dim;
static char *OsdMap = NULL;

static void SetPalette( int i, char r, char g, char b, char alpha);
static SDL_Surface* TTF2Surface(TTF_Font *font, const char *text);
static uint32_t GetSurfacePixel(SDL_Surface *surface, int x, int y);

#define SetBackColor(r, g, b, a )    SetPalette(1, r, g, b, a)
#define SetFrontColor(r, g, b, a )   SetPalette(2, r, g, b, a)
#define SetCurrentColor(r, g, b, a ) SetPalette(0, r, g, b, a)

#define OSDMAXTHREAD 10
static ThreadList *ThreadHeader[10] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static void OSD_CleanRect(int x, int y, int w, int h)
{
	int i, j;
	for (i=x; i<x+w; i++)
		for (j=y; j<y+h; j++)
			OSD_SetPixel(OsdMap, &dim, i, j, 1);
}

void OSD_CleanScreen(void)
{
	SetBackColor(0, 0, 0, 0);
	OSD_CleanRect(0, 0, dim.width, dim.height);
}

static void FreeScrollText(ScrollText *scroll)
{
	if (scroll->Surface) {
		SDL_FreeSurface(scroll->Surface);
		scroll->Surface = NULL;
	}
	free(scroll);
}

/**
Param	pos_x: start to Postition Show
Param	x: Show Pos Point.x
Param	y: Show Pos Point.y
Param	pFontObj: Showt Font Object Handle
Param   return void
*/
static void ShowSurface(int pos_x,int x, int y, SDL_Surface *pFontObj)
{
	if(NULL == pFontObj)
		return ;
	int i, j, color;
	for (i= pos_x ; i<pFontObj->w; i++){
		for (j= 0; j<pFontObj->h; j++){
			color = GetSurfacePixel(pFontObj, i, j) == 0xFF? 1: 2;
			OSD_SetPixel(OsdMap, &dim, x + i, y + j, color);
		}
	}
}

static void SetPalette(int i, char r, char g, char b, char alpha)
{
	kernelproperty Kp;
	RMKERNELOSDBUFPaletteEntry_type color;

	color.i = i;
	color.R = r;
	color.G = g;
	color.B = b;
	color.alpha = alpha;

	Kp.kernelpropid_value = PROPID_KERNEL_OSDBUF_PaletteEntry;
	Kp.PropTypeLength = sizeof(RMKERNELOSDBUFPaletteEntry_type);
	Kp.pValue = &color;

	RUA_KERNEL_SET_PROPERTY(rua, &Kp);
}

static void StrToScrollText(ScrollText *scrolltext, const char *text)
{
	if (scrolltext == NULL) return;
	char *cmdstr = strdup(text);
	cmdstr = strtok(cmdstr, "&");
	char *cmd = NULL, *value = NULL;
	while(cmdstr){
		cmd = cmdstr;
		value = strchr(cmdstr, '=');
		if (value) {
			*value = '\0';
			value++;
//			printf("cmd=%s, value=%s\n", cmd, value);
			if (strcmp(cmd, "speed") == 0)
				scrolltext->Speed = atoi(value);
			else if (strcasecmp(cmd, "x") == 0)
				scrolltext->x = atoi(value);
			else if (strcasecmp(cmd, "y") == 0)
				scrolltext->y = atoi(value);
			else if (strcasecmp(cmd, "count") == 0)
				scrolltext->Count = atoi(value);
			else if (strcasecmp(cmd, "size") == 0)
				scrolltext->FontSize = atoi(value);
			else if (strcasecmp(cmd, "background") == 0){
				sscanf(value, "%hhu,%hhu,%hhu,%hhu", &scrolltext->GroundColor.R,
					&scrolltext->GroundColor.G, &scrolltext->GroundColor.B,
					&scrolltext->GroundColor.A);
			}
			else if (strcasecmp(cmd, "foreground") == 0){
				sscanf(value, "%hhu,%hhu,%hhu,%hhu", &scrolltext->FrontColor.R,
					&scrolltext->FrontColor.G, &scrolltext->FrontColor.B,
					&scrolltext->FrontColor.A);
			}
			else if (strcasecmp(cmd, "text") == 0)
				strcpy(scrolltext->Text, value);
			else if (strcasecmp(cmd, "timeout") == 0)
				scrolltext->Timeout = atoi(value);
		}
		cmdstr = strtok(NULL, "&");
	}
	free(cmdstr);
}

int CreateScrollTextStr(int id, const char *text)
{
	if (ThreadHeader[id] == NULL) return -1;

	ScrollText *tmpScrollText = (ScrollText*)malloc(sizeof(ScrollText));
	memset(tmpScrollText, 0, sizeof(ScrollText));

	tmpScrollText->x          = ThreadHeader[id]->x;
	tmpScrollText->y          = ThreadHeader[id]->y;
	tmpScrollText->Count      = ThreadHeader[id]->Count;
	tmpScrollText->Speed      = ThreadHeader[id]->Speed;
	tmpScrollText->FontSize   = ThreadHeader[id]->FontSize;
	tmpScrollText->Timeout    = ThreadHeader[id]->Timeout;
	tmpScrollText->Clear      = ThreadHeader[id]->Clear;
	tmpScrollText->FrontColor = ThreadHeader[id]->FrontColor;
	tmpScrollText->GroundColor= ThreadHeader[id]->GroundColor;
	tmpScrollText->Running    = 1;
	tmpScrollText->Next       = NULL;

	StrToScrollText(tmpScrollText, text);

	TTF_Init();
	TTF_Font *font = TTF_OpenFont("/ktvdata/chinese.ttf", tmpScrollText->FontSize);
	if (font) {
		tmpScrollText->Surface = TTF2Surface(font, tmpScrollText->Text);
		TTF_CloseFont(font);
	}
	TTF_Quit();
	if( tmpScrollText->Surface == NULL){
		printf("Create Surface Error\n");
		return -1;
	}
	tmpScrollText->h = tmpScrollText->Surface->h;
	tmpScrollText->w = dim.width;

	pthread_mutex_lock(&ThreadHeader[id]->mutex);
	if (ThreadHeader[id]->LastScroll)
		ThreadHeader[id]->LastScroll->Next = tmpScrollText;
	ThreadHeader[id]->LastScroll = tmpScrollText;
	if (ThreadHeader[id]->Scroll == NULL)
		ThreadHeader[id]->Scroll = tmpScrollText;
	pthread_mutex_unlock(&ThreadHeader[id]->mutex);
	sem_post(&ThreadHeader[id]->g_sem);
	return id;
}

static SDL_Surface* TTF2Surface(TTF_Font *font, const char *text)
{
#define BUF_LEN 1024
	if (strcmp(text, "") == 0) return NULL;
	char cBuf[BUF_LEN];
	memset(cBuf, '\0', BUF_LEN);

	size_t OutByteSize = Unicode("GBK", text, cBuf, BUF_LEN);

	if (OutByteSize <= 0) {
		return NULL;
	}
	SDL_Color fontcolor;
	fontcolor.r = 0;
	fontcolor.g = 0;
	fontcolor.b = 0xFF;

	return TTF_RenderUNICODE_Blended(font, (Uint16 *)cBuf, fontcolor);
}

static uint32_t GetSurfacePixel(SDL_Surface *surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			return *p;
		case 2:
			return *(Uint16 *)p;
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
		case 4:
			return *(uint32_t *)p;
		default:
			return 0;  /* shouldn't happen, but avoids warnings */
	}
}

int OSD_Create(int cardid)
{
	rua = RUA_OpenDevice(cardid, TRUE);

	if (rua == NULL){
		fprintf(stderr, "can't AccessDevice\n");
		return -1;
	}
	kernelproperty Kp = {
		kernelpropid_value: PROPID_KERNEL_OSDBUF_Dimensions,
		PropTypeLength: sizeof(RMKERNELOSDBUFDimensions_type),
		pValue: &dim
	};
	if (RUA_KERNEL_GET_PROPERTY(rua, &Kp)!=0) {
		fprintf(stderr,"osdbuf not enabled on this board\n");
		return -1;
	}

	OsdMap= mmap(NULL,
		dim.width*dim.height / ( 8 / dim.color_depth), PROT_READ| PROT_WRITE,
		MAP_SHARED, (int)rua, REALMAGICHWL_MMAP_OSDBUF);

	if ((int)OsdMap==-1) {
		fprintf(stderr,"Cannot mmap()\n");
		return -1;
	}
	OSD_CleanScreen();
	return 0;
}

void OSD_Quit(void)
{
	int i;
	for (i=0; i<OSDMAXTHREAD; i++)
		if ( ThreadHeader[i] )
			ThreadHeader[i]->Running = 0;
	munmap(OsdMap, dim.width*dim.height/(8/dim.color_depth));
	RUA_ReleaseDevice(rua);
}

static void* ScrollTextRun(void *pObj)
{
	ScrollText *pScrollText = (ScrollText *)pObj;
	if( pScrollText ==NULL) return NULL;

	SetFrontColor(pScrollText->FrontColor.R, pScrollText->FrontColor.G,
		pScrollText->FrontColor.B, pScrollText->FrontColor.A);
	SetBackColor(pScrollText->GroundColor.R, pScrollText->GroundColor.G,
		pScrollText->GroundColor.B, pScrollText->GroundColor.A);
	SetCurrentColor(pScrollText->GroundColor.R, pScrollText->GroundColor.G, pScrollText->GroundColor.R, 0);

	if (pScrollText->Clear == 1) OSD_CleanScreen();
	int x = pScrollText->x, y = pScrollText->y;
	int pos = 0;
	ShowSurface(pos, x, y, pScrollText->Surface);
	struct timeval start, firsttime;
	gettimeofday(&start, NULL);
	firsttime = start;
	while (pScrollText->Running)
	{
		int32_t ticks;
		int32_t startticks;
		struct timeval now;
		gettimeofday(&now, NULL);
		ticks = (now.tv_sec-start.tv_sec) * 1000 + (now.tv_usec-start.tv_usec) / 1000;
		start = now;
		if (pScrollText->Timeout != -1) {
			startticks = (now.tv_sec-firsttime.tv_sec) * 1000 + (now.tv_usec-firsttime.tv_usec) / 1000;
			if (startticks > pScrollText->Timeout)
				break;
		}
		usleep(500 - ticks);
		if (pScrollText->Speed > 0){
			if(x<0){
				pos += pScrollText->Speed;
				if(pos >= pScrollText->Surface->w){
					pos = 0;
					x = pScrollText->x;
					pScrollText->Count--;
					if(!pScrollText->Count){
						break;
					}
				}
			}
			x -= pScrollText->Speed;
			ShowSurface(pos, x, y, pScrollText->Surface);
		}
	}
	pScrollText->Running = 0;
	if (pScrollText->Clear == 1) OSD_CleanScreen();
	return NULL;
}

#ifdef DEBUG
static void PrintTextList(ThreadList *node)
{
	ScrollText *tmp = node->Scroll;
	while (tmp){
		printf("Text: %s\n", tmp->Text);
		tmp = tmp->Next;
	}
}
#endif

static void *ScrollTextAlwayThread(void *data)
{
	ThreadList *node = (ThreadList*) data;
	if (node == NULL) return NULL;
	while(node->Running == 1) {
		sem_wait(&node->g_sem);
#ifdef DEBUG
		PrintTextList(node);
#endif
		if (node->Scroll) {
			pthread_mutex_lock(&node->mutex);
			node->WorkScroll = node->Scroll;
			node->WorkScroll->Next = NULL;
			node->Scroll = node->Scroll->Next;
			if (node->Scroll == NULL)
				node->LastScroll = NULL;
			pthread_mutex_unlock(&node->mutex);
			ScrollTextRun (node->WorkScroll);
			FreeScrollText(node->WorkScroll);
			node->WorkScroll = NULL;
		}
	}
	return NULL;
}

int CreateThreadList(const char *text)
{
	int i;
	for (i=0; i<OSDMAXTHREAD; i++)
		if (ThreadHeader[i] == NULL)
			break;
	if (i >= OSDMAXTHREAD) return -1;
	ThreadHeader[i] = (ThreadList*)malloc(sizeof(ThreadList));
	if (ThreadHeader[i] == NULL) return -1;


	ThreadList *head = ThreadHeader[i];
	memset(head, 0, sizeof(ThreadList));
	ScrollText tmpScrollText = {
		x: 0,
		y: 0,
		Count: 1,
		Speed: 1,
		FontSize: 36,
		Timeout: -1,
		Clear: 1,
	};
	StrToScrollText(&tmpScrollText, text);
	head->x          = tmpScrollText.x;
	head->y          = tmpScrollText.y;
	head->FrontColor = tmpScrollText.FrontColor;
	head->GroundColor= tmpScrollText.GroundColor;
	head->FontSize   = tmpScrollText.FontSize;
	head->Count      = tmpScrollText.Count;
	head->Speed      = tmpScrollText.Speed;
	head->Timeout    = tmpScrollText.Timeout;
	head->Clear      = tmpScrollText.Clear;
	head->Running    = 1;

	head->Scroll = head->WorkScroll = head->LastScroll = NULL;
	sem_init(&head->g_sem, 0, 0);
	pthread_mutex_init(&head->mutex, NULL);
	pthread_create(&head->WorkThread, NULL, &ScrollTextAlwayThread, (void *)head);
	return i;
}

void CleanScrollTextList(int id)
{
	if (ThreadHeader[id] == NULL)
		return;

	pthread_mutex_lock(&ThreadHeader[id]->mutex);
	while (ThreadHeader[id]->Scroll){
		free(ThreadHeader[id]->Scroll);
		ThreadHeader[id]->Scroll = ThreadHeader[id]->Scroll->Next;
	}
	pthread_mutex_unlock(&ThreadHeader[id]->mutex);
	if (ThreadHeader[id]->WorkScroll)
		ThreadHeader[id]->WorkScroll->Running = 0;
}
