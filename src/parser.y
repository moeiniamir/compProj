/* File: parser.y
 * --------------
 * Yacc input file to generate the parser for the compiler.
 *
 * pp2: your job is to write a parser that will construct the parse tree
 *      and if no parse errors were found, print it.  The parser should
 *      accept the language as described in specification, and as augmented
 *      in the pp2 handout.
 *
 * pp3: add parser rules and tree construction from your pp2. You should
 *      not need to make any significant changes in the parser itself. After
 *      parsing completes, if no syntax errors were found, the parser calls
 *      program->Check() to kick off the semantic analyzer pass. The
 *      interesting work happens during the tree traversal.
 *
 * pp5: add parser rules and tree construction from your past projects.
 *      You should not need to make any significant changes in the parser
 *      itself.
 *      After parsing completes, if no errors were found, the parser calls
 *      program->Emit() to kick off the code generation pass. The
 *      interesting work happens during the tree traversal.
 *
 * Author: Deyuan Guo
 * Date: Oct 29, 2013
 */

%{

/* Just like lex, the text within this first region delimited by %{ and %}
 * is assumed to be C/C++ code and will be copied verbatim to the y.tab.c
 * file ahead of the definitions of the yyparse() function. Add other header
 * file inclusions or C++ variable declarations/prototypes that are needed
 * by your code here.
 */
#include "scanner.h" // for yylex
#include "parser.h"
#include "errors.h"
#include "codegen.h"
#include "ast.h"

void yyerror(const char *msg); // standard error-handling routine

%}

/* The section before the first %% is the Definitions section of the yacc
 * input file. Here is where you declare tokens and types, add precedence
 * and associativity options, and so on.
 */

/* yylval
 * ------
 * Here we define the type of the yylval global variable that is used by
 * the scanner to store attibute information about the token just scanned
 * and thus communicate that information to the parser.
 *
 * pp2: You will need to add new fields to this union as you add different
 *      attributes to your non-terminal symbols.
 */
%union {
    int integerConstant;
    bool boolConstant;
    char *stringConstant;
    double doubleConstant;
    char identifier[MaxIdentLen+1]; // +1 for terminating null

    Program *program;

    Decl *decl;
    VarDecl *var;
    FnDecl *fDecl;
    ClassDecl *cDecl;
    InterfaceDecl *iDecl;

    Type *type;
    NamedType *namedType;
    List<NamedType*> *namedTypeList;

    Stmt *stmt;
    List<Stmt*> *stmtList;
    List<VarDecl*> *varList;
    List<Decl*> *declList;
    StmtBlock *stmtBlock;
    ForStmt *forStmt;
    WhileStmt *whileStmt;
    IfStmt *ifStmt;
    SwitchStmt *switchStmt;
    CaseStmt *caseStmt;
    List<CaseStmt*> *caseStmtList;
    BreakStmt *breakStmt;
    ReturnStmt *returnStmt;
    PrintStmt *printStmt;

    Expr *expr;
    List<Expr*> *exprList;
    Call *call;
    LValue *lValue;
}


/* Tokens
 * ------
 * Here we tell yacc about all the token types that we are using.
 * Yacc will assign unique numbers to these and export the #define
 * in the generated y.tab.h header file.
 */
%token   T_Void T_Bool T_Int T_Double T_String T_Class
%token   T_LessEqual T_GreaterEqual T_Equal T_NotEqual T_Dims
%token   T_And T_Or T_Null T_Extends T_This T_Interface T_Implements
%token   T_While T_For T_If T_Else T_Return T_Break
%token   T_New T_NewArray T_Print T_ReadInteger T_ReadLine
%token   T_Incr T_Decr T_Switch T_Case T_Default

%token   <identifier> T_Identifier
%token   <stringConstant> T_StringConstant
%token   <integerConstant> T_IntConstant
%token   <doubleConstant> T_DoubleConstant
%token   <boolConstant> T_BoolConstant


