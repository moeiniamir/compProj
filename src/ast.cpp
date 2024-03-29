

#include "ast.h"
#include <string.h>
#include <stdio.h>
#include "globals.h"
#include "ds.h"
#include <iostream>


CodeGenerator *CG = new CodeGenerator();

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}


Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
}

void Identifier::CheckDecl() {
    Decl *d = scopeHandler->Lookup(this);
    if (d == NULL) {
        semantic_error = 1;
        return;
    } else {
        this->SetDecl(d);
    }
}

void Identifier::Check(checkStep c) {
    if (c == sem_decl) {
        this->CheckDecl();
    }
}

bool Identifier::IsEquivalentTo(Identifier *other) {
    bool eq = false;
    if (!strcmp(name, other->GetIdName())) eq = true;
    return eq;
}

void Identifier::Emit() {
    if (decl)
        asm_loc = decl->GetEmitLoc();
}

void Identifier::AddPrefix(const char *prefix) {
    char *s = (char *) malloc(strlen(name) + strlen(prefix) + 1);
    sprintf(s, "%s%s", prefix, name);
    name = s;
}


Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {

    (id = n)->SetParent(this);
    idx = -1;
    semantic_type = NULL;
}


VariableDecl::VariableDecl(Identifier *n, Type *t) : Decl(n) {

    (type = t)->SetParent(this);
    class_member_ofst = -1;
}

void VariableDecl::BuildSymTable() {
    if (scopeHandler->LocalLookup(this->GetId())) {
        Decl *d = scopeHandler->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = scopeHandler->InsertSymbol(this);
        id->SetDecl(this);
    }
}

void VariableDecl::CheckDecl() {
    type->Check(sem_decl);
    id->Check(sem_decl);

    semantic_type = type->GetType();
}

void VariableDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        default:
            type->Check(c);
            id->Check(c);
    }
}

void VariableDecl::AssignOffset() {
    if (this->IsGlobal()) {
        asm_loc = new Location(gpRelative, CG->GetNextGlobalLoc(),
                               id->GetIdName());
    }
}

void VariableDecl::AssignMemberOffset(bool inClass, int offset) {
    class_member_ofst = offset;

    asm_loc = new Location(fpRelative, offset, id->GetIdName(), CG->ThisPtr);
}

void VariableDecl::Emit() {
    if (type == Type::doubleType) {
        semantic_error = 1;
        return;
    }

    if (!asm_loc) {

        asm_loc = new Location(fpRelative, CG->GetNextLocalLoc(),
                               id->GetIdName());
    }
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType *> *imp, List<Decl *> *m) : Decl(n) {


    extends = ex;
    if (extends) extends->SetParent(this);
    (implements = imp)->SetParentAll(this);
    (members = m)->SetParentAll(this);
    instance_size = 4;
    vtable_size = 0;
}

void ClassDecl::BuildSymTable() {
    if (scopeHandler->LocalLookup(this->GetId())) {

        Decl *d = scopeHandler->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = scopeHandler->InsertSymbol(this);
        id->SetDecl(this);
    }

    scopeHandler->BuildScope(this->GetId()->GetIdName());
    if (extends) {

        scopeHandler->SetScopeParent(extends->GetId()->GetIdName());
    }

    for (int i = 0; i < implements->NumElements(); i++) {
        scopeHandler->SetInterface(implements->Nth(i)->GetId()->GetIdName());
    }
    members->BuildSymTableAll();
    scopeHandler->ExitScope();
}

void ClassDecl::CheckDecl() {
    id->Check(sem_decl);

    if (extends) {
        extends->Check(sem_decl, classReason);
    }

    for (int i = 0; i < implements->NumElements(); i++) {
        implements->Nth(i)->Check(sem_decl, interfaceReason);
    }
    scopeHandler->EnterScope();
    members->CheckAll(sem_decl);
    scopeHandler->ExitScope();

    semantic_type = new NamedType(id);
    semantic_type->SetSelfType();
}

void ClassDecl::CheckInherit() {
    scopeHandler->EnterScope();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);


        if (d->IsVariableDecl()) {

            Decl *t = scopeHandler->LookupParent(d->GetId());
            if (t != NULL) {

                semantic_error = 1;
                return;
            }

            t = scopeHandler->LookupInterface(d->GetId());
            if (t != NULL) {

                semantic_error = 1;
                return;
            }

        } else if (d->IsFunctionDecl()) {

            Decl *t = scopeHandler->LookupParent(d->GetId());
            if (t != NULL) {
                if (!t->IsFunctionDecl()) {
                    semantic_error = 1;
                    return;
                } else {

                    FunctionDecl *fn1 = dynamic_cast<FunctionDecl *>(d);
                    FunctionDecl *fn2 = dynamic_cast<FunctionDecl *>(t);
                    if (fn1->GetType() && fn2->GetType()
                        && !fn1->IsEquivalentTo(fn2)) {

                        semantic_error = 1;
                        return;
                    }
                }
            }

            t = scopeHandler->LookupInterface(d->GetId());
            if (t != NULL) {

                FunctionDecl *fn1 = dynamic_cast<FunctionDecl *>(d);
                FunctionDecl *fn2 = dynamic_cast<FunctionDecl *>(t);
                if (fn1->GetType() && fn2->GetType()
                    && !fn1->IsEquivalentTo(fn2)) {

                    semantic_error = 1;
                    return;
                }
            }

            d->Check(sem_inh);
        }
    }


    for (int i = 0; i < implements->NumElements(); i++) {
        Decl *d = implements->Nth(i)->GetId()->GetDecl();
        if (d != NULL) {
            List<Decl *> *m = dynamic_cast<InterfaceDecl *>(d)->GetMembers();

            for (int j = 0; j < m->NumElements(); j++) {
                Identifier *mid = m->Nth(j)->GetId();
                Decl *t = scopeHandler->LookupField(this->id, mid);
                if (t == NULL) {
                    semantic_error = 1;
                    return;
                    break;
                } else {

                    FunctionDecl *fn1 = dynamic_cast<FunctionDecl *>(m->Nth(j));
                    FunctionDecl *fn2 = dynamic_cast<FunctionDecl *>(t);
                    if (!fn1 || !fn2 || !fn1->GetType() || !fn2->GetType()
                        || !fn1->IsEquivalentTo(fn2)) {
                        semantic_error = 1;
                        return;
                        break;
                    }
                }
            }
        }
    }
    scopeHandler->ExitScope();
}

void ClassDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        case sem_inh:
            this->CheckInherit();
            break;
        default:
            id->Check(c);
            if (extends) extends->Check(c);
            implements->CheckAll(c);
            scopeHandler->EnterScope();
            members->CheckAll(c);
            scopeHandler->ExitScope();
    }
}

bool ClassDecl::IsChildOf(Decl *other) {
    if (other->IsClassDecl()) {
        if (id->IsEquivalentTo(other->GetId())) {

            return true;
        } else if (!extends) {
            return false;
        } else {

            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl *>(d)->IsChildOf(other);
        }
    } else if (other->IsInterfaceDecl()) {
        for (int i = 0; i < implements->NumElements(); i++) {
            Identifier *iid = implements->Nth(i)->GetId();
            if (iid->IsEquivalentTo(other->GetId())) {
                return true;
            }
        }
        if (!extends) {
            return false;
        } else {

            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl *>(d)->IsChildOf(other);
        }
    } else {
        return false;
    }
}

void ClassDecl::AddMembersToList(List<VariableDecl *> *vars, List<FunctionDecl *> *fns) {
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVariableDecl()) {
            vars->InsertAt(dynamic_cast<VariableDecl *>(d), 0);
        } else if (d->IsFunctionDecl()) {
            fns->InsertAt(dynamic_cast<FunctionDecl *>(d), 0);
        }
    }
}

void ClassDecl::AssignOffset() {


    var_members = new List<VariableDecl *>;
    methods = new List<FunctionDecl *>;
    ClassDecl *c = this;
    while (c) {
        c->AddMembersToList(var_members, methods);
        NamedType *t = c->GetExtends();
        if (!t) break;
        c = dynamic_cast<ClassDecl *>(t->GetId()->GetDecl());
    }


    for (int i = 0; i < methods->NumElements(); i++) {
        FunctionDecl *f1 = methods->Nth(i);
        for (int j = i + 1; j < methods->NumElements(); j++) {
            FunctionDecl *f2 = methods->Nth(j);
            if (!strcmp(f1->GetId()->GetIdName(), f2->GetId()->GetIdName())) {


                methods->RemoveAt(i);
                methods->InsertAt(f2, i);
                methods->RemoveAt(j);
                j--;
            }
        }
    }


    for (int i = 0; i < methods->NumElements(); i++) {

    }

    for (int i = 0; i < var_members->NumElements(); i++) {

    }


    instance_size = var_members->NumElements() * 4 + 4;
    vtable_size = methods->NumElements() * 4;

    int var_offset = instance_size;
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVariableDecl()) {
            var_offset -= 4;
            d->AssignMemberOffset(true, var_offset);
        } else if (d->IsFunctionDecl()) {

            for (int i = 0; i < methods->NumElements(); i++) {
                FunctionDecl *f1 = methods->Nth(i);
                if (!strcmp(f1->GetId()->GetIdName(), d->GetId()->GetIdName()))
                    d->AssignMemberOffset(true, i * 4);
            }
        }
    }
}

void ClassDecl::AddPrefixToMethods() {

    for (int i = 0; i < members->NumElements(); i++) {
        members->Nth(i)->AddPrefixToMethods();
    }
}

void ClassDecl::Emit() {


    members->EmitAll();


    List<const char *> *method_labels = new List<const char *>;
    for (int i = 0; i < methods->NumElements(); i++) {
        FunctionDecl *fn = methods->Nth(i);

        method_labels->Append(fn->GetId()->GetIdName());
    }
    CG->GenVTable(id->GetIdName(), method_labels);
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl *> *m) : Decl(n) {

    (members = m)->SetParentAll(this);
}

void InterfaceDecl::BuildSymTable() {
    if (scopeHandler->LocalLookup(this->GetId())) {
        Decl *d = scopeHandler->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = scopeHandler->InsertSymbol(this);
        id->SetDecl(this);
    }
    scopeHandler->BuildScope(this->GetId()->GetIdName());
    members->BuildSymTableAll();
    scopeHandler->ExitScope();
}

void InterfaceDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            semantic_type = new NamedType(id);
            semantic_type->SetSelfType();

        default:
            id->Check(c);
            scopeHandler->EnterScope();
            members->CheckAll(c);
            scopeHandler->ExitScope();
    }
}

void InterfaceDecl::Emit() {
    semantic_error = 1;
    return;

}

FunctionDecl::FunctionDecl(Identifier *n, Type *r, List<VariableDecl *> *d) : Decl(n) {

    (returnType = r)->SetParent(this);
    (formals = d)->SetParentAll(this);
    body = NULL;
    vtable_ofst = -1;
}

void FunctionDecl::SetFunctionBody(Stmt *b) {
    (body = b)->SetParent(this);
}

void FunctionDecl::BuildSymTable() {
    if (scopeHandler->LocalLookup(this->GetId())) {
        Decl *d = scopeHandler->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = scopeHandler->InsertSymbol(this);
        id->SetDecl(this);
    }
    scopeHandler->BuildScope();
    formals->BuildSymTableAll();
    if (body) body->BuildSymTable();
    scopeHandler->ExitScope();
}

