/* File: errors.cc
 * ---------------
 * Implementation for error-reporting class.
 */

#include "errors.h"
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
using namespace std;

#include "scanner.h" // for GetLineNumbered
#include "ast_type.h"
#include "ast_expr.h"
#include "ast_stmt.h"
#include "ast_decl.h"


void yyerror(const char *msg) {
    syntax_error = 1;
    return;
}

int semantic_error = 0;
int syntax_error = 0;

