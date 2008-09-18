#ifndef COLOR_H
#define COLOR_H

typedef struct tagCOLOR{
        short r;
        short g;
        short b;
        short a;
}TColor;

typedef struct tagCOLORLISTS{
        char Name[20];
        long Value;
}ColorLists;

#define COLORNUM 20

#define clBlack      0x000000
#define clMaroon     0x000080
#define clGreen      0x008000
#define clOlive      0x008080
#define clNavy       0x800000
#define clPurple     0x800080
#define clTeal       0x808000
#define clGray       0x808080
#define clSilver     0xC0C0C0
#define clRed        0x0000FF
#define clLime       0x00FF00
#define clYellow     0x00FFFF
#define clBlue       0xFF0000
#define clFuchsia    0xFF00FF
#define clAqua       0xFFFF00
#define clLtGray     0xC0C0C0
#define clDkGray     0x808080
#define clWhite      0xFFFFFF
#define clMoneyGreen 0xC0DCC0
#define clSkyBlue    0xF0CAA6
#define clCream      0xF0FBFF
#define clMedGray    0xA4A0A0

TColor StrToColor(const char *str);
TColor argb2color(short a, short r, short g, short b);
#endif

