

#include "globals.h"
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

#include "scanner.h"
#include "ast.h"


void yyerror(const char *msg) {
    syntax_error = 1;
    return;
}

int semantic_error = 0;
int syntax_error = 0;