void FunctionDecl::CheckDecl() {
    returnType->Check(sem_decl);
    id->Check(sem_decl);
    scopeHandler->EnterScope();
    formals->CheckAll(sem_decl);
    if (body) body->Check(sem_decl);
    scopeHandler->ExitScope();


    if (!strcmp(id->GetIdName(), "main")) {
        if (formals->NumElements() != 0) {
            semantic_error = 1;
            return;
        }
    }

    semantic_type = returnType->GetType();
}

void FunctionDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        default:
            returnType->Check(c);
            id->Check(c);
            scopeHandler->EnterScope();
            formals->CheckAll(c);
            if (body) body->Check(c);
            scopeHandler->ExitScope();
    }
}

bool FunctionDecl::IsEquivalentTo(Decl *other) {


    if (!other->IsFunctionDecl()) {
        return false;
    }
    FunctionDecl *fn = dynamic_cast<FunctionDecl *>(other);
    if (!returnType->IsEquivalentTo(fn->GetType())) {
        return false;
    }
    if (formals->NumElements() != fn->GetFormals()->NumElements()) {
        return false;
    }
    for (int i = 0; i < formals->NumElements(); i++) {

        Type *var_type1 =
                (dynamic_cast<VariableDecl *>(formals->Nth(i)))->GetType();
        Type *var_type2 =
                (dynamic_cast<VariableDecl *>(fn->GetFormals()->Nth(i)))->GetType();
        if (!var_type1->IsEquivalentTo(var_type2)) {
            return false;
        }
    }
    return true;
}

void FunctionDecl::AddPrefixToMethods() {


    Decl *d = dynamic_cast<Decl *>(this->GetParent());
    if (d && d->IsClassDecl()) {
        id->AddPrefix(".");
        id->AddPrefix(d->GetId()->GetIdName());
        id->AddPrefix("_");
    } else if (strcmp(id->GetIdName(), "main")) {
        id->AddPrefix("_");
    }
}

void FunctionDecl::AssignMemberOffset(bool inClass, int offset) {
    vtable_ofst = offset;
}

void FunctionDecl::Emit() {

    if (returnType == Type::doubleType) {
        semantic_error = 1;
        return;

    }

    Decl *d = dynamic_cast<Decl *>(this->GetParent());
    CG->GenLabel(id->GetIdName());


    BeginFunc *f = CG->GenBeginFunc();


    if (d && d->IsClassDecl()) {
        CG->GetNextParamLoc();
    }


    for (int i = 0; i < formals->NumElements(); i++) {
        VariableDecl *v = formals->Nth(i);
        if (v->GetType() == Type::doubleType) {
            semantic_error = 1;
            return;

        }
        Location *l = new Location(fpRelative, CG->GetNextParamLoc(),
                                   v->GetId()->GetIdName());
        v->SetEmitLoc(l);
    }

    if (body) body->Emit();


    f->SetFrameSize(CG->GetFrameSize());

    CG->GenEndFunc();
}


void EmptyExpr::Check(checkStep c) {
    if (c == sem_type) {
        semantic_type = Type::voidType;
    }
}

IntLiteral::IntLiteral(yyltype loc, int val) : Expr(loc) {
    value = val;
}

void IntLiteral::Check(checkStep c) {
    if (c == sem_decl) {
        semantic_type = Type::intType;
    }
}

void IntLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value);
}

DoubleLiteral::DoubleLiteral(yyltype loc, double val) : Expr(loc) {
    value = val;
}

void DoubleLiteral::Check(checkStep c) {
    if (c == sem_decl) {
        semantic_type = Type::doubleType;
    }
}

void DoubleLiteral::Emit() {
    semantic_error = 1;
    return;

}

BoolLiteral::BoolLiteral(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

void BoolLiteral::Check(checkStep c) {
    if (c == sem_decl) {
        semantic_type = Type::boolType;
    }
}

void BoolLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value ? 1 : 0);
}

StringLiteral::StringLiteral(yyltype loc, const char *val) : Expr(loc) {

    value = strdup(val);
}

void StringLiteral::Check(checkStep c) {
    if (c == sem_decl) {
        semantic_type = Type::stringType;
    }
}

void StringLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value);
}


void NullLiteral::Check(checkStep c) {
    if (c == sem_decl) {
        semantic_type = Type::nullType;
    }
}

void NullLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(0);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {

    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r)
        : Expr(Join(l->GetLocation(), r->GetLocation())) {

    (op = o)->SetParent(this);
    (left = l)->SetParent(this);
    (right = r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r)
        : Expr(Join(o->GetLocation(), r->GetLocation())) {

    left = NULL;
    (op = o)->SetParent(this);
    (right = r)->SetParent(this);
}

void ArithmeticExpr::CheckType() {
    if (left) left->Check(sem_type);
    op->Check(sem_type);
    right->Check(sem_type);

    if (!strcmp(op->GetOpStr(), "-") && !left) {
        Type *tr = right->GetType();
        if (tr == NULL) {

            return;
        }
        if (tr == Type::intType) {
            semantic_type = Type::intType;
        } else if (tr == Type::doubleType) {
            semantic_type = Type::doubleType;
        } else {
            semantic_error = 1;
            return;
        }
    } else {
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {

            return;
        }
        if (tl == Type::intType && tr == Type::intType) {
            semantic_type = Type::intType;
        } else if ((tl == Type::doubleType && tr == Type::doubleType)

                ) {
            semantic_type = Type::doubleType;
        } else {
            semantic_error = 1;
            return;
        }
    }
}

void ArithmeticExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void ArithmeticExpr::Emit() {
    if (left) left->Emit();
    right->Emit();

    Location *l = left ? left->GetEmitLocDeref() : CG->GenLoadConstant(0);
    asm_loc = CG->GenBinaryOp(op->GetOpStr(), l, right->GetEmitLocDeref());
}

void RelationalExpr::CheckType() {
    left->Check(sem_type);
    op->Check(sem_type);
    right->Check(sem_type);


    semantic_type = Type::boolType;

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {

        return;
    }

    if (!(tl == Type::intType && tr == Type::intType) &&
        !(tl == Type::doubleType && tr == Type::doubleType)) {
        semantic_error = 1;
        return;
    }
}

void RelationalExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void RelationalExpr::Emit() {
    left->Emit();
    right->Emit();

    asm_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                              right->GetEmitLocDeref());
}

