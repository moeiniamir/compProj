

%{


#include "scanner.h"
#include "parser.h"
#include "globals.h"
#include "codegen.h"
#include "ast.h"

void yyerror(const char *msg);

%}




%union {
    int integerLiteral;
    bool booleanLiteral;
    char *stringLiteral;
    double doubleLiteral;
    char identifier[MaxIdentLen+1];

    Program *program;

    Decl *decl;
    VariableDecl *var;
    FunctionDecl *fDecl;
    ClassDecl *cDecl;
    InterfaceDecl *iDecl;

    Type *type;
    NamedType *namedType;
    List<NamedType*> *namedTypeList;

    Stmt *stmt;
    List<Stmt*> *stmts;
    List<VariableDecl*> *varList;
    List<Decl*> *declList;
    StmtBlock *stmtBlock;
    ForStmt *forStmt;
    WhileStmt *whileStmt;
    IfStmt *ifStmt;
    BreakStmt *breakStmt;
    ReturnStmt *returnStmt;
    PrintStmt *printStmt;

    Expr *expr;
    List<Expr*> *exprList;
    Call *call;
    LValue *lValue;
}



%token   T_Void T_Bool T_Int T_Double T_String T_Class
%token   T_LessEqual T_GreaterEqual T_Equal T_NotEqual T_Dims
%token   T_And T_Or T_Null T_Extends T_This T_Interface T_Implements
%token   T_While T_For T_If T_Else T_Return T_Break T_Continue
%token   T_Dtoi T_Itod T_Btoi T_Itob T_Private T_Protected T_Public
%token   T_New T_NewArray T_Print T_ReadInteger T_ReadLine

%token   <identifier> T_ID
%token   <stringLiteral> T_STRINGLITERAL
%token   <integerLiteral> T_INTLITERAL
%token   <doubleLiteral> T_DOUBLELITERAL
%token   <booleanLiteral> T_BOOLEANLITERAL



%type <program>         Program

%type <decl>            Decl Field
%type <declList>        DeclList Prototypes Fields
%type <var>             Variable VariableDecl
%type <varList>         Formals FormalList VariableDecls
%type <fDecl>           FunctionDecl FnHeader Prototype
%type <cDecl>           ClassDecl
%type <iDecl>           InterfaceDecl

%type <type>            Type
%type <namedType>       Extends
%type <namedTypeList>   Implements ImplementsContinue

%type <stmt>            Stmt
%type <stmts>        	Stmts
%type <stmtBlock>       StmtBlock
%type <forStmt>         ForStmt
%type <whileStmt>       WhileStmt
%type <ifStmt>          IfStmt
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


Program:
DeclList
{
      @1;

      Program *program = new Program($1);

      if (syntax_error == 0)
	  program->BuildSymTable();
      else{
	  printf("syntax error");
	  return 1;
      }
      if (semantic_error == 0)
	  program->Check();
      if (semantic_error == 0)
	  program->Emit();
      if (semantic_error != 0){
	  CodeGenerator *CG = new CodeGenerator();
	  CG->GenLabel("main");
	  BeginFunc *bf = CG->GenBeginFunc();
	  BuiltIn f = PrintString;
	  char const * str = "Semantic Error";
	  Location *l = CG->GenLoadConstant(str);

	  CG->GenBuiltInCall(f, l);
	  bf->SetFrameSize(CG->GetFrameSize());
	  CG->GenEndFunc();

	  CG->DoFinalCodeGen();
      }
}
;

DeclList:
DeclList Decl        { ($$=$1)->Append($2); }
| Decl               { ($$ = new List<Decl*>)->Append($1); }
;

Decl:
VariableDecl              { $$=$1; }
| FunctionDecl               { $$=$1; }
| ClassDecl            { $$=$1; }
| InterfaceDecl        { $$=$1; }
;

VariableDecls:
VariableDecls VariableDecl     { ($$=$1)->Append($2); }
|              { $$ = new List<VariableDecl*>; }
;

VariableDecl:
Variable ';'         { $$=$1; }
;

Variable:
Type T_ID    { $$ = new VariableDecl(new Identifier(@2, $2), $1); }
;

Type:
T_Int                { $$ = Type::intType; }
| T_Double             { $$ = Type::doubleType; }
| T_Bool               { $$ = Type::boolType; }
| T_String             { $$ = Type::stringType; }
| T_ID         { $$ = new NamedType(new Identifier(@1,$1)); }
| Type T_Dims          { $$ = new ArrayType(Join(@1, @2), $1); }
;

