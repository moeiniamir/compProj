/* File: ast_stmt.h
 * ----------------
 * The Stmt class and its subclasses are used to represent
 * statements in the parse tree.  For each statment in the
 * language (for, if, return, etc.) there is a corresponding
 * node class for that construct.
 *
 * pp2: You will need to add new expression and statement node c
 * classes for the additional grammar elements (Switch/Postfix)
 *
 * pp3: You will need to extend the Stmt classes to implement
 * semantic analysis for rules pertaining to statements.
 *
 * pp5: You will need to extend the Stmt classes to implement
 * code generation for statements.
 *
 * Author: Deyuan Guo
 */


#ifndef _H_ast_stmt
#define _H_ast_stmt

#include "list.h"
#include "ast.h"

class Decl;
class VariableDecl;
class Expr;

class Program : public Node
{
  protected:
     List<Decl*> *decls;

  public:
     Program(List<Decl*> *declList);
     const char *GetPrintNameForNode() { return "Program"; }
     void PrintChildren(int indentLevel);

     void BuildST();
     void Check();
     void Check(checkT c) { Check(); }

    // code generation
    void Emit();
};

class Stmt : public Node
{
  public:
     Stmt() : Node() {}
     Stmt(yyltype loc) : Node(loc) {}
};

class StmtBlock : public Stmt
{
  protected:
    List<VariableDecl*> *decls;
    List<Stmt*> *stmts;

  public:
    StmtBlock(List<VariableDecl*> *variableDeclarations, List<Stmt*> *statements);
    const char *GetPrintNameForNode() { return "StmtBlock"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
};


class ConditionalStmt : public Stmt
{
  protected:
    Expr *test;
    Stmt *body;

  public:
    ConditionalStmt(Expr *testExpr, Stmt *body);
};

class LoopStmt : public ConditionalStmt
{
  protected:
    const char *end_loop_label;

  public:
    LoopStmt(Expr *testExpr, Stmt *body)
            : ConditionalStmt(testExpr, body) {}
    bool IsLoopStmt() { return true; }

    // code generation
    virtual const char * GetEndLoopLabel() { return end_loop_label; }
};

class ForStmt : public LoopStmt
{
  protected:
    Expr *init, *step;
    void CheckType();

  public:
    ForStmt(Expr *init, Expr *test, Expr *step, Stmt *body);
    const char *GetPrintNameForNode() { return "ForStmt"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
};

class WhileStmt : public LoopStmt
{
  protected:
    void CheckType();

  public:
    WhileStmt(Expr *test, Stmt *body) : LoopStmt(test, body) {}
    const char *GetPrintNameForNode() { return "WhileStmt"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
};

class IfStmt : public ConditionalStmt
{
  protected:
    Stmt *elseBody;
    void CheckType();

  public:
    IfStmt(Expr *test, Stmt *thenBody, Stmt *elseBody);
    const char *GetPrintNameForNode() { return "IfStmt"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
};

class BreakStmt : public Stmt
{
  public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    const char *GetPrintNameForNode() { return "BreakStmt"; }
    void Check(checkT c);

    // code generation
    void Emit();
};

class IntLiteral;

class CaseStmt : public Stmt
{
  protected:
    IntLiteral *value;
    List<Stmt*> *stmts;
    const char *case_label;

  public:
    CaseStmt(IntLiteral *v, List<Stmt*> *stmts);
    const char *GetPrintNameForNode() { return value ? "Case" : "Default"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
    void GenCaseLabel();
    const char * GetCaseLabel() { return case_label; }
    IntLiteral * GetCaseValue() { return value; }
};

class SwitchStmt : public Stmt
{
  protected:
    Expr *expr;
    List<CaseStmt*> *cases;
    const char *end_switch_label;

  public:
    SwitchStmt(Expr *expr, List<CaseStmt*> *cases);
    const char *GetPrintNameForNode() { return "SwitchStmt"; }
    void PrintChildren(int indentLevel);

    void BuildST();
    void Check(checkT c);

    // code generation
    void Emit();
    bool IsSwitchStmt() { return true; }
    const char * GetEndSwitchLabel() { return end_switch_label; }
};

class ReturnStmt : public Stmt
{
  protected:
    Expr *expr;

  public:
    ReturnStmt(yyltype loc, Expr *expr);
    const char *GetPrintNameForNode() { return "ReturnStmt"; }
    void PrintChildren(int indentLevel);

    void Check(checkT c);

    // code generation
    void Emit();
};

class PrintStmt : public Stmt
{
  protected:
    List<Expr*> *args;

  public:
    PrintStmt(List<Expr*> *arguments);
    const char *GetPrintNameForNode() { return "PrintStmt"; }
    void PrintChildren(int indentLevel);

    void Check(checkT c);

    // code generation
    void Emit();
};


#endif

