

#ifndef _H_ast
#define _H_ast

#include <stdlib.h>
#include <iostream>
#include "scopeHandler.h"
#include "globals.h"
#include "codegen.h"
#include "ds.h"
#include <iostream>




extern CodeGenerator *CG;

class Node
{
  protected:
    yyltype *location;
    Node *parent;
    Type *semantic_type;
    Location *asm_loc;

  public:
    Node(yyltype loc);
    Node();

    yyltype *GetLocation()   { return location; }
    void SetParent(Node *p)  { parent = p; }
    Node *GetParent()        { return parent; }


    virtual void BuildSymTable() {}
    virtual void Check(checkStep c) {}
    virtual Type * GetType() { return semantic_type; }
    virtual bool IsLoopStmt() { return false; }
    virtual bool IsSwitchStmt() { return false; }
    virtual bool IsCaseStmt() { return false; }


    virtual void Emit() {}
    virtual Location * GetEmitLoc() { return asm_loc; }
};


class Identifier : public Node
{
  protected:
    char *name;
    Decl *decl;
    void CheckDecl();

  public:
    Identifier(yyltype loc, const char *name);
    friend std::ostream& operator<<(std::ostream& out, Identifier *id)
        { return out << id->name; }

    char *GetIdName() { return name; }
    void Check(checkStep c);
    bool IsEquivalentTo(Identifier *other);
    void SetDecl(Decl *d) { decl = d; }
    Decl * GetDecl() { return decl; }


    void Emit();
    void AddPrefix(const char *prefix);
    Location * GetEmitLocDeref() { return GetEmitLoc(); }
};







class Error : public Node
{
  public:
    Error() : Node() {}
};






class Type : public Node
{
protected:
    char *typeName;

public :
    static Type *intType, *doubleType, *boolType, *voidType,
            *nullType, *stringType, *errorType;

    Type(yyltype loc) : Node(loc) { semantic_type = NULL; }
    Type(const char *str);



    virtual void PrintToStream(std::ostream& out) { out << typeName; }
    friend std::ostream& operator<<(std::ostream& out, Type *t)
    { t->PrintToStream(out); return out; }
    virtual bool IsEquivalentTo(Type *other) { return this == other; }
    virtual bool IsCompatibleWith(Type *other) { return this == other; }

    char * GetTypeName() { return typeName; }

    virtual bool IsBasicType()
    { return !this->IsNamedType() && !this->IsArrayType(); }
    virtual bool IsNamedType() { return false; }
    virtual bool IsArrayType() { return false; }
    void Check(checkStep c);
    virtual void Check(checkStep c, checkFor r) { Check(c); }
    virtual void SetSelfType() { semantic_type = this; }


    virtual int GetTypeSize() { return 4; }
};

class NamedType : public Type
{
protected:
    Identifier *id;
    void CheckDecl(checkFor r);

public:
    NamedType(Identifier *i);



    void PrintToStream(std::ostream& out) { out << id; }

    Identifier *GetId() { return id; }

    bool IsEquivalentTo(Type *other);
    bool IsCompatibleWith(Type *other);

    bool IsNamedType() { return true; }
    void Check(checkStep c, checkFor r);
    void Check(checkStep c) { Check(c, typeReason); }
};

class ArrayType : public Type
{
protected:
    Type *elemType;
    void CheckDecl();

public:
    ArrayType(yyltype loc, Type *elemType);



    void PrintToStream(std::ostream& out) { out << elemType << "[]"; }

    Type * GetElemType() { return elemType; }
    bool IsEquivalentTo(Type *other);
    bool IsCompatibleWith(Type *other);

    bool IsArrayType() { return true; }
    void Check(checkStep c);
};






class Type;
class NamedType;
class Identifier;
class Stmt;
class FunctionDecl;