FunctionDecl:
FnHeader StmtBlock   { ($$=$1)->SetFunctionBody($2); }
;

FnHeader:
Type T_ID '(' Formals ')'   { $$ = new FunctionDecl(new Identifier(@2, $2), $1, $4); }
| T_Void T_ID '(' Formals ')'     { $$ = new FunctionDecl(new Identifier(@2, $2), Type::voidType, $4); }
;

Formals:
FormalList           { $$ = $1; }
|              { $$ = new List<VariableDecl*>; }
;

FormalList:
FormalList ',' Variable            { ($$=$1)->Append($3); }
| Variable             { ($$ = new List<VariableDecl*>)->Append($1); }
;

ClassDecl:
T_Class T_ID Extends Implements '{' Fields '}'
{
$$ = new ClassDecl(
   (new Identifier(@2, $2)),
   $3, $4, $6);
}
;

Extends:
T_Extends T_ID
{
$$ = new NamedType(
   new Identifier(@2, $2));
}
|              { $$ = NULL; }
;

Implements:
T_Implements ImplementsContinue { $$ = $2; }
|              { $$ = new List<NamedType*>; }
;

ImplementsContinue:
ImplementsContinue ',' T_ID
{
($$ = $1)->Append(new NamedType(
	 new Identifier(@3, $3)));
}
| T_ID
{
($$ = new List<NamedType*>)->Append(
	 new NamedType(
	 new Identifier(@1, $1)));
}
;

Fields:
Fields Field      { ($$ = $1)->Append($2); }
|              { $$ = new List<Decl*>; }
;

Field
: AccessMode VariableDecl        { $$ = $2; }
| AccessMode FunctionDecl        { $$ = $2; }
;

AccessMode :
T_Private
| T_Protected
| T_Public
|
;

InterfaceDecl:
T_Interface T_ID '{' Prototypes '}'
{
$$ = new InterfaceDecl(
   (new Identifier(@2, $2)), $4);
}
;

Prototypes:
Prototypes Prototype         { ($$ = $1)->Append($2); }
|              { $$ = new List<Decl*>; }
;

Prototype:
Type T_ID '(' Formals ')' ';'      { $$ = new FunctionDecl((new Identifier(@2, $2)), $1, $4); }
| T_Void T_ID '(' Formals ')' ';'
{
$$ = new FunctionDecl((new Identifier(@2, $2)),
   Type::voidType, $4);
}
;

StmtBlock:
'{' VariableDecls Stmts '}'         { $$ = new StmtBlock($2, $3); }
| '{' VariableDecls '}'     { $$ = new StmtBlock($2, new List<Stmt *>); }
;

Stmts:
Stmts Stmt    { ($$ = $1)->Append($2); }
| Stmt         { ($$ = new List<Stmt*>)->Append($1); }
;

Stmt:
ExprOpt ';'            { $$ = $1; }
| IfStmt               { $$ = $1; }
| WhileStmt            { $$ = $1; }
| ForStmt              { $$ = $1; }
| BreakStmt            { $$ = $1; }
| ContinueStmt
| ReturnStmt           { $$ = $1; }
| PrintStmt            { $$ = $1; }
| StmtBlock            { $$ = $1; }
;

IfStmt:
T_If '(' Expr ')' Stmt %prec T_NONELSE        { $$ = new IfStmt($3, $5, NULL); }
| T_If '(' Expr ')' Stmt T_Else Stmt   { $$ = new IfStmt($3, $5, $7); }
;

WhileStmt:
T_While '(' Expr ')' Stmt     { $$ = new WhileStmt($3, $5); }
;

ForStmt:
T_For '(' ExprOpt ';' Expr ';' ExprOpt ')' Stmt     { $$ = new ForStmt($3, $5, $7, $9); }
;

ReturnStmt:
T_Return Expr ';'    { $$ = new ReturnStmt(@2, $2); }
|    T_Return ';'         { $$ = new ReturnStmt(@1, new EmptyExpr()); }
;

BreakStmt:
T_Break ';'    { $$ = new BreakStmt(@1); }
;

ContinueStmt:
T_Continue ';'
;

PrintStmt:
T_Print '(' ExprPlus ')' ';'   { $$ = new PrintStmt($3); }
;

