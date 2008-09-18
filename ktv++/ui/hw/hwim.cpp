#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hwim.h"

hwInput::hwInput()
{
	clearPoint();
	recmode = REC_ALL;
}

void hwInput::setMode(int mode)
{
	recmode = mode;
}

void hwInput::clearPoint(void)
{
	memset(data.m_data, -1, (NALLOC + 4)*sizeof(WORD));
	data.m_len = 0;
	data.strokeLen = 0;
}

void hwInput::addPoint(int x, int y)
{
	int i=0;
	while (i<data.m_len-1) {
		if ((data.m_data[i+1] ==x) &&
			(data.m_data[i+2]==y))
			return;
		i++;
	}

	if (data.m_len < MAXSIZE -1 ) {
		if ((x > 0) && (y > 0)){
			data.m_data[data.m_len++] = x;
			data.m_data[data.m_len++] = y;
			data.strokes[data.strokeLen].endIndex = data.m_len + 2;
		}
	}
}

void hwInput::startStroke(void)
{
	data.strokes[data.strokeLen].startIndex = data.m_len;
}

void hwInput::endStroke(void)
{
	data.m_data[data.m_len++] = (WORD)-1;
	data.m_data[data.m_len++] = 0;
	data.strokes[data.strokeLen].endIndex = data.m_len;
	data.strokeLen++;
}

void hwInput::removeStroke(void)
{
	data.strokeLen--;
	int len = (data.strokes[data.strokeLen].startIndex - data.strokes[data.strokeLen].endIndex) * sizeof(WORD);
	memset(data.m_data + data.strokes[data.strokeLen].startIndex, 0, len);
}

int hwInput::process(char *pResult)
{
//	for (int i=0;i<data.m_len+2;i++)
//		printf("%d ", data.m_data[i]);
//	printf("Ok\n");
//	printf("recmode=%x\n", recmode);
	int x= HWRecognize(data.m_data, data.m_len + 2, pResult, NWORD, recmode);
//	clearPoint();
	return x;
}

