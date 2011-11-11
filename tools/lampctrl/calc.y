%{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "calc.h"
#include "serial.h"

/* prototypes */
nodeType *opr(int oper, int nops, ...);
nodeType *id(int i);
nodeType *con(int value);
void freeNode(nodeType *p);
int ex(nodeType *p);
int yylex(void);

extern FILE *yyin, *yyout;
extern char *optarg;
extern void printbin(int x, int len);

unsigned char curvalue;
FILE *curout = NULL;

void yyerror(char *s);
int sym[26];                    /* symbol table    */
int portid = -1;                /* µÆ¹â¿ØÖÆÆ÷´®¿ÚºÅ */

%}

%union {
	int iValue;                 /* integer value      */
	char sIndex;                /* symbol table index */
	nodeType *nPtr;             /* node pointer       */
};

%token <iValue> INTEGER
%token <sIndex> VARIABLE
%token WHILE FOR IF PRINT OPEN INPUT SET SLEEP OPENPORT CLOSEPORT MICVOLUME MICDELAY MICECHO MICBASS MUSICINPUT DSKPORT MUSICVOLUME SUBWOOFER MUSICTREBLE MUSICBASS SUBWOOFERFREQ

%nonassoc IFX
%nonassoc ELSE


%left GE LE EQ NE NE1 '>' '<' AND OR
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%type <nPtr> stmt expr stmt_list

%%

program:
        function                { exit(0); }
        ;

function:
          function stmt         { ex($2); freeNode($2); }
        | /* NULL */
        ;

stmt:
        ';'                                       { $$ = opr(';', 2, NULL, NULL);           }
        | expr ';'                                { $$ = $1;                                }
        | PRINT expr ';'                          { $$ = opr(PRINT    ,  1, $2);            }
        | OPEN expr ';'                           { $$ = opr(OPEN     ,  1, $2);            }
        | INPUT expr ';'                          { $$ = opr(INPUT    ,  1, $2);            }
        | SET expr '=' expr ';'                   { $$ = opr(SET      ,  2, $2, $4);        }
        | SLEEP expr ';'                          { $$ = opr(SLEEP    ,  1, $2);            }
        | OPENPORT expr ';'                       { $$ = opr(OPENPORT ,  1, $2);            }
        | CLOSEPORT ';'                           { $$ = opr(CLOSEPORT,  2, NULL, NULL);    }
        | VARIABLE '=' expr ';'                   { $$ = opr('=',        2, id($1), $3);    }
        | MUSICINPUT expr ';'                     { $$ = opr(MUSICINPUT, 1, $2);            }
        | MICDELAY expr ',' expr ';'              { $$ = opr(MICDELAY,   2, $2, $4);        }
        | MICECHO expr ',' expr ';'               { $$ = opr(MICECHO,    2, $2, $4);        }
        | MICBASS expr ',' expr ';'               { $$ = opr(MICBASS,    2, $2, $4);        }

        | DSKPORT expr ';'                        { $$ = opr(DSKPORT,    1, $2);            }
        | MUSICVOLUME expr ';'                    { $$ = opr(MUSICVOLUME,1, $2);            }
        | MICVOLUME expr ';'                      { $$ = opr(MICVOLUME,  1, $2);            }
        | SUBWOOFER expr ';'                      { $$ = opr(SUBWOOFER,  1, $2);            }
        | MUSICTREBLE expr ';'                    { $$ = opr(MUSICTREBLE,1, $2);            }
        | MUSICBASS expr ';'                      { $$ = opr(MUSICBASS,  1, $2);            }
        | SUBWOOFERFREQ expr ';'                  { $$ = opr(SUBWOOFERFREQ,  1, $2);        }

        | WHILE '(' expr ')' stmt                 { $$ = opr(WHILE,      2, $3, $5);        }
        | FOR '(' expr ';' expr ';' expr ')' stmt { $$ = opr(FOR,        4, $3, $5, $7, $9);}
        | IF '(' expr ')' stmt %prec IFX          { $$ = opr(IF,         2, $3, $5);        }
        | IF '(' expr ')' stmt ELSE stmt          { $$ = opr(IF,         3, $3, $5, $7);    }
        | '{' stmt_list '}'                       { $$ = $2;                                }
	| error ';' {yyerrok;}
	| error '}'{}
;

stmt_list:
          stmt                  { $$ = $1; }
        | stmt_list stmt        { $$ = opr(';', 2, $1, $2); }
        ;