class Decl : public Node
{
protected:
    Identifier *id;
    int idx;

public:
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d)
    { return out << d->id; }

    Identifier *GetId() { return id; }
    int GetIndex() { return idx; }

    virtual bool IsVariableDecl() { return false; }
    virtual bool IsClassDecl() { return false; }
    virtual bool IsInterfaceDecl() { return false; }
    virtual bool IsFunctionDecl() { return false; }


    virtual void AssignOffset() {}
    virtual void AssignMemberOffset(bool inClass, int offset) {}
    virtual void AddPrefixToMethods() {}
};

class VariableDecl : public Decl
{
protected:
    Type *type;
    bool is_global;
    int class_member_ofst;
    void CheckDecl();
    bool IsGlobal() { return this->GetParent()->GetParent() == NULL; }
    bool IsClassMember() {
        Decl *d = dynamic_cast<Decl*>(this->GetParent());
        return d ? d->IsClassDecl() : false;
    }

public:
    VariableDecl(Identifier *name, Type *type);

    Type * GetType() { return type; }
    bool IsVariableDecl() { return true; }

    void BuildSymTable();
    void Check(checkStep c);


    void AssignOffset();
    void AssignMemberOffset(bool inClass, int offset);
    void Emit();
    void SetEmitLoc(Location *l) { asm_loc = l; }
};

class ClassDecl : public Decl
{
protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    int instance_size;
    int vtable_size;
    List<VariableDecl*> *var_members;
    List<FunctionDecl*> *methods;
    void CheckDecl();
    void CheckInherit();

public:
    ClassDecl(Identifier *name, NamedType *extends,
              List<NamedType*> *implements, List<Decl*> *members);

    bool IsClassDecl() { return true; }
    void BuildSymTable();
    void Check(checkStep c);
    bool IsChildOf(Decl *other);
    NamedType * GetExtends() { return extends; }


    void AssignOffset();
    void Emit();
    int GetInstanceSize() { return instance_size; }
    int GetVTableSize() { return vtable_size; }
    void AddMembersToList(List<VariableDecl*> *vars, List<FunctionDecl*> *fns);
    void AddPrefixToMethods();
};

class InterfaceDecl : public Decl
{
protected:
    List<Decl*> *members;
    void CheckDecl();

public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);

    bool IsInterfaceDecl() { return true; }
    void BuildSymTable();
    void Check(checkStep c);
    List<Decl*> * GetMembers() { return members; }


    void Emit();
};

class FunctionDecl : public Decl
{
protected:
    List<VariableDecl*> *formals;
    Type *returnType;
    Stmt *body;
    int vtable_ofst;
    void CheckDecl();

public:
    FunctionDecl(Identifier *name, Type *returnType, List<VariableDecl*> *formals);
    void SetFunctionBody(Stmt *b);

    Type * GetReturnType() { return returnType; }
    List<VariableDecl*> * GetFormals() { return formals; }

    bool IsFunctionDecl() { return true; }
    bool IsEquivalentTo(Decl *fn);

    void BuildSymTable();
    void Check(checkStep c);


    void AddPrefixToMethods();
    void AssignMemberOffset(bool inClass, int offset);
    void Emit();
    int GetVTableOffset() { return vtable_ofst; }
    bool HasReturnValue() { return returnType != Type::voidType; }
    bool IsClassMember() {
        Decl *d = dynamic_cast<Decl*>(this->GetParent());
        return d ? d->IsClassDecl() : false;
    }
};






class Decl;
class VariableDecl;
class Expr;

class Program : public Node
{
protected:
    List<Decl*> *decls;

public:
    Program(List<Decl*> *declList);

    void BuildSymTable();
    void Check();
    void Check(checkStep c) { Check(); }


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

    void BuildSymTable();
    void Check(checkStep c);


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


    virtual const char * GetEndLoopLabel() { return end_loop_label; }
};

class ForStmt : public LoopStmt
{
protected:
    Expr *init, *step;
    void CheckType();

public:
    ForStmt(Expr *init, Expr *test, Expr *step, Stmt *body);


    void BuildSymTable();
    void Check(checkStep c);


    void Emit();
};

