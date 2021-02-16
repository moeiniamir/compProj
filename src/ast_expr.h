/* File: ast_expr.h
 * ----------------
 * The Expr class and its subclasses are used to represent
 * expressions in the parse tree.  For each expression in the
 * language (add, call, New, etc.) there is a corresponding
 * node class for that construct.
 *
 * pp3: You will need to extend the Expr classes to implement 
 * semantic analysis for rules pertaining to expressions.
 *
 * pp5: You will need to extend the Expr classes to implement
 * code generation for expressions.
 *
 * Author: Deyuan Guo
 */


#ifndef _H_ast_expr
#define _H_ast_expr

#include "ast.h"
#include "ast_stmt.h"
#include "list.h"
#include "ast_type.h"

class NamedType; // for new
class Type; // for NewArray


class Expr : public Stmt
{
  public:
    Expr(yyltype loc) : Stmt(loc) { semantic_type = NULL; }
    Expr() : Stmt() { semantic_type = NULL; }

    // code generation
    virtual Location * GetEmitLocDeref() { return GetEmitLoc(); }
    virtual bool IsArrayAccessRef() { return false; }
    virtual bool IsEmptyExpr() { return false; }
};

/* This node type is used for those places where an expression is optional.
 * We could use a NULL pointer, but then it adds a lot of checking for
 * NULL. By using a valid, but no-op, node, we save that trouble */
class EmptyExpr : public Expr
{
  public:
    void Check(checkT c);

    // code generation
    bool IsEmptyExpr() { return true; }
};

class IntLiteral : public Expr
{
  protected:
    int value;

  public:
    IntLiteral(yyltype loc, int val);

    void Check(checkT c);

    // code generation
    void Emit();
};

class DoubleLiteral : public Expr
{
  protected:
    double value;

  public:
    DoubleLiteral(yyltype loc, double val);

    void Check(checkT c);

    // code generation
    void Emit();
};

class BoolLiteral : public Expr
{
  protected:
    bool value;

  public:
    BoolLiteral(yyltype loc, bool val);

    void Check(checkT c);

    // code generation
    void Emit();
};

class StringLiteral : public Expr
{
  protected:
    char *value;

  public:
    StringLiteral(yyltype loc, const char *val);

    void Check(checkT c);

    // code generation
    void Emit();
};

class NullLiteral: public Expr
{
  public:
    NullLiteral(yyltype loc) : Expr(loc) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class Operator : public Node
{
  protected:
    char tokenString[4];

  public:
    Operator(yyltype loc, const char *tok);
    friend std::ostream& operator<<(std::ostream& out, Operator *o)
        { return out << o->tokenString; }

    const char * GetOpStr() { return tokenString; }
 };

class CompoundExpr : public Expr
{
  protected:
    Operator *op;
    Expr *left, *right; // left will be NULL if unary

  public:
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs); // for binary
    CompoundExpr(Operator *op, Expr *rhs);             // for unary

};

class ArithmeticExpr : public CompoundExpr
{
  protected:
    void CheckType();

  public:
    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class RelationalExpr : public CompoundExpr
{
  protected:
    void CheckType();

  public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class EqualityExpr : public CompoundExpr
{
  protected:
    void CheckType();

  public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class LogicalExpr : public CompoundExpr
{
  protected:
    void CheckType();

  public:
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class AssignExpr : public CompoundExpr
{
  protected:
    void CheckType();

  public:
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class LValue : public Expr
{
  public:
    LValue(yyltype loc) : Expr(loc) {}
};

class This : public Expr
{
  protected:
    void CheckType();

  public:
    This(yyltype loc) : Expr(loc) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class ArrayAccess : public LValue
{
  protected:
    Expr *base, *subscript;
    void CheckType();

  public:
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);

    void Check(checkT c);

    // code generation
    void Emit();
    bool IsArrayAccessRef() { return true; }
    Location * GetEmitLocDeref();
};

/* Note that field access is used both for qualified names
 * base.field and just field without qualification. We don't
 * know for sure whether there is an implicit "this." in
 * front until later on, so we use one node type for either
 * and sort it out later. */
class FieldAccess : public LValue
{
  protected:
    Expr *base; // will be NULL if no explicit base
    Identifier *field;
    void CheckDecl();
    void CheckType();

  public:
    FieldAccess(Expr *base, Identifier *field); //ok to pass NULL base

    void Check(checkT c);

    // code generation
    void Emit();
    Location * GetEmitLocDeref();
};

/* Like field access, call is used both for qualified base.field()
 * and unqualified field().  We won't figure out until later
 * whether we need implicit "this." so we use one node type for either
 * and sort it out later. */
class Call : public Expr
{
  protected:
    Expr *base; // will be NULL if no explicit base
    Identifier *field;
    List<Expr*> *actuals;
    void CheckDecl();
    void CheckType();
    void CheckFuncArgs();

  public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr*> *args);

    void Check(checkT c);

    // code generation
    void Emit();
};

class NewExpr : public Expr
{
  protected:
    NamedType *cType;
    void CheckDecl();
    void CheckType();

  public:
    NewExpr(yyltype loc, NamedType *clsType);

    void Check(checkT c);

    // code generation
    void Emit();
};

class NewArrayExpr : public Expr
{
  protected:
    Expr *size;
    Type *elemType;
    void CheckType();

  public:
    NewArrayExpr(yyltype loc, Expr *sizeExpr, Type *elemType);

    void Check(checkT c);

    // code generation
    void Emit();
};

class ReadIntegerExpr : public Expr
{
  public:
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    void Check(checkT c);

    // code generation
    void Emit();
};

class ReadLineExpr : public Expr
{
  public:
    ReadLineExpr(yyltype loc) : Expr (loc) {}
    void Check(checkT c);

    // code generation
    void Emit();
};


class PostfixExpr : public Expr
{
  protected:
    LValue *lvalue;
    Operator *op;
    void CheckType();

  public:
    PostfixExpr(LValue *lv, Operator *op);

    void Check(checkT c);

    // code generation
    void Emit();
};

#endif

