/* File: ast.h
 * -----------
 * This file defines the abstract base class Node and the concrete
 * Identifier and Error node subclasses that are used through the tree as
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or,
 * more correctly, of instances of concrete subclassses such as VariableDecl,
 * ForStmt, and AssignExpr).
 *
 * Location: Each node maintains its lexical location (line and columns in
 * file), that location can be NULL for those nodes that don't care/use
 * locations. The location is typcially set by the node constructor.  The
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a
 * bottom-up parse we don't know the parent at the time of construction) but
 * instead we wait until assigning the children into the parent node and then
 * set up links in both directions. The parent link is typically not used
 * during parsing, but is more important in later phases.
 *
 * Printing: The only interesting behavior of the node classes for pp2 is the
 * bility to print the tree using an in-order walk.  Each node class is
 * responsible for printing itself/children by overriding the virtual
 * PrintChildren() and GetPrintNameForNode() methods. All the classes we
 * provide already implement these methods, so your job is to construct the
 * nodes and wire them up during parsing. Once that's done, printing is a snap!
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.
 *
 * Code generation: For pp5 you are adding "Emit" behavior to the ast
 * node classes. Your code generator should do an postorder walk on the
 * parse tree, and when visiting each node, emitting the necessary
 * instructions for that construct.
 *
 * Author: Deyuan Guo
 */

#ifndef _H_ast
#define _H_ast

#include <stdlib.h>   // for NULL
#include <iostream>
#include "scopeHandler.h"
#include "globals.h"
#include "codegen.h"
#include "ds.h"
#include <iostream>



// the global code generator class.
extern CodeGenerator *CG;

class Node
{
  protected:
    yyltype *location;
    Node *parent;
    Type *semantic_type; // link to the type of each node (not for stmt)
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

    // code generation
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

    // code generation
    void Emit();
    void AddPrefix(const char *prefix);
    Location * GetEmitLocDeref() { return GetEmitLoc(); }
};


// This node class is designed to represent a portion of the tree that
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node
{
  public:
    Error() : Node() {}
};


/* File: ast_type.h
 * ----------------
 * In our parse tree, Type nodes are used to represent and
 * store type information. The base Type class is used
 * for built-in types, the NamedType for classes and interfaces,
 * and the ArrayType for arrays of other types.
 *
 * pp3: You will need to extend the Type classes to implement
 * the type system and rules for type equivalency and compatibility.
 *
 * pp5: You will need to extend the Type classes to implement
 * code generation for types.
 *
 * Author: Deyuan Guo
 */



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

    // code generation
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



/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement
 * semantic processing including detection of declaration conflicts
 * and managing scoping issues.
 *
 * pp5: You will need to extend the Decl classes to implement
 * code generation for declarations.
 *
 * Author: Deyuan Guo
 */


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

    // code generation
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

    // code generation
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

    // code generation
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

    // code generation
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

    // code generation
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

    void BuildSymTable();
    void Check(checkStep c);

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


    void BuildSymTable();
    void Check(checkStep c);

    // code generation
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


    void BuildSymTable();
    void Check(checkStep c);

    // code generation
    void Emit();
};

class BreakStmt : public Stmt
{
public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    void Check(checkStep c);

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


    void BuildSymTable();
    void Check(checkStep c);

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


    void BuildSymTable();
    void Check(checkStep c);

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


    void Check(checkStep c);

    // code generation
    void Emit();
};

class PrintStmt : public Stmt
{
protected:
    List<Expr*> *args;

public:
    PrintStmt(List<Expr*> *arguments);


    void Check(checkStep c);

    // code generation
    void Emit();
};





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
    void Check(checkStep c);

    // code generation
    bool IsEmptyExpr() { return true; }
};

class IntLiteral : public Expr
{
protected:
    int value;

public:
    IntLiteral(yyltype loc, int val);

    void Check(checkStep c);

    // code generation
    void Emit();
};

class DoubleLiteral : public Expr
{
protected:
    double value;

public:
    DoubleLiteral(yyltype loc, double val);

    void Check(checkStep c);

    // code generation
    void Emit();
};

class BoolLiteral : public Expr
{
protected:
    bool value;

public:
    BoolLiteral(yyltype loc, bool val);

    void Check(checkStep c);

    // code generation
    void Emit();
};

class StringLiteral : public Expr
{
protected:
    char *value;

public:
    StringLiteral(yyltype loc, const char *val);

    void Check(checkStep c);

    // code generation
    void Emit();
};

class NullLiteral: public Expr
{
public:
    NullLiteral(yyltype loc) : Expr(loc) {}
    void Check(checkStep c);

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
    void Check(checkStep c);

    // code generation
    void Emit();
};

class RelationalExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);

    // code generation
    void Emit();
};

class EqualityExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);

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
    void Check(checkStep c);

    // code generation
    void Emit();
};

class AssignExpr : public CompoundExpr
{
protected:
    void CheckType();

public:
    AssignExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check(checkStep c);

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
    void Check(checkStep c);

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

    void Check(checkStep c);

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

    void Check(checkStep c);

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

    void Check(checkStep c);

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

    void Check(checkStep c);

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

    void Check(checkStep c);

    // code generation
    void Emit();
};

class ReadIntegerExpr : public Expr
{
public:
    ReadIntegerExpr(yyltype loc) : Expr(loc) {}
    void Check(checkStep c);

    // code generation
    void Emit();
};

class ReadLineExpr : public Expr
{
public:
    ReadLineExpr(yyltype loc) : Expr (loc) {}
    void Check(checkStep c);

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

    void Check(checkStep c);

    // code generation
    void Emit();
};




#endif