class WhileStmt : public LoopStmt
{
protected:
    void CheckType();

public:
    WhileStmt(Expr *test, Stmt *body) : LoopStmt(test, body) {}


    void BuildSymTable();
    void Check(checkStep c);


    void Emit();
};

class IfStmt : public ConditionalStmt
{
protected:
    Stmt *elseBody;
    void CheckType();

public:
    IfStmt(Expr *test, Stmt *thenBody, Stmt *elseBody);


    void BuildSymTable();
    void Check(checkStep c);


    void Emit();
};

class BreakStmt : public Stmt
{
public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    void Check(checkStep c);


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


    void BuildSymTable();
    void Check(checkStep c);


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


    void BuildSymTable();
    void Check(checkStep c);


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


    void Check(checkStep c);


    void Emit();
};

class PrintStmt : public Stmt
{
protected:
    List<Expr*> *args;

public:
    PrintStmt(List<Expr*> *arguments);


    void Check(checkStep c);


    void Emit();
};








class NamedType;
class Type;


class Expr : public Stmt
{
public:
    Expr(yyltype loc) : Stmt(loc) { semantic_type = NULL; }
    Expr() : Stmt() { semantic_type = NULL; }


    virtual Location * GetEmitLocDeref() { return GetEmitLoc(); }
    virtual bool IsArrayAccessRef() { return false; }
    virtual bool IsEmptyExpr() { return false; }
};


class EmptyExpr : public Expr
{
public:
    void Check(checkStep c);


    bool IsEmptyExpr() { return true; }
};

class IntLiteral : public Expr
{
protected:
    int value;

public:
    IntLiteral(yyltype loc, int val);

    void Check(checkStep c);


    void Emit();
};

class DoubleLiteral : public Expr
{
protected:
    double value;

public:
    DoubleLiteral(yyltype loc, double val);

    void Check(checkStep c);


    void Emit();
};

class BoolLiteral : public Expr
{
protected:
    bool value;

public:
    BoolLiteral(yyltype loc, bool val);

    void Check(checkStep c);


    void Emit();
};

class StringLiteral : public Expr
{
protected:
    char *value;

public:
    StringLiteral(yyltype loc, const char *val);

    void Check(checkStep c);


    void Emit();
};

class NullLiteral: public Expr
{
public:
    NullLiteral(yyltype loc) : Expr(loc) {}
    void Check(checkStep c);


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
    Expr *left, *right;

public:
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs);
    CompoundExpr(Operator *op, Expr *rhs);

};

class ArithmeticExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    void Check(checkStep c);


    void Emit();
};

class RelationalExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);


    void Emit();
};

class EqualityExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);


    void Emit();
};

class LogicalExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    void Check(checkStep c);


    void Emit();
};

class AssignExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);


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
    void Check(checkStep c);


    void Emit();
};

class ArrayAccess : public LValue
{
protected:
    Expr *base, *subscript;
    void CheckType();

public:
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);

    void Check(checkStep c);


    void Emit();
    bool IsArrayAccessRef() { return true; }
    Location * GetEmitLocDeref();
};


class FieldAccess : public LValue
{
protected:
    Expr *base;
    Identifier *field;
    void CheckDecl();
    void CheckType();

public:
    FieldAccess(Expr *base, Identifier *field);

    void Check(checkStep c);


    void Emit();
    Location * GetEmitLocDeref();
};


class Call : public Expr
{
protected:
    Expr *base;
    Identifier *field;
    List<Expr*> *actuals;
    void CheckDecl();
    void CheckType();
    void CheckFuncArgs();

public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr*> *args);

    void Check(checkStep c);


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

    void Check(checkStep c);


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

    void Check(checkStep c);


    void Emit();
};

class ReadIntegerExpr : public Expr
{
public:
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    void Check(checkStep c);


    void Emit();
};

class ReadLineExpr : public Expr
{
public:
    ReadLineExpr(yyltype loc) : Expr (loc) {}
    void Check(checkStep c);


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

    void Check(checkStep c);


    void Emit();
};




#endif

