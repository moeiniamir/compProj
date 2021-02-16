/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"


/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error");

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
    semantic_type = NULL;
}

void Type::PrintChildren(int indentLevel) {
    printf("%s", typeName);
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
}

void Type::Check(checkT c) {
    if (c == E_CheckDecl) {
        Type::intType->SetSelfType();
        Type::doubleType->SetSelfType();
        Type::voidType->SetSelfType();
        Type::boolType->SetSelfType();
        Type::nullType->SetSelfType();
        Type::stringType->SetSelfType();
        Type::errorType->SetSelfType();
        semantic_type = this;
    }
}

NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
}

void NamedType::PrintChildren(int indentLevel) {
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
    id->Print(indentLevel+1);
}

void NamedType::CheckDecl(reasonT r) {
    Decl *d = symtab->Lookup(this->id);
    if (d == NULL || (!d->IsClassDecl() && !d->IsInterfaceDecl())) {
        semantic_error = 1;
        return;
    } else if (r == LookingForClass && !d->IsClassDecl()) {
        semantic_error = 1;
        return;
    } else if (r == LookingForInterface && !d->IsInterfaceDecl()) {
        semantic_error = 1;
        return;
    } else {
        this->id->SetDecl(d);
        semantic_type = this;
    }
}

void NamedType::Check(checkT c, reasonT r) {
    if (c == E_CheckDecl) {
        this->CheckDecl(r);
    } else {
        id->Check(c);
    }
}

bool NamedType::IsEquivalentTo(Type *other) {
    Assert(this->GetType() && other->GetType());

    if (!other->IsNamedType()) {
        return false;
    }
    NamedType * nt = dynamic_cast<NamedType*>(other);
    return (id->IsEquivalentTo(nt->GetId()));
}

/* NamedType A IsCompatibleWith NamedType B,
 * means that A = B,
 * or class B is the subclass of class A,
 * or class B or its parents implement interface A.
 */
bool NamedType::IsCompatibleWith(Type *other) {
    Assert(this->GetType() && other->GetType());

    if (other == nullType) {
        return true;
    } else if (!other->IsNamedType()) {
        return false;
    } else if (this->IsEquivalentTo(other)) {
        return true;
    } else {
        NamedType * nt = dynamic_cast<NamedType*>(other);
        Decl *decl1 = id->GetDecl();
        Decl *decl2 = nt->GetId()->GetDecl();
        Assert(decl1 && decl2);
        if (!decl2->IsClassDecl()) {
            return false;
        }
        ClassDecl *cdecl2 = dynamic_cast<ClassDecl*>(decl2);
        // subclass can compatible with its parent class and interface.
        return cdecl2->IsChildOf(decl1);
    }
}

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}
void ArrayType::PrintChildren(int indentLevel) {
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
    elemType->Print(indentLevel+1);
}

void ArrayType::CheckDecl() {
    elemType->Check(E_CheckDecl);
    if (elemType->GetType()) {
        semantic_type = this;
    }
}

void ArrayType::Check(checkT c) {
    if (c == E_CheckDecl) {
        this->CheckDecl();
    } else {
        elemType->Check(c);
    }
}

bool ArrayType::IsEquivalentTo(Type *other) {
    Assert(this->GetType() && other->GetType());

    if (!other->IsArrayType()) {
        return false;
    }
    ArrayType * nt = dynamic_cast<ArrayType*>(other);
    return (elemType->IsEquivalentTo(nt->GetElemType()));
}

bool ArrayType::IsCompatibleWith(Type *other) {
    Assert(this->GetType() && other->GetType());

    if (other == nullType) {
        return elemType->IsCompatibleWith(other);
    } else {
        return this->IsEquivalentTo(other);
    }
}

