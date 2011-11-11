%{
#include <stdlib.h>
#include "calc.h"
#include "y.tab.h"
#include "serial.h"
void yyerror(char *);
void yycomment( char *s);
void yyeatchar(int c);
%}

D			[0-9]
H			[a-fA-F0-9]

%%


[a-z]	{
			yylval.sIndex = *yytext - 'a';
			return VARIABLE;
		}

[0-9]+	{
			yylval.iValue = atoi(yytext);
			return INTEGER;
		}

"/*"			{yycomment("*/"); }  /**/
^#.*			;/* Ignore lines that start with '#' */
\/\/.*          ; //

{H}+[Hh]	{
			    yylval.iValue = strtoul((char *) yytext, (char **) 0, 16);
			    return INTEGER;
			}
[0-1]+[bB]	{
				yylval.iValue = strtoul(yytext, (char **) 0, 2);
				return INTEGER;
			}

[-()<>=+*/;{}.] {return *yytext;}
">="            return GE;
"<="            return LE;
"=="            return EQ;
"!="            return NE;
"<>"            return NE1;
"&&"            return AND;
"||"            return OR;
"while"         return WHILE;
"for"           return FOR;
"if"            return IF;
"else"          return ELSE;
"print"         return PRINT;
"open"          return OPEN;
"input"         return INPUT;
"set"           return SET;
"openport"      return OPENPORT;
"closeport"     return CLOSEPORT;
"sleep"         return SLEEP;
"micvolume"     return MICVOLUME;
"dskport"       return DSKPORT;
"musicinput"    return MUSICINPUT;
"musicvolume"   return MUSICVOLUME;
"subwoofer"     return SUBWOOFER;
"musictreble"   return MUSICTREBLE;
"musicbass"     return MUSICBASS;
"subwooferfreq" return SUBWOOFERFREQ;

[ \t\n\r]+      ;       /* ignore whitespace */
.               yyerror("Unknown character");
%%

int yywrap(void) {
	return 1;
}

int column = 0;
int line_num = 1;

void yycomment( char *s)
{
	char c, c1;
	void yyeatchar(int);

loop:
	while ((c = input()) != '*' && c != 0)
		yyeatchar (c);

	if ((c1 = input()) != '/' && c != 0) {
		yyeatchar (c);
		unput(c1);
		goto loop;
	}
	if (c != 0){
		yyeatchar (c);
		/* putchar(c1) */ ;
	}
}

void yyeatchar(int c)
{
	extern int column, line_num;

	if (c == '\n'){
		line_num++;
//		putchar(c);
	}
}