/* Non-terminal types
 * ------------------
 * In order for yacc to assign/access the correct field of $$, $1, we
 * must to declare which field is appropriate for the non-terminal.
 * As an example, this first type declaration establishes that the DeclList
 * non-terminal uses the field named "declList" in the yylval union. This
 * means that when we are setting $$ for a reduction for DeclList ore reading
 * $n which corresponds to a DeclList nonterminal we are accessing the field
 * of the union named "declList" which is of type List<Decl*>.
 * pp2: You'll need to add many of these of your own.
 */
%type <program>         Program

%type <decl>            Decl Field
%type <declList>        DeclList PrototypeList FieldList
%type <var>             Variable VarDecl
%type <varList>         Formals FormalList VarDecls
%type <fDecl>           FnDecl FnHeader Prototype
%type <cDecl>           ClassDecl
%type <iDecl>           InterfaceDecl

%type <type>            Type
%type <namedType>       ClassExt
%type <namedTypeList>   ClassImpl ImplList

%type <stmt>            Stmt
%type <stmtList>        StmtList
%type <stmtBlock>       StmtBlock
%type <forStmt>         ForStmt
%type <whileStmt>       WhileStmt
%type <ifStmt>          IfStmt
%type <switchStmt>      SwitchStmt
%type <caseStmt>        CaseStmt DefaultStmt
%type <caseStmtList>    CaseStmts
%type <breakStmt>       BreakStmt
%type <returnStmt>      ReturnStmt
%type <printStmt>       PrintStmt

%type <expr>            Expr ExprOpt Constant
%type <exprList>        ExprPlus Actuals
%type <call>            Call
%type <lValue>          LValue

%nonassoc '='
%left     T_Or
%left     T_And
%nonassoc T_Equal T_NotEqual
%nonassoc '<' T_LessEqual '>' T_GreaterEqual
%left     '+' '-'
%left     '*' '/' '%'
%nonassoc NOT NEG T_Incr T_Decr
%left     '[' '.'
%nonassoc T_NONELSE
%nonassoc T_Else

%%

/* Rules
 * -----
 * All productions and actions should be placed between the start and stop
 * %% markers which delimit the Rules section.

 */
Program   :    DeclList            {
                                      @1;
                                      /* pp2: The @1 is needed to convince
                                       * yacc to set up yylloc. You can remove
                                       * it once you have other uses of @n */
                                      Program *program = new Program($1);
                                      // if no errors, advance to next phase
                                      if (syntax_error == 0)
                                          program->BuildST();
                                      else{
                                          printf("syntax error");
                                          return 1;
                                      }
printf("# %d",semantic_error);
                                      if (semantic_error == 0)
                                          program->Check();
printf("# %d",semantic_error);
                                      if (semantic_error == 0)
                                          program->Emit();
printf("# %d",semantic_error);
                                      if (semantic_error != 0){
                                          CodeGenerator *CG = new CodeGenerator();
                                          CG->GenLabel("main");
                                          BeginFunc *bf = CG->GenBeginFunc();
                                          BuiltIn f = PrintString;
                                          char const * str = "Semantic Error";
                                          Location *l = CG->GenLoadConstant(str);
                                          Assert(l);
                                          CG->GenBuiltInCall(f, l);
                                          bf->SetFrameSize(CG->GetFrameSize());
                                          CG->GenEndFunc();
                                          //CG->GenBuiltInCall(Halt);
                                          CG->DoFinalCodeGen();
                                      }
                                    }
;

DeclList  :    DeclList Decl        { ($$=$1)->Append($2); }
          |    Decl                 { ($$ = new List<Decl*>)->Append($1); }
;

Decl      :    VarDecl              { $$=$1; }
          |    FnDecl               { $$=$1; }
          |    ClassDecl            { $$=$1; }
          |    InterfaceDecl        { $$=$1; }
;

VarDecls  :    VarDecls VarDecl     { ($$=$1)->Append($2); }
          |    /* empty */          { $$ = new List<VarDecl*>; }
;

VarDecl   :    Variable ';'         { $$=$1; }
;

Variable  :    Type T_Identifier    { $$ = new VarDecl(new Identifier(@2, $2), $1); }
;

