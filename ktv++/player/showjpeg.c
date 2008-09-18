/*****************************************
 Copyright ?2001-2003
 Sigma Designs, Inc. All Rights Reserved
 Proprietary and Confidential
 *****************************************/
#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>
#include <stdlib.h>
#include <unistd.h>

#define ALLOW_OS_CODE 1
#include "showjpeg.h"
#include "strext.h"

#define FRAME_WIDTH (720)
#define FRAME_HEIGHT (480)
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT)
static unsigned char Yr[FRAME_SIZE];
static unsigned char UVr[FRAME_SIZE/2];

static unsigned char Y[FRAME_SIZE];
static unsigned char UV[FRAME_SIZE/2];


static void setPAL(RUA_handle h)
{
	printf("Setting PAL\n");
	// set SAA7114 to PAL.
	{
		evdTvStandard_type evdTvStandard_value = evTvStandard_PAL;
		RUA_DECODER_SET_PROPERTY(h,
			VIDEO_DECODER_SET,
			evdTvStandard,
			sizeof(evdTvStandard_type),
			&evdTvStandard_value);
	}

	// set SM2288 to ``PAL'' input (at output of SAA7114, PAL frames or PAL frames are just the same thing)
/*
	encoderproperty Ep={0,};
	{
		RMENCODERVIDEOStandard_type RMENCODERVIDEOStandard_value=PARAM_H_VIDEO_STANDARD_PAL;
		Ep.encoderpropid_value=PROPID_ENCODER_VIDEO_Standard;
		Ep.PropTypeLength=sizeof(RMENCODERVIDEOStandard_type);
		Ep.pValue=&RMENCODERVIDEOStandard_value;

		RUA_ENCODER_SET_PROPERTY(h,&Ep);
	}
*/
}

bool ShowJpeg(RUA_handle h, const char *file)
{
	if (!FileExists(file)) return false;
	RMKERNELGENERICYuvFrame_type f;
	Wnd_type w;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *jpegfile;
	JSAMPARRAY scanline;
	int nb_lines_buffer, nb_lines_read, line_size;
	int i, j, scale;
	unsigned int k;
	signed int xoffset, yoffset;
	kernelproperty Kp;

	// clear Y buffer (all black);
	for(i=0; i<FRAME_SIZE; i++)
		Yr[i]=Y[i]=0;
	for(i=0; i<FRAME_SIZE/2; i++)
		UVr[i]=UV[i]=0;

	jpegfile = fopen(file,"rb");
	if(jpegfile == NULL)
		return false;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo,jpegfile);

	jpeg_read_header(&cinfo,TRUE);

	/* set decompression parameters here */
	cinfo.out_color_space = JCS_YCbCr;
	cinfo.dct_method = JDCT_IFAST;
	cinfo.scale_num = 1;
	scale = 0;

	if(scale == 0) { // autoscale
		scale = 1;
		while( scale < 8 && ((cinfo.image_width / scale ) > 720 || (cinfo.image_height / scale ) > 480 )) {
			scale = scale * 2;
		}
	}

	cinfo.scale_denom = scale;
	cinfo.quantize_colors = FALSE;
	jpeg_start_decompress(&cinfo);
	line_size = cinfo.output_width * cinfo.output_components;
	nb_lines_buffer = 1;

	f.x=0;
	f.y=0;
	f.w=cinfo.output_width;
	f.h=cinfo.output_height;
	f.pY=Y;
	f.pUV=UV;

	xoffset=0; // (((int)(FRAME_WIDTH-cinfo.output_width))/4)*2;
	yoffset=0; // ((int)(FRAME_HEIGHT-cinfo.output_height))/2;

	scanline = (JSAMPARRAY) malloc(nb_lines_buffer * sizeof(JSAMPROW));
	for(i=0; i< nb_lines_buffer; i++)
		scanline[i] = (JSAMPROW) malloc(line_size * sizeof(JSAMPLE));

	while(cinfo.output_scanline < cinfo.output_height) {
		nb_lines_read = jpeg_read_scanlines(&cinfo,scanline,nb_lines_buffer);
		for(i=0; i< nb_lines_read; i++) {
			j=cinfo.output_scanline+i-nb_lines_read;
			for(k=0;k<f.w;k++) {
				Y[j*f.w+k] = scanline[i][k*3];
				if(((j&1)==0) && ((k&1)==0)){
					UV[(j/2*f.w)+k] = scanline[i][k*3+1]; // +scanline[i][(k+1)*3+1];
					UV[(j/2*f.w)+k+1] = scanline[i][k*3+2]; // +scanline[i][(k+1)*3+2];
				}
			}
		}
	}

	for(i=0; i< nb_lines_buffer; i++)
	{
		free(scanline[i]);
	}
	free(scanline);

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(jpegfile);

	RUA_DECODER_RESET(h);

	w.x=f.x;
	w.y=f.y;
	w.w=f.w;
	w.h=f.h;

//	printf("w.x=%ld,y=%ld,w=%ld,h=%ld\n", w.x,w.y,w.w,w.h);

	Kp.kernelpropid_value = PROPID_KERNEL_GENERIC_YuvFrame;
	Kp.PropTypeLength = sizeof(RMKERNELGENERICYuvFrame_type);
	Kp.pValue = &f;
	RUA_KERNEL_SET_PROPERTY(h,&Kp);

	RUA_DECODER_SET_PROPERTY(h,VIDEO_SET,evZoomedWindow,sizeof(Wnd_type),&w);
	return true;
}

void ShowJpegNew(const char *file, bool PAL)
{
	RUA_handle h;
	h=RUA_OpenDevice(0,TRUE);
	if (h!=NULL) {
		if (PAL == TRUE)
			setPAL(h);
		ShowJpeg(h, file);
	}
	RUA_ReleaseDevice(h);
}