void EqualityExpr::CheckType() {
    left->Check(sem_type);
    op->Check(sem_type);
    right->Check(sem_type);

    Type *tl = left->GetType();
    Type *tr = right->GetType();


    semantic_type = Type::boolType;

    if (tl == NULL || tr == NULL) {

        return;
    }

    if (!tr->IsCompatibleWith(tl) && !tl->IsCompatibleWith(tr)) {
        semantic_error = 1;
        return;
    }
}

void EqualityExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void EqualityExpr::Emit() {
    left->Emit();
    right->Emit();

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == tr && (tl == Type::intType || tl == Type::boolType)) {
        asm_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                                  right->GetEmitLocDeref());
    } else if (tl == tr && tl == Type::stringType) {
        asm_loc = CG->GenBuiltInCall(StringEqual, left->GetEmitLocDeref(),
                                     right->GetEmitLocDeref());
        if (!strcmp(op->GetOpStr(), "!=")) {

            asm_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                                      asm_loc);
        }
    } else {


        asm_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                                  right->GetEmitLocDeref());
    }
}

void LogicalExpr::CheckType() {
    if (left) left->Check(sem_type);
    op->Check(sem_type);
    right->Check(sem_type);


    semantic_type = Type::boolType;

    if (!strcmp(op->GetOpStr(), "!")) {
        Type *tr = right->GetType();
        if (tr == NULL) {

            return;
        }
        if (tr != Type::boolType) {
            semantic_error = 1;
            return;
        }
    } else {
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {

            return;
        }
        if (tl != Type::boolType || tr != Type::boolType) {
            semantic_error = 1;
            return;
        }
    }
}

void LogicalExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        if (left) left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void LogicalExpr::Emit() {
    if (left) left->Emit();
    right->Emit();

    if (left) {
        asm_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                                  right->GetEmitLocDeref());
    } else {

        asm_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                                  right->GetEmitLocDeref());
    }
}

void AssignExpr::CheckType() {
    left->Check(sem_type);
    op->Check(sem_type);
    right->Check(sem_type);

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {

        return;
    }

    if (!tl->IsCompatibleWith(tr)) {
        semantic_error = 1;
        return;
    }
}

void AssignExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        left->Check(c);
        op->Check(c);
        right->Check(c);
    }
}

void AssignExpr::Emit() {
    right->Emit();
    left->Emit();
    Location *r = right->GetEmitLocDeref();
    Location *l = left->GetEmitLoc();
    if (r && l) {

        if (l->GetBase() != NULL) {
            CG->GenStore(l->GetBase(), r, l->GetOffset());
        } else if (left->IsArrayAccessRef()) {
            CG->GenStore(l, r);
        } else {
            CG->GenAssign(l, r);
        }
        asm_loc = left->GetEmitLocDeref();
    }
}

void This::CheckType() {
    Decl *d = scopeHandler->LookupThis();
    if (!d || !d->IsClassDecl()) {
        semantic_error = 1;
        return;
    } else {

        semantic_type = new NamedType(d->GetId());
        semantic_type->SetSelfType();
    }
}

void This::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    }
}

void This::Emit() {
    asm_loc = CG->ThisPtr;
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base = b)->SetParent(this);
    (subscript = s)->SetParent(this);
}

void ArrayAccess::CheckType() {
    Type *t;
    int err = 0;

    subscript->Check(sem_type);
    t = subscript->GetType();
    if (t == NULL) {

    } else if (t != Type::intType) {
        semantic_error = 1;
        return;
    }

    base->Check(sem_type);
    t = base->GetType();
    if (t == NULL) {

        err++;
    } else if (!t->IsArrayType()) {
        semantic_error = 1;
        return;
        err++;
    }

    if (!err) {

        semantic_type = (dynamic_cast<ArrayType *>(t))->GetElemType();
    }
}

void ArrayAccess::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        base->Check(c);
        subscript->Check(c);
    }
}

void ArrayAccess::Emit() {
    base->Emit();
    subscript->Emit();
    Location *t0 = subscript->GetEmitLocDeref();

    Location *t1 = CG->GenLoadConstant(0);
    Location *t2 = CG->GenBinaryOp("<", t0, t1);
    Location *t3 = base->GetEmitLocDeref();
    Location *t4 = CG->GenLoad(t3, -4);
    Location *t5 = CG->GenBinaryOp("<", t0, t4);
    Location *t6 = CG->GenBinaryOp("==", t5, t1);
    Location *t7 = CG->GenBinaryOp("||", t2, t6);
    const char *l = CG->NewLabel();
    CG->GenIfZ(t7, l);
    Location *t8 = CG->GenLoadConstant(indx_out_of_bound);
    CG->GenBuiltInCall(PrintString, t8);
    CG->GenBuiltInCall(Halt);
    CG->GenLabel(l);

    Location *t9 = CG->GenLoadConstant(semantic_type->GetTypeSize());
    Location *t10 = CG->GenBinaryOp("*", t9, t0);
    Location *t11 = CG->GenBinaryOp("+", t3, t10);
    asm_loc = t11;
}