expr:
          INTEGER               { $$ = con($1); }
        | VARIABLE              { $$ = id($1);  }
        | '-' expr %prec UMINUS { $$ = opr(UMINUS, 1, $2);  }
        | expr '+' expr         { $$ = opr('+', 2, $1, $3); }
        | expr '-' expr         { $$ = opr('-', 2, $1, $3); }
        | expr '*' expr         { $$ = opr('*', 2, $1, $3); }
        | expr '/' expr         { $$ = opr('/', 2, $1, $3); }
        | expr '<' expr         { $$ = opr('<', 2, $1, $3); }
        | expr '>' expr         { $$ = opr('>', 2, $1, $3); }
        | expr GE  expr         { $$ = opr(GE,  2, $1, $3); }
        | expr LE  expr         { $$ = opr(LE,  2, $1, $3); }
        | expr NE  expr         { $$ = opr(NE,  2, $1, $3); }
        | expr NE1 expr         { $$ = opr(NE,  2, $1, $3); }
        | expr EQ  expr         { $$ = opr(EQ,  2, $1, $3); }
        | expr AND expr         { $$ = opr(AND, 2, $1, $3); }
        | expr OR  expr         { $$ = opr(OR,  2, $1, $3); }
        | '(' expr ')'          { $$ = $2; }
        ;

%%

#define SIZEOF_NODETYPE ((char *)&p->con - (char *)p)

nodeType *con(int value)
{
	nodeType *p;
	size_t nodeSize;

	/* allocate node */
	nodeSize = SIZEOF_NODETYPE + sizeof(conNodeType);
	if ((p = malloc(nodeSize)) == NULL)
		yyerror("out of memory");

	/* copy information */
	p->type = typeCon;
	p->con.value = value;

	return p;
}

nodeType *id(int i)
{
	nodeType *p;
	size_t nodeSize;

	/* allocate node */
	nodeSize = SIZEOF_NODETYPE + sizeof(idNodeType);
	if ((p = malloc(nodeSize)) == NULL)
		yyerror("out of memory");

	/* copy information */
	p->type = typeId;
	p->id.i = i;

	return p;
}

nodeType *opr(int oper, int nops, ...)
{
	va_list ap;
	nodeType *p;
	size_t nodeSize;
	int i;

	/* allocate node */
	nodeSize = SIZEOF_NODETYPE + sizeof(oprNodeType) +
		(nops - 1) * sizeof(nodeType*);
	if ((p = malloc(nodeSize)) == NULL)
		yyerror("out of memory");

	/* copy information */
	p->type = typeOpr;
	p->opr.oper = oper;
	p->opr.nops = nops;
	va_start(ap, nops);
	for (i = 0; i < nops; i++)
		p->opr.op[i] = va_arg(ap, nodeType*);
	va_end(ap);
	return p;
}

void freeNode(nodeType *p)
{
	int i;

	if (!p) return;
	if (p->type == typeOpr) {
		for (i = 0; i < p->opr.nops; i++)
			freeNode(p->opr.op[i]);
	}
	free (p);
}

void yyerror(char *s)
{
//	extern int yylineno;
//	fprintf(stderr, "line %d: %s\n", yylineno, s);
	fprintf(stderr, "Line: %s\n", s);
}

#ifdef READTHREAD
static void *CommReadThread(void *val)
{
	int nread;
	char buf[512];
	while (1) { //¿»·¶Á¡Ê¾Ý
		if (portid == -1) {
			sleep(1);
			continue;
		}
		nread = SyncReadComm(portid, buf, 512);
		if (nread > 0) {
			buf[nread] = '\0';
			if ((buf[0] == '1') || (buf[0] == '0'))
				printf("%s\n", buf);
//			fflush(stdout);
		}
	sleep(1);
	}
}
#endif

int main(int argc, char *argv[])
{
	int ch;
	opterr =0;
	int fd[2];
	portid= -1;

	curout = fopen("/ktvdata/cur.dat", "a+");
	fseek(curout, 0L, SEEK_SET);
	if (curout) {
		fread(&curvalue, sizeof(unsigned char), 1, curout);
//		printbin(curvalue, 8);
	}


//	int i;
//	for (i=0;i<argc;i++)
//		printf("argv[%d]=%s\n", i, argv[i]);
	while ((ch = getopt(argc, argv, "f:c:p:h"))!= -1)
	{
		switch (ch) {
			case 'f':
				yyin = fopen(optarg, "r");
				break;
			case 'c':
				pipe(fd);
				dup2(fd[0], STDIN_FILENO);
				write(fd[1], optarg, strlen(optarg));
				close(fd[0]);
				close(fd[1]);
				break;
			case 'h':
				printf("usage: %s [-f <file>] [-c <cmdlist>] [-p <commport>] [-h]\n", argv[0]);
				exit(0);
				break;
			case 'p':{
				int port = atoi(optarg);
				if ((portid = OpenComm(port, 19200, 8, "1", 'N')) == -1)
					fprintf(stderr, "open /dev/ttyS%d error.\n", port);
#ifdef READTHREAD
				pthread_t PlayPthread = 0;
				pthread_create(&PlayPthread, NULL, &CommReadThread, NULL);
#endif
				break;
			}
		}
	}

	yyparse();
	fclose(yyin);
	fclose(yyout);
	return 0;
}