Type      :    T_Int                { $$ = Type::intType; }
          |    T_Bool               { $$ = Type::boolType; }
          |    T_String             { $$ = Type::stringType; }
          |    T_Double             { $$ = Type::doubleType; }
          |    T_Identifier         { $$ = new NamedType(new Identifier(@1,$1)); }
          |    Type T_Dims          { $$ = new ArrayType(Join(@1, @2), $1); }
;

FnDecl    :    FnHeader StmtBlock   { ($$=$1)->SetFunctionBody($2); }
;

FnHeader  :    Type T_Identifier '(' Formals ')'
                                    { $$ = new FnDecl(new Identifier(@2, $2), $1, $4); }
          |    T_Void T_Identifier '(' Formals ')'
                                    { $$ = new FnDecl(new Identifier(@2, $2), Type::voidType, $4); }
;

Formals   :    FormalList           { $$ = $1; }
          |    /* empty */          { $$ = new List<VarDecl*>; }
;

FormalList:    FormalList ',' Variable
                                    { ($$=$1)->Append($3); }
          |    Variable             { ($$ = new List<VarDecl*>)->Append($1); }
;

ClassDecl :    T_Class T_Identifier ClassExt ClassImpl '{' FieldList '}'
                                    { $$ = new ClassDecl(
                                           (new Identifier(@2, $2)),
                                           $3, $4, $6);
                                    }
;

ClassExt  :    T_Extends T_Identifier
                                    { $$ = new NamedType(
                                           new Identifier(@2, $2));
                                    }
          |    /* empty */          { $$ = NULL; }
;

ClassImpl :    T_Implements ImplList
                                    { $$ = $2; }
          |    /* empty */          { $$ = new List<NamedType*>; }
;

ImplList  :    ImplList ',' T_Identifier
                                    { ($$ = $1)->Append(new NamedType(
                                                 new Identifier(@3, $3)));
                                    }
          |    T_Identifier         { ($$ = new List<NamedType*>)->Append(
                                                 new NamedType(
                                                 new Identifier(@1, $1)));
                                    }
;

FieldList :    FieldList Field      { ($$ = $1)->Append($2); }
          |    /* empty */          { $$ = new List<Decl*>; }
;

Field     :    VarDecl              { $$ = $1; }
          |    FnDecl               { $$ = $1; }
;

InterfaceDecl
          : T_Interface T_Identifier '{' PrototypeList '}'
                                    { $$ = new InterfaceDecl(
                                           (new Identifier(@2, $2)), $4);
                                    }
;

PrototypeList
          : PrototypeList Prototype
                                    { ($$ = $1)->Append($2); }
          |    /* empty */          { $$ = new List<Decl*>; }
;

Prototype :    Type T_Identifier '(' Formals ')' ';'
                                    { $$ = new FnDecl((new Identifier(@2, $2)), $1, $4); }
          |    T_Void T_Identifier '(' Formals ')' ';'
                                    { $$ = new FnDecl((new Identifier(@2, $2)),
                                           Type::voidType, $4);
                                    }
;

StmtBlock :    '{' VarDecls StmtList '}'
                                    { $$ = new StmtBlock($2, $3); }
          |    '{' VarDecls '}'     { $$ = new StmtBlock($2, new List<Stmt *>);
                                    }
;

StmtList  :    StmtList Stmt        { ($$ = $1)->Append($2); }
          |    Stmt                 { ($$ = new List<Stmt*>)->Append($1); }
;

Stmt      :    ExprOpt ';'          { $$ = $1; }
          |    IfStmt               { $$ = $1; }
          |    WhileStmt            { $$ = $1; }
          |    ForStmt              { $$ = $1; }
          |    BreakStmt            { $$ = $1; }
          |    SwitchStmt           { $$ = $1; }
          |    ReturnStmt           { $$ = $1; }
          |    PrintStmt            { $$ = $1; }
          |    StmtBlock            { $$ = $1; }
;

IfStmt    :    T_If '(' Expr ')' Stmt %prec T_NONELSE
                                    { $$ = new IfStmt($3, $5, NULL); }
          |    T_If '(' Expr ')' Stmt T_Else Stmt
                                    { $$ = new IfStmt($3, $5, $7); }