Location *ArrayAccess::GetEmitLocDeref() {
    Location *t = CG->GenLoad(asm_loc, 0);
    return t;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f)
        : LValue(b ? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {

    base = b;
    if (base) base->SetParent(this);
    (field = f)->SetParent(this);
}

void FieldAccess::CheckDecl() {
    if (!base) {
        Decl *d = scopeHandler->Lookup(field);
        if (d == NULL) {
            semantic_error = 1;
            return;
            return;
        } else {
            field->SetDecl(d);
        }
    } else {

        base->Check(sem_decl);
    }
}

void FieldAccess::CheckType() {
    if (!base) {
        if (field->GetDecl()) {
            if (field->GetDecl()->IsVariableDecl()) {
                semantic_type = field->GetDecl()->GetType();
            } else {
                semantic_error = 1;
                return;
            }
        }
        return;
    }


    base->Check(sem_type);
    Type *base_t = base->GetType();
    if (base_t != NULL) {
        if (!base_t->IsNamedType()) {
            semantic_error = 1;
            return;
            return;
        }

        Decl *d = scopeHandler->LookupField(
                dynamic_cast<NamedType *>(base_t)->GetId(), field);

        if (d == NULL || !d->IsVariableDecl()) {
            semantic_error = 1;
            return;
        } else {


            Decl *cur_class = scopeHandler->LookupThis();
            if (!cur_class || !cur_class->IsClassDecl()) {


                semantic_error = 1;
                return;
                return;
            }


            Type *cur_t = cur_class->GetType();
            d = scopeHandler->LookupField(
                    dynamic_cast<NamedType *>(cur_t)->GetId(), field);

            if (d == NULL || !d->IsVariableDecl()) {
                semantic_error = 1;
                return;
                return;
            }
            if (cur_t->IsCompatibleWith(base_t) ||
                base_t->IsCompatibleWith(cur_t)) {
                field->SetDecl(d);
                semantic_type = d->GetType();
            } else {
                semantic_error = 1;
                return;
            }
        }
    }
}

void FieldAccess::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        case sem_type:
            this->CheckType();
            break;
        default:;

    }
}

void FieldAccess::Emit() {
    if (base) base->Emit();
    field->Emit();
    asm_loc = field->GetEmitLocDeref();


    if (base)
        asm_loc = new Location(fpRelative, asm_loc->GetOffset(),
                               asm_loc->GetName(), base->GetEmitLocDeref());
}

Location *FieldAccess::GetEmitLocDeref() {
    Location *t = asm_loc;
    if (t->GetBase() != NULL) {

        t = CG->GenLoad(t->GetBase(), t->GetOffset());
    }
    return t;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr *> *a) : Expr(loc) {

    base = b;
    if (base) base->SetParent(this);
    (field = f)->SetParent(this);
    (actuals = a)->SetParentAll(this);
}

void Call::CheckDecl() {
    if (!base) {
        Decl *d = scopeHandler->Lookup(field);
        if (d == NULL || !d->IsFunctionDecl()) {
            semantic_error = 1;
            return;
            return;
        } else {
            field->SetDecl(d);

            semantic_type = d->GetType();
        }
    } else {

        base->Check(sem_decl);
    }
    actuals->CheckAll(sem_decl);
}

void Call::CheckType() {
    if (!base) {
        if (field->GetDecl() && !semantic_type) {

            semantic_type = field->GetDecl()->GetType();
        }
    } else {

        base->Check(sem_type);
        Type *t = base->GetType();
        if (t != NULL) {
            if (t->IsArrayType() && !strcmp(field->GetIdName(), "length")) {


                int n = actuals->NumElements();
                if (n) {
                    semantic_error = 1;
                    return;
                }
                semantic_type = Type::intType;
            } else if (!t->IsNamedType()) {
                semantic_error = 1;
                return;
            } else {
                Decl *d = scopeHandler->LookupField(
                        dynamic_cast<NamedType *>(t)->GetId(), field);
                if (d == NULL || !d->IsFunctionDecl()) {
                    semantic_error = 1;
                    return;
                } else {
                    field->SetDecl(d);
                    semantic_type = d->GetType();
                }
            }
        }
    }
    actuals->CheckAll(sem_type);
    this->CheckFuncArgs();
}

void Call::CheckFuncArgs() {
    Decl *f = field->GetDecl();
    if (!f || !f->IsFunctionDecl()) return;
    FunctionDecl *fun = dynamic_cast<FunctionDecl *>(f);
    List<VariableDecl *> *formals = fun->GetFormals();

    int n_expected = formals->NumElements();
    int n_given = actuals->NumElements();
    if (n_given != n_expected) {
        semantic_error = 1;
        return;
    } else {
        for (int i = 0; i < actuals->NumElements(); i++) {
            Type *t_a = actuals->Nth(i)->GetType();
            Type *t_f = formals->Nth(i)->GetType();

            if (t_a && t_f && !t_f->IsCompatibleWith(t_a)) {
                semantic_error = 1;
                return;
            }
        }
    }
}

void Call::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        case sem_type:
            this->CheckType();
            break;
        default:;

    }
}

void Call::Emit() {


    if (base) base->Emit();
    field->Emit();
    actuals->EmitAll();


    if (base && base->GetType()->IsArrayType() &&
        !strcmp(field->GetIdName(), "length")) {
        Location *t0 = base->GetEmitLocDeref();
        Location *t1 = CG->GenLoad(t0, -4);
        asm_loc = t1;
        return;
    }

    FunctionDecl *fn = dynamic_cast<FunctionDecl *>(field->GetDecl());

    bool is_ACall = (base != NULL) || (fn->IsClassMember());


    Location *this_loc;
    if (base) {
        this_loc = base->GetEmitLocDeref();
    } else if (fn->IsClassMember()) {
        this_loc = CG->ThisPtr;
    }

    Location *t;
    if (is_ACall) {
        t = CG->GenLoad(this_loc, 0);
        t = CG->GenLoad(t, fn->GetVTableOffset());
    }


    for (int i = actuals->NumElements() - 1; i >= 0; i--) {
        Location *l = actuals->Nth(i)->GetEmitLocDeref();
        CG->GenPushParam(l);
    }


    if (is_ACall) {

        CG->GenPushParam(this_loc);

        asm_loc = CG->GenACall(t, fn->HasReturnValue());

        CG->GenPopParams(actuals->NumElements() * 4 + 4);
    } else {

        field->AddPrefix("_");
        asm_loc = CG->GenLCall(field->GetIdName(),
                               semantic_type != Type::voidType);

        CG->GenPopParams(actuals->NumElements() * 4);
    }
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) {

    (cType = c)->SetParent(this);
}

