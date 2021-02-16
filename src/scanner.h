

#ifndef _H_scanner
#define _H_scanner

#include <stdio.h>

#define MaxIdentLen 31

extern char *yytext;


int yylex();
void yyrestart(FILE *fp);


void initializeFlex();


#endif

