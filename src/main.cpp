

#include <string.h>
#include <stdio.h>
#include "globals.h"
#include "parser.h"


int main(int argc, char *argv[]) {
    initializeFlex();
    yyparse();
    return 0;
}