ExprOpt:
Expr     { $$ = $1; }
|              { $$ = new EmptyExpr(); }
;

ExprPlus:
ExprPlus ',' Expr    { ($$ = $1)->Append($3); }
| Expr           { ($$ = new List<Expr*>)->Append($1); }
;

Expr:
LValue '=' Expr      { $$ = new AssignExpr($1, (new Operator(@2, "=")), $3); }
| Constant             { $$ = $1; }
| LValue               { $$ = $1; }
| T_This               { $$ = new This(@1); }
| Call                 { $$ = $1; }
| '(' Expr ')'         { $$ = $2; }
| Expr '+' Expr        { $$ = new ArithmeticExpr($1, (new Operator(@2, "+")), $3); }
| Expr '-' Expr        { $$ = new ArithmeticExpr($1, (new Operator(@2, "-")), $3); }
| Expr '*' Expr        { $$ = new ArithmeticExpr($1, (new Operator(@2, "*")), $3); }
| Expr '/' Expr        { $$ = new ArithmeticExpr($1, (new Operator(@2, "/")), $3); }
| Expr '%' Expr        { $$ = new ArithmeticExpr($1, (new Operator(@2, "%")), $3); }
| '-' Expr %prec NEG   { $$ = new ArithmeticExpr((new Operator(@1, "-")), $2); }
| Expr '<' Expr        { $$ = new RelationalExpr($1, (new Operator(@2, "<")), $3); }
| Expr T_LessEqual Expr
{
$$ = new RelationalExpr($1,
   (new Operator(@2, "<=")), $3);
}
| Expr '>' Expr        { $$ = new RelationalExpr($1, (new Operator(@2, ">")), $3); }
| Expr T_GreaterEqual Expr
{
$$ = new RelationalExpr($1,
   (new Operator(@2, ">=")), $3);
}
| Expr T_Equal Expr    { $$ = new EqualityExpr($1, (new Operator(@2, "==")), $3); }
| Expr T_NotEqual Expr { $$ = new EqualityExpr($1, (new Operator(@2, "!=")), $3); }
| Expr T_And Expr      { $$ = new LogicalExpr($1, (new Operator(@2, "&&")), $3); }
| Expr T_Or Expr       { $$ = new LogicalExpr($1, (new Operator(@2, "||")), $3); }
| '!' Expr %prec NOT   { $$ = new LogicalExpr((new Operator(@1, "!")), $2); }
| T_ReadInteger '(' ')'    { $$ = new ReadIntegerExpr(Join(@1, @3)); }
| T_ReadLine '(' ')'   { $$ = new ReadLineExpr(Join(@1, @3)); }
| T_New T_ID
{
$$ = new NewExpr(Join(@1, @2), (new
   NamedType(new Identifier(@2, $2))));
}
| T_NewArray '(' Expr ',' Type ')'   { $$ = new NewArrayExpr(Join(@1, @6), $3, $5); }
| T_Itod '(' Expr ')'
| T_Dtoi '(' Expr ')'
| T_Itob '(' Expr ')'
| T_Btoi '(' Expr ')'
;

LValue:
T_ID         { $$ = new FieldAccess(NULL, (new Identifier(@1, $1))); }
| Expr '.' T_ID   { $$ = new FieldAccess($1, (new Identifier(@3, $3))); }
| Expr '[' Expr ']'    { $$ = new ArrayAccess(Join(@1, @4), $1, $3); }
;

Call:
T_ID '(' Actuals ')'
{
$$ = new Call(Join(@1, @4), NULL,
   (new Identifier(@1, $1)), $3);
}
| Expr '.' T_ID '(' Actuals ')'
{
$$ = new Call(Join(@1, @6), $1,
   (new Identifier(@3, $3)), $5);
}
;

Actuals:
ExprPlus          { $$ = $1; }
|            { $$ = new List<Expr*>; }
;

Constant:
T_INTLITERAL        { $$ = new IntLiteral(@1, $1); }
| T_DOUBLELITERAL     { $$ = new DoubleLiteral(@1, $1); }
| T_BOOLEANLITERAL       { $$ = new BoolLiteral(@1, $1); }
| T_STRINGLITERAL     { $$ = new StringLiteral(@1, $1); }
| T_Null               { $$ = new NullLiteral(@1); }
;

%%