;

WhileStmt :    T_While '(' Expr ')' Stmt
                                    { $$ = new WhileStmt($3, $5); }
;

ForStmt   :    T_For '(' ExprOpt ';' Expr ';' ExprOpt ')' Stmt
                                    { $$ = new ForStmt($3, $5, $7, $9); }
;

SwitchStmt:    T_Switch '(' Expr ')' '{' CaseStmts DefaultStmt '}'
                                    { if ($7) $6->Append($7);
                                      $$ = new SwitchStmt($3, $6);
                                    }
;

CaseStmts :    CaseStmts CaseStmt   { ($$ = $1)->Append($2); }
          |    /* empty */          { $$ = new List<CaseStmt*>; }
;

CaseStmt  :    T_Case T_IntConstant ':' StmtList
                                    { $$ = new CaseStmt(
                                           (new IntConstant(@2, $2)), $4);
                                    }
          |    T_Case T_IntConstant ':'
                                    { $$ = new CaseStmt(
                                           (new IntConstant(@2, $2)),
                                           (new List<Stmt*>));
                                    }
;

DefaultStmt
          :    T_Default ':' StmtList
                                    { $$ = new CaseStmt(NULL, $3); }
          |    T_Default ':'        { $$ = new CaseStmt(NULL, new List<Stmt*>); }
          |    /* empty */          { $$ = NULL; }
;

ReturnStmt:    T_Return Expr ';'    { $$ = new ReturnStmt(@2, $2); }
          |    T_Return ';'         { $$ = new ReturnStmt(@1, new EmptyExpr()); }
;

BreakStmt :    T_Break ';'          { $$ = new BreakStmt(@1); }
;

PrintStmt :    T_Print '(' ExprPlus ')' ';'
                                    { $$ = new PrintStmt($3); }
;

ExprOpt   :    Expr                 { $$ = $1; }
          |    /* empty */          { $$ = new EmptyExpr(); }
;

ExprPlus  :    ExprPlus ',' Expr    { ($$ = $1)->Append($3); }
          |    Expr                 { ($$ = new List<Expr*>)->Append($1); }
;

Expr      :    LValue '=' Expr      { $$ = new AssignExpr($1,
                                           (new Operator(@2, "=")), $3);
                                    }
          |    Constant             { $$ = $1; }
          |    LValue               { $$ = $1; }
          |    LValue T_Incr        { $$ = new PostfixExpr($1,
                                           (new Operator(@2, "++")));
                                    }
          |    LValue T_Decr        { $$ = new PostfixExpr($1,
                                           (new Operator(@2, "--")));
                                    }
          |    T_This               { $$ = new This(@1); }
          |    Call                 { $$ = $1; }
          |    '(' Expr ')'         { $$ = $2; }
          |    Expr '+' Expr        { $$ = new ArithmeticExpr($1,
                                           (new Operator(@2, "+")), $3);
                                    }
          |    Expr '-' Expr        { $$ = new ArithmeticExpr($1,
                                           (new Operator(@2, "-")), $3);
                                    }
          |    Expr '*' Expr        { $$ = new ArithmeticExpr($1,
                                           (new Operator(@2, "*")), $3);
                                    }
          |    Expr '/' Expr        { $$ = new ArithmeticExpr($1,
                                           (new Operator(@2, "/")), $3);
                                    }
          |    Expr '%' Expr        { $$ = new ArithmeticExpr($1,
                                           (new Operator(@2, "%")), $3);
                                    }
          |    '-' Expr %prec NEG   { $$ = new ArithmeticExpr(
                                           (new Operator(@1, "-")), $2);
                                    }
          |    Expr '<' Expr        { $$ = new RelationalExpr($1,
                                           (new Operator(@2, "<")), $3);
                                    }
          |    Expr T_LessEqual Expr
                                    { $$ = new RelationalExpr($1,
                                           (new Operator(@2, "<=")), $3);
                                    }
          |    Expr '>' Expr        { $$ = new RelationalExpr($1,
                                           (new Operator(@2, ">")), $3);
                                    }
          |    Expr T_GreaterEqual Expr
                                    { $$ = new RelationalExpr($1,
                                           (new Operator(@2, ">=")), $3);
                                    }
          |    Expr T_Equal Expr    { $$ = new EqualityExpr($1,
                                           (new Operator(@2, "==")), $3);
                                    }
          |    Expr T_NotEqual Expr { $$ = new EqualityExpr($1,
                                           (new Operator(@2, "!=")), $3);
                                    }
          |    Expr T_And Expr      { $$ = new LogicalExpr($1,
                                           (new Operator(@2, "&&")), $3);
                                    }
          |    Expr T_Or Expr       { $$ = new LogicalExpr($1,
                                           (new Operator(@2, "||")), $3);
                                    }
          |    '!' Expr %prec NOT   { $$ = new LogicalExpr(
                                           (new Operator(@1, "!")), $2);
                                    }
          |    T_ReadInteger '(' ')'
                                    { $$ = new ReadIntegerExpr(Join(@1, @3)); }
          |    T_ReadLine '(' ')'   { $$ = new ReadLineExpr(Join(@1, @3)); }
          |    T_New T_Identifier
                                    { $$ = new NewExpr(Join(@1, @2), (new
                                           NamedType(new Identifier(@2, $2))));
                                    }
          |    T_NewArray '(' Expr ',' Type ')'
                                    { $$ = new NewArrayExpr(Join(@1, @6), $3, $5); }
