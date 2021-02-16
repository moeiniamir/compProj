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

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"
#include "ast_type.h"

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
    void Check(checkT c);

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
    void Check(checkT c);
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
    void Check(checkT c);
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
    void Check(checkT c);

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

#endif