void NewExpr::CheckDecl() {
    cType->Check(sem_decl, classReason);
}

void NewExpr::CheckType() {
    cType->Check(sem_type);

    if (cType->GetType()) {
        semantic_type = cType;
    }
}

void NewExpr::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl();
            break;
        case sem_type:
            this->CheckType();
            break;
        default:
            cType->Check(c);
    }
}

void NewExpr::Emit() {
    ClassDecl *d = dynamic_cast<ClassDecl *>(cType->GetId()->GetDecl());

    int size = d->GetInstanceSize();
    Location *t = CG->GenLoadConstant(size);
    asm_loc = CG->GenBuiltInCall(Alloc, t);
    Location *l = CG->GenLoadLabel(d->GetId()->GetIdName());
    CG->GenStore(asm_loc, l, 0);
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {

    (size = sz)->SetParent(this);
    (elemType = et)->SetParent(this);
}

void NewArrayExpr::CheckType() {
    Type *t;

    size->Check(sem_type);
    t = size->GetType();
    if (t == NULL) {

    } else if (t != Type::intType) {
        semantic_error = 1;
        return;
    }

    elemType->Check(sem_type);
    if (!elemType->GetType()) {

        return;
    } else {

        semantic_type = new ArrayType(*location, elemType);
        semantic_type->Check(sem_decl);
    }
}

void NewArrayExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        size->Check(c);
        elemType->Check(c);
    }
}

void NewArrayExpr::Emit() {
    size->Emit();
    Location *t0 = size->GetEmitLocDeref();
    Location *t1 = CG->GenLoadConstant(0);
    Location *t2 = CG->GenBinaryOp("<", t0, t1);

    const char *l = CG->NewLabel();
    CG->GenIfZ(t2, l);
    Location *t3 = CG->GenLoadConstant(neg_arr_size);
    CG->GenBuiltInCall(PrintString, t3);
    CG->GenBuiltInCall(Halt);

    CG->GenLabel(l);
    Location *t4 = CG->GenLoadConstant(1);
    Location *t5 = CG->GenBinaryOp("+", t4, t0);
    Location *t6 = CG->GenLoadConstant(elemType->GetTypeSize());
    Location *t7 = CG->GenBinaryOp("*", t5, t6);
    Location *t8 = CG->GenBuiltInCall(Alloc, t7);
    CG->GenStore(t8, t0);
    Location *t9 = CG->GenBinaryOp("+", t8, t6);
    asm_loc = t9;
}

void ReadIntegerExpr::Check(checkStep c) {
    if (c == sem_type) {
        semantic_type = Type::intType;
    }
}

void ReadIntegerExpr::Emit() {
    asm_loc = CG->GenBuiltInCall(ReadInteger);
}

void ReadLineExpr::Check(checkStep c) {
    if (c == sem_type) {
        semantic_type = Type::stringType;
    }
}

void ReadLineExpr::Emit() {
    asm_loc = CG->GenBuiltInCall(ReadLine);
}

PostfixExpr::PostfixExpr(LValue *lv, Operator *o)
        : Expr(Join(lv->GetLocation(), o->GetLocation())) {

    (lvalue = lv)->SetParent(this);
    (op = o)->SetParent(this);
}

void PostfixExpr::CheckType() {
    lvalue->Check(sem_type);
    op->Check(sem_type);
    Type *t = lvalue->GetType();
    if (t == NULL) {

    } else if (t != Type::intType) {
        semantic_error = 1;
        return;
    } else {
        semantic_type = t;
    }
}

void PostfixExpr::Check(checkStep c) {
    if (c == sem_type) {
        this->CheckType();
    } else {
        lvalue->Check(c);
        op->Check(c);
    }
}

void PostfixExpr::Emit() {
    lvalue->Emit();

    Location *l1 = lvalue->GetEmitLoc();
    Location *l2 = lvalue->GetEmitLocDeref();


    Location *t0 = CG->GenTempVar();
    CG->GenAssign(t0, l2);


    l2 = CG->GenBinaryOp(strcmp(op->GetOpStr(), "++") ? "-" : "+",
                         l2, CG->GenLoadConstant(1));


    if (l1->GetBase() != NULL) {
        CG->GenStore(l1->GetBase(), l2, l1->GetOffset());
    } else if (lvalue->IsArrayAccessRef()) {
        CG->GenStore(l1, l2);
    } else {
        CG->GenAssign(l1, l2);
    }


    asm_loc = t0;
}


Program::Program(List<Decl *> *d) {

    (decls = d)->SetParentAll(this);
}

void Program::BuildSymTable() {
    scopeHandler = new SymbolTable();


    decls->BuildSymTableAll();
}

void Program::Check() {


    scopeHandler->ResetSymbolTable();
    decls->CheckAll(sem_decl);


    scopeHandler->ResetSymbolTable();
    decls->CheckAll(sem_inh);


    scopeHandler->ResetSymbolTable();
    decls->CheckAll(sem_type);
}

void Program::Emit() {


    bool has_main = false;
    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
        if (d->IsFunctionDecl()) {
            if (!strcmp(d->GetId()->GetIdName(), "main")) {
                has_main = true;
                break;
            }
        }
    }
    if (!has_main) {
        semantic_error = 1;
        return;
        return;
    }


    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AssignOffset();
    }

    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AddPrefixToMethods();
    }


    decls->EmitAll();


    if (semantic_error != 0)
        return;


    CG->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VariableDecl *> *d, List<Stmt *> *s) {

    (decls = d)->SetParentAll(this);
    (stmts = s)->SetParentAll(this);
}


