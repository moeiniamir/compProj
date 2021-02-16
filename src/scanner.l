


%option noyywrap

%{

#include <string.h>
#include "scanner.h"
#include "globals.h"
#include "parser.h"
#include "ds.h"

#define TAB_SIZE 8


static int curLineNum, curColNum;
List<const char*> savedLines;

%}


%s N
%x COPY COMM
%option stack


DIGIT             ([0-9])
HEX_DIGIT         ([0-9a-fA-F])
HEX_INTEGER       (0[Xx]{HEX_DIGIT}+)
INTEGER           ({DIGIT}+)
EXPONENT          ([Ee][+-]?{INTEGER})
DOUBLE            ({INTEGER}"."{DIGIT}*{EXPONENT}?)
BEG_STRING        (\"[^"\n]*)
STRING            ({BEG_STRING}\")
IDENTIFIER        ([a-zA-Z][a-zA-Z_0-9]*)
OPERATOR          ([-+/*%=.,:;!<>()[\]{}])
BEG_COMMENT       ("/*")
END_COMMENT       ("*/")
SINGLE_COMMENT    ("//"[^\n]*)

%%

<COPY>.*               { char curLine[512];

                         savedLines.Append(strdup(yytext));
                         curColNum = 1; yy_pop_state(); yyless(0); }
<COPY><<EOF>>          { yy_pop_state(); }
<*>\n                  { curLineNum++; curColNum = 1;
                         if (YYSTATE == COPY) savedLines.Append("");
                         else yy_push_state(COPY); }

<*>[\t]                { curColNum += TAB_SIZE - curColNum%TAB_SIZE + 1; }
[[:space:]]+                   {   }


{BEG_COMMENT}          { BEGIN(COMM); }
<COMM>{END_COMMENT}    { BEGIN(N); }
<COMM><<EOF>>          { syntax_error = 1;
                                 return 0; }
<COMM>.                {  }
{SINGLE_COMMENT}       {  }



"void"              {return T_Void;}
"int"               {return T_Int;}
"double"            {return T_Double;}
"bool"              {return T_Bool;}
"string"            {return T_String;}
"null"              {return T_Null;}
"class"             {return T_Class;}
"extends"           {return T_Extends;}
"this"              {return T_This;}
"interface"         {return T_Interface;}
"implements"        {return T_Implements;}
"while"             {return T_While;}
"for"               {return T_For;}
"if"                {return T_If;}
"else"              {return T_Else;}
"return"            {return T_Return;}
"break"             {return T_Break;}
"continue"          {return T_Continue;}
"new"               {return T_New;}
"NewArray"          {return T_NewArray;}
"Print"             {return T_Print;}
"ReadInteger"       {return T_ReadInteger;}
"ReadLine"          {return T_ReadLine;}
"dtoi"              {return T_Dtoi;}
"itod"              {return T_Itod;}
"btoi"              {return T_Btoi;}
"itob"              {return T_Itob;}
"private"           {return T_Private;}
"protected"         {return T_Protected;}
"public"            {return T_Public;}




"<="                {return T_LessEqual;}
">="                {return T_GreaterEqual;}
"=="                {return T_Equal;}
"!="                {return T_NotEqual;}
"&&"                {return T_And;}
"||"                {return T_Or;}
{OPERATOR}          {return yytext[0];}
"[]"                {return T_Dims;}


"true"|"false"      { yylval.booleanLiteral = (yytext[0] == 't');
                         return T_BOOLEANLITERAL; }
{INTEGER}           { yylval.integerLiteral = strtol(yytext, NULL, 10);
                         return T_INTLITERAL; }
{HEX_INTEGER}       { yylval.integerLiteral = strtol(yytext, NULL, 16);
                         return T_INTLITERAL; }
{DOUBLE}            { yylval.doubleLiteral = atof(yytext);
                         return T_DOUBLELITERAL; }
{STRING}            { yylval.stringLiteral = strdup(yytext);
                         return T_STRINGLITERAL; }
{BEG_STRING}        { syntax_error = 1;
                                                       return 0; }



{IDENTIFIER}        { if (strlen(yytext) > MaxIdentLen)
                         {syntax_error = 1;
                          return 0;}
                       strncpy(yylval.identifier, yytext, MaxIdentLen);
                       yylval.identifier[MaxIdentLen] = '\0';
                       return T_ID; }



.                   { syntax_error = 1;
                      return 0; }

%%

void initializeFlex()
{
    yy_flex_debug = false;
}
