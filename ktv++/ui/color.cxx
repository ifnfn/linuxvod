#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "color.h"

#define REGCOLOR(REG_NAME) {#REG_NAME, REG_NAME},

static ColorLists Colors[COLORNUM] = {
	REGCOLOR(clBlack)
	REGCOLOR(clMaroon)
	REGCOLOR(clGreen)
	REGCOLOR(clOlive)
	REGCOLOR(clNavy)
	REGCOLOR(clPurple)
	REGCOLOR(clTeal)
	REGCOLOR(clGray)
	REGCOLOR(clSilver)
	REGCOLOR(clRed)
	REGCOLOR(clLime)
	REGCOLOR(clYellow)
	REGCOLOR(clBlue)
	REGCOLOR(clFuchsia)
	REGCOLOR(clAqua)
	REGCOLOR(clWhite)
	REGCOLOR(clMoneyGreen)
	REGCOLOR(clSkyBlue)
	REGCOLOR(clCream)
	REGCOLOR(clMedGray)
};

TColor StrToColor(const char *str)
{
	TColor color;
	uint32_t cValue = 0;
	color.a = color.r = color.g = color.b = 0;
	if (strlen(str) == 0)
		return color;
	if (!isdigit(str[0]))
	{
		for (int i=0; i<COLORNUM;i++)
		{
			if (strcasecmp(Colors[i].Name, str) == 0)
			{
				cValue = Colors[i].Value;
				break;
			}
		}
	} else
		sscanf(str, "%X", &cValue);
	color.a = 0;
	color.r = (unsigned char)(cValue);
	color.g = (unsigned char)(cValue>>8);
	color.b = (unsigned char)(cValue>>16);
	return color;
}

TColor argb2color(short a, short r, short g, short b)
{
	TColor c;
	c.a = a;
	c.r = r;
	c.g = g;
	c.b = b;
	return c;
}