void StmtBlock::BuildSymTable() {
    scopeHandler->BuildScope();
    decls->BuildSymTableAll();
    stmts->BuildSymTableAll();
    scopeHandler->ExitScope();
}

void StmtBlock::Check(checkStep c) {
    scopeHandler->EnterScope();
    decls->CheckAll(c);
    stmts->CheckAll(c);
    scopeHandler->ExitScope();
}

void StmtBlock::Emit() {
    decls->EmitAll();
    stmts->EmitAll();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) {

    (test = t)->SetParent(this);
    (body = b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b) : LoopStmt(t, b) {

    (init = i)->SetParent(this);
    (step = s)->SetParent(this);
}


void ForStmt::BuildSymTable() {
    scopeHandler->BuildScope();
    body->BuildSymTable();
    scopeHandler->ExitScope();
}

void ForStmt::CheckType() {
    init->Check(sem_type);
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    step->Check(sem_type);
    scopeHandler->EnterScope();
    body->Check(sem_type);
    scopeHandler->ExitScope();
}

void ForStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType();
            break;
        default:
            init->Check(c);
            test->Check(c);
            step->Check(c);
            scopeHandler->EnterScope();
            body->Check(c);
            scopeHandler->ExitScope();
    }
}

void ForStmt::Emit() {
    init->Emit();

    const char *l0 = CG->NewLabel();
    CG->GenLabel(l0);
    test->Emit();
    Location *t0 = test->GetEmitLocDeref();
    const char *l1 = CG->NewLabel();
    end_loop_label = l1;
    CG->GenIfZ(t0, l1);

    body->Emit();
    step->Emit();
    CG->GenGoto(l0);

    CG->GenLabel(l1);
}


void WhileStmt::BuildSymTable() {
    scopeHandler->BuildScope();
    body->BuildSymTable();
    scopeHandler->ExitScope();
}

void WhileStmt::CheckType() {
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    scopeHandler->EnterScope();
    body->Check(sem_type);
    scopeHandler->ExitScope();
}

void WhileStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType();
            break;
        default:
            test->Check(c);
            scopeHandler->EnterScope();
            body->Check(c);
            scopeHandler->ExitScope();
    }
}

void WhileStmt::Emit() {
    const char *l0 = CG->NewLabel();
    CG->GenLabel(l0);

    test->Emit();
    Location *t0 = test->GetEmitLocDeref();
    const char *l1 = CG->NewLabel();
    end_loop_label = l1;
    CG->GenIfZ(t0, l1);

    body->Emit();
    CG->GenGoto(l0);

    CG->GenLabel(l1);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb) : ConditionalStmt(t, tb) {

    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}


void IfStmt::BuildSymTable() {
    scopeHandler->BuildScope();
    body->BuildSymTable();
    scopeHandler->ExitScope();
    if (elseBody) {
        scopeHandler->BuildScope();
        elseBody->BuildSymTable();
        scopeHandler->ExitScope();
    }
}

void IfStmt::CheckType() {
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    scopeHandler->EnterScope();
    body->Check(sem_type);
    scopeHandler->ExitScope();
    if (elseBody) {
        scopeHandler->EnterScope();
        elseBody->Check(sem_type);
        scopeHandler->ExitScope();
    }
}

void IfStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType();
            break;
        default:
            test->Check(c);
            scopeHandler->EnterScope();
            body->Check(c);
            scopeHandler->ExitScope();
            if (elseBody) {
                scopeHandler->EnterScope();
                elseBody->Check(c);
                scopeHandler->ExitScope();
            }
    }
}

void IfStmt::Emit() {
    test->Emit();
    Location *t0 = test->GetEmitLocDeref();
    const char *l0 = CG->NewLabel();
    CG->GenIfZ(t0, l0);

    body->Emit();
    const char *l1 = CG->NewLabel();
    CG->GenGoto(l1);

    CG->GenLabel(l0);
    if (elseBody) elseBody->Emit();
    CG->GenLabel(l1);
}

void BreakStmt::Check(checkStep c) {
    if (c == sem_type) {
        Node *n = this;
        while (n->GetParent()) {
            if (n->IsLoopStmt() || n->IsCaseStmt()) return;
            n = n->GetParent();
        }
        semantic_error = 1;
        return;
    }
}

void BreakStmt::Emit() {

    Node *n = this;
    while (n->GetParent()) {
        if (n->IsLoopStmt()) {
            const char *l = dynamic_cast<LoopStmt *>(n)->GetEndLoopLabel();

            CG->GenGoto(l);
            return;
        } else if (n->IsSwitchStmt()) {
            const char *l = dynamic_cast<SwitchStmt *>(n)->GetEndSwitchLabel();

            CG->GenGoto(l);
            return;
        }
        n = n->GetParent();
    }
}

CaseStmt::CaseStmt(IntLiteral *v, List<Stmt *> *s) {

    value = v;
    if (value) value->SetParent(this);
    (stmts = s)->SetParentAll(this);
    case_label = NULL;
}


void CaseStmt::BuildSymTable() {
    scopeHandler->BuildScope();
    stmts->BuildSymTableAll();
    scopeHandler->ExitScope();
}

void CaseStmt::Check(checkStep c) {
    if (value) value->Check(c);
    scopeHandler->EnterScope();
    stmts->CheckAll(c);
    scopeHandler->ExitScope();
}

void CaseStmt::GenCaseLabel() {
    case_label = CG->NewLabel();
}

void CaseStmt::Emit() {
    CG->GenLabel(case_label);
    stmts->EmitAll();
}

SwitchStmt::SwitchStmt(Expr *e, List<CaseStmt *> *c) {

    (expr = e)->SetParent(this);
    (cases = c)->SetParentAll(this);
    end_switch_label = NULL;
}


