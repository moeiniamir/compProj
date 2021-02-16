/* File: errors.h
 * --------------
 * This file defines an error-reporting class with a set of already
 * implemented static methods for reporting the standard Decaf errors.
 * You should report all errors via this class so that your error
 * messages will have the same wording/spelling as ours and thus
 * diff can easily compare the two. If needed, you can add new
 * methods if you have some fancy error-reporting, but for the most
 * part, you will just use the class as given.
 */

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

/* General notes on using this class
 * ----------------------------------
 * Each of the methods in thie class matches one of the standard Decaf
 * errors and reports a specific problem such as an unterminated string,
 * type mismatch, declaration conflict, etc. You will call these methods
 * to report problems encountered during the analysis phases. All methods
 * on this class are static, thus you can invoke methods directly via
 * the class name, e.g.
 *
 *    if (missingEnd) ReportError::UntermString(&yylloc, str);
 *
 * For some methods, the first argument is the pointer to the location
 * structure that identifies where the problem is (usually this is the
 * location of the offending token). You can pass NULL for the argument
 * if there is no appropriate position to point out. For other methods,
 * location is accessed by messaging the node in error which is passed
 * as an argument. You cannot pass NULL for these arguments.
 */


typedef enum {typeReason, classReason, interfaceReason, variableReason, functionReason} checkFor;

typedef enum {
    sem_decl,
    sem_inh,
    sem_type
} checkStep;

// Wording to use for runtime error messages
static const char *indx_out_of_bound = "subscript out of bound\\n";
static const char *neg_arr_size = "Array size is <= 0\\n";

extern int syntax_error;
extern int semantic_error;


/* Typedef: yyltype
 * ----------------
 * Defines the struct type that is used by the scanner to store
 * position information about each lexeme scanned.
 */
typedef struct yyltype
{
    int timestamp;                 // you can ignore this field
    int first_line, first_column;
    int last_line, last_column;
    char *text;                    // you can also ignore this field
} yyltype;
#define YYLTYPE yyltype


/* Global variable: yylloc
 * ------------------------
 * The global variable holding the position information about the
 * lexeme just scanned.
 */
extern struct yyltype yylloc;


/* Function: Join
 * --------------
 * Takes two locations and returns a new location which represents
 * the span from first to last, inclusive.
 */
inline yyltype Join(yyltype first, yyltype last)
{
    yyltype combined;
    combined.first_column = first.first_column;
    combined.first_line = first.first_line;
    combined.last_column = last.last_column;
    combined.last_line = last.last_line;
    return combined;
}

/* Same as above, except operates on pointers as a convenience  */
inline yyltype Join(yyltype *firstPtr, yyltype *lastPtr)
{
    return Join(*firstPtr, *lastPtr);
}

#endif

