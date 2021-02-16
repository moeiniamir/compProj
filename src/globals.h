

#ifndef _H_globals
#define _H_globals

#include <string>
using std::string;
class Type;
class Identifier;
class Expr;
class BreakStmt;
class ReturnStmt;
class This;
class Decl;
class Operator;




typedef enum {typeReason, classReason, interfaceReason, variableReason, functionReason} checkFor;

typedef enum {
    sem_decl,
    sem_inh,
    sem_type
} checkStep;


static const char *indx_out_of_bound = "subscript out of bound\\n";
static const char *neg_arr_size = "Array size is <= 0\\n";

extern int syntax_error;
extern int semantic_error;



typedef struct yyltype
{
    int timestamp;
    int first_line, first_column;
    int last_line, last_column;
    char *text;
} yyltype;
#define YYLTYPE yyltype



extern struct yyltype yylloc;



inline yyltype Join(yyltype first, yyltype last)
{
    yyltype combined;
    combined.first_column = first.first_column;
    combined.first_line = first.first_line;
    combined.last_column = last.last_column;
    combined.last_line = last.last_line;
    return combined;
}


inline yyltype Join(yyltype *firstPtr, yyltype *lastPtr)
{
    return Join(*firstPtr, *lastPtr);
}

#endif