;

LValue    :    T_Identifier         { $$ = new FieldAccess(NULL,
                                           (new Identifier(@1, $1)));
                                    }
          |    Expr '.' T_Identifier
                                    { $$ = new FieldAccess($1,
                                           (new Identifier(@3, $3)));
                                    }
          |    '.' T_Identifier     { $$ = new FieldAccess(NULL,
                                           (new Identifier(@2, $2)));
                                    }
          |    Expr '[' Expr ']'    { $$ = new ArrayAccess(Join(@1, @4), $1, $3); }
;

Call      :    T_Identifier '(' Actuals ')'
                                    { $$ = new Call(Join(@1, @4), NULL,
                                           (new Identifier(@1, $1)), $3);
                                    }
          |    Expr '.' T_Identifier '(' Actuals ')'
                                    { $$ = new Call(Join(@1, @6), $1,
                                           (new Identifier(@3, $3)), $5);
                                    }
          |    '.' T_Identifier '(' Actuals ')'
                                    { $$ = new Call(Join(@1, @5), NULL,
                                           (new Identifier(@2, $2)), $4);
                                    }
;

Actuals   :    ExprPlus             { $$ = $1; }
          |    /* empty */          { $$ = new List<Expr*>; }
;

Constant  :    T_IntConstant        { $$ = new IntConstant(@1, $1); }
          |    T_DoubleConstant     { $$ = new DoubleConstant(@1, $1); }
          |    T_BoolConstant       { $$ = new BoolConstant(@1, $1); }
          |    T_StringConstant     { $$ = new StringConstant(@1, $1); }
          |    T_Null               { $$ = new NullConstant(@1); }
;

%%

/* The closing %% above marks the end of the Rules section and the beginning
 * of the User Subroutines section. All text from here to the end of the
 * file is copied verbatim to the end of the generated y.tab.c file.
 * This section is where you put definitions of helper functions.
 */

/* Function: InitParser
 * --------------------
 * This function will be called before any calls to yyparse().  It is designed
 * to give you an opportunity to do anything that must be done to initialize
 * the parser (set global variables, configure starting state, etc.). One
 * thing it already does for you is assign the value of the global variable
 * yydebug that controls whether yacc prints debugging information about
 * parser actions (shift/reduce) and contents of state stack during parser.
 * If set to false, no information is printed. Setting it to true will give
 * you a running trail that might be helpful when debugging your parser.
 * Please be sure the variable is set to false when submitting your final
 * version.
 */
void InitParser()
{
   PrintDebug("parser", "Initializing parser");
   yydebug = false;
}

