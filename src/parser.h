

#ifndef _H_parser
#define _H_parser


#include "scanner.h"
#include "ds.h"
#include "ast.h"


#ifndef YYBISON

#include "y.tab.h"

#endif

int yyparse();


#endif