void SwitchStmt::BuildSymTable() {
    scopeHandler->BuildScope();
    cases->BuildSymTableAll();
    scopeHandler->ExitScope();
}

void SwitchStmt::Check(checkStep c) {
    expr->Check(c);
    scopeHandler->EnterScope();
    cases->CheckAll(c);
    scopeHandler->ExitScope();
}

void SwitchStmt::Emit() {
    expr->Emit();


    end_switch_label = CG->NewLabel();

    Location *switch_value = expr->GetEmitLocDeref();


    for (int i = 0; i < cases->NumElements(); i++) {
        CaseStmt *c = cases->Nth(i);


        c->GenCaseLabel();
        const char *cl = c->GetCaseLabel();


        IntLiteral *cv = c->GetCaseValue();


        if (cv) {

            cv->Emit();
            Location *cvl = cv->GetEmitLocDeref();
            Location *t = CG->GenBinaryOp("!=", switch_value, cvl);
            CG->GenIfZ(t, cl);
        } else {

            CG->GenGoto(cl);
        }
    }


    cases->EmitAll();


    CG->GenLabel(end_switch_label);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {

    (expr = e)->SetParent(this);
}


void ReturnStmt::Check(checkStep c) {
    expr->Check(c);
    if (c == sem_type) {
        Node *n = this;

        while (n->GetParent()) {
            if (dynamic_cast<FunctionDecl *>(n) != NULL) break;
            n = n->GetParent();
        }
        Type *t_given = expr->GetType();
        Type *t_expected = dynamic_cast<FunctionDecl *>(n)->GetType();
        if (t_given && t_expected) {
            if (!t_expected->IsCompatibleWith(t_given)) {
                semantic_error = 1;
                return;
            }
        }
    }
}

void ReturnStmt::Emit() {
    if (expr->IsEmptyExpr()) {
        CG->GenReturn();
    } else {
        expr->Emit();
        CG->GenReturn(expr->GetEmitLocDeref());
    }
}

PrintStmt::PrintStmt(List<Expr *> *a) {

    (args = a)->SetParentAll(this);
}


void PrintStmt::Check(checkStep c) {
    args->CheckAll(c);
    if (c == sem_type) {
        for (int i = 0; i < args->NumElements(); i++) {
            Type *t = args->Nth(i)->GetType();
            if (t != NULL && t != Type::stringType && t != Type::intType
                && t != Type::boolType) {
                semantic_error = 1;
                return;
            }
        }
    }
}

void PrintStmt::Emit() {
    for (int i = 0; i < args->NumElements(); i++) {
        args->Nth(i)->Emit();

        Type *t = args->Nth(i)->GetType();
        BuiltIn f;
        if (t == Type::intType) {
            f = PrintInt;
        } else if (t == Type::stringType) {
            f = PrintString;
        } else {
            f = PrintBool;
        }
        Location *l = args->Nth(i)->GetEmitLocDeref();

        CG->GenBuiltInCall(f, l);
    }

    BuiltIn f = PrintString;
    char const *str = "\\n";
    Location *l = CG->GenLoadConstant(str);

    CG->GenBuiltInCall(f, l);
}


Type *Type::intType = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType = new Type("void");
Type *Type::boolType = new Type("bool");
Type *Type::nullType = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType = new Type("error");

Type::Type(const char *n) {

    typeName = strdup(n);
    semantic_type = NULL;
}


void Type::Check(checkStep c) {
    if (c == sem_decl) {
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

    (id = i)->SetParent(this);
}


void NamedType::CheckDecl(checkFor r) {
    Decl *d = scopeHandler->Lookup(this->id);
    if (d == NULL || (!d->IsClassDecl() && !d->IsInterfaceDecl())) {
        semantic_error = 1;
        return;
    } else if (r == classReason && !d->IsClassDecl()) {
        semantic_error = 1;
        return;
    } else if (r == interfaceReason && !d->IsInterfaceDecl()) {
        semantic_error = 1;
        return;
    } else {
        this->id->SetDecl(d);
        semantic_type = this;
    }
}

void NamedType::Check(checkStep c, checkFor r) {
    if (c == sem_decl) {
        this->CheckDecl(r);
    } else {
        id->Check(c);
    }
}

bool NamedType::IsEquivalentTo(Type *other) {


    if (!other->IsNamedType()) {
        return false;
    }
    NamedType *nt = dynamic_cast<NamedType *>(other);
    return (id->IsEquivalentTo(nt->GetId()));
}


bool NamedType::IsCompatibleWith(Type *other) {


    if (other == nullType) {
        return true;
    } else if (!other->IsNamedType()) {
        return false;
    } else if (this->IsEquivalentTo(other)) {
        return true;
    } else {
        NamedType *nt = dynamic_cast<NamedType *>(other);
        Decl *decl1 = id->GetDecl();
        Decl *decl2 = nt->GetId()->GetDecl();

        if (!decl2->IsClassDecl()) {
            return false;
        }
        ClassDecl *cdecl2 = dynamic_cast<ClassDecl *>(decl2);

        return cdecl2->IsChildOf(decl1);
    }
}

ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {

    (elemType = et)->SetParent(this);
}


void ArrayType::CheckDecl() {
    elemType->Check(sem_decl);
    if (elemType->GetType()) {
        semantic_type = this;
    }
}

void ArrayType::Check(checkStep c) {
    if (c == sem_decl) {
        this->CheckDecl();
    } else {
        elemType->Check(c);
    }
}

bool ArrayType::IsEquivalentTo(Type *other) {


    if (!other->IsArrayType()) {
        return false;
    }
    ArrayType *nt = dynamic_cast<ArrayType *>(other);
    return (elemType->IsEquivalentTo(nt->GetElemType()));
}

bool ArrayType::IsCompatibleWith(Type *other) {


    if (other == nullType) {
        return elemType->IsCompatibleWith(other);
    } else {
        return this->IsEquivalentTo(other);
    }
}

