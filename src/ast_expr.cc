/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include <iostream>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"


void EmptyExpr::Check(checkT c) {
    if (c == E_CheckType) {
        semantic_type = Type::voidType;
    }
}

IntLiteral::IntLiteral(yyltype loc, int val) : Expr(loc) {
    value = val;
}

void IntLiteral::Check(checkT c) {
    if (c == E_CheckDecl) {
        semantic_type = Type::intType;
    }
}

void IntLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value);
}

DoubleLiteral::DoubleLiteral(yyltype loc, double val) : Expr(loc) {
    value = val;
}

void DoubleLiteral::Check(checkT c) {
    if (c == E_CheckDecl) {
        semantic_type = Type::doubleType;
    }
}

void DoubleLiteral::Emit() {
    semantic_error = 1;
    return;
    Assert(0);
}

BoolLiteral::BoolLiteral(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

void BoolLiteral::Check(checkT c) {
    if (c == E_CheckDecl) {
        semantic_type = Type::boolType;
    }
}

void BoolLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value ? 1 : 0);
}

StringLiteral::StringLiteral(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

void StringLiteral::Check(checkT c) {
    if (c == E_CheckDecl) {
        semantic_type = Type::stringType;
    }
}

void StringLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(value);
}


void NullLiteral::Check(checkT c) {
    if (c == E_CheckDecl) {
        semantic_type = Type::nullType;
    }
}

void NullLiteral::Emit() {
    asm_loc = CG->GenLoadConstant(0);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r)
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r)
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL;
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

void ArithmeticExpr::CheckType() {
    if (left) left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    if (!strcmp(op->GetOpStr(), "-") && !left) {
        Type *tr = right->GetType();
        if (tr == NULL) {
            // some error accur in left or right, so skip it.
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
    } else { // for + - * / %
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tl == Type::intType && tr == Type::intType) {
            semantic_type = Type::intType;
        } else if ((tl == Type::doubleType && tr == Type::doubleType)
                // && strcmp(op->GetOpStr(), "%") // % can apply to float.
                ) {
            semantic_type = Type::doubleType;
        } else {
            semantic_error = 1;
    return;
        }
    }
}

void ArithmeticExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    // the type of RelationalExpr is always boolType.
    semantic_type = Type::boolType;

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!(tl == Type::intType && tr == Type::intType) &&
            !(tl == Type::doubleType && tr == Type::doubleType)) {
        semantic_error = 1;
    return;
    }
}

void RelationalExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    // the type of EqualityExpr is always boolType.
    semantic_type = Type::boolType;

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!tr->IsCompatibleWith(tl) && !tl->IsCompatibleWith(tr)) {
        semantic_error = 1;
    return;
    }
}

void EqualityExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
            // for s1 != s2, generate s1 == s2, then generate logical not.
            asm_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                asm_loc);
        }
    } else {
        // array? class? interface?
        // just compare the reference.
        asm_loc = CG->GenBinaryOp(op->GetOpStr(), left->GetEmitLocDeref(),
                right->GetEmitLocDeref());
    }
}

void LogicalExpr::CheckType() {
    if (left) left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    // the type of LogicalExpr is always boolType.
    semantic_type = Type::boolType;

    if (!strcmp(op->GetOpStr(), "!")) {
        Type *tr = right->GetType();
        if (tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tr != Type::boolType) {
            semantic_error = 1;
    return;
        }
    } else { // for && and ||
        Type *tl = left->GetType();
        Type *tr = right->GetType();
        if (tl == NULL || tr == NULL) {
            // some error accur in left or right, so skip it.
            return;
        }
        if (tl != Type::boolType || tr != Type::boolType) {
            semantic_error = 1;
    return;
        }
    }
}

void LogicalExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
        // use 0 == bool_var to compute !bool_var.
        asm_loc = CG->GenBinaryOp("==", CG->GenLoadConstant(0),
                right->GetEmitLocDeref());
    }
}

void AssignExpr::CheckType() {
    left->Check(E_CheckType);
    op->Check(E_CheckType);
    right->Check(E_CheckType);

    Type *tl = left->GetType();
    Type *tr = right->GetType();

    if (tl == NULL || tr == NULL) {
        // some error accur in left or right, so skip it.
        return;
    }

    if (!tl->IsCompatibleWith(tr)) {
        semantic_error = 1;
    return;
    }
}

void AssignExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
        // base can be this or class instances.
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
    Decl *d = symtab->LookupThis();
    if (!d || !d->IsClassDecl()) {
        semantic_error = 1;
    return;
    } else {
        // Note: here create a new NamedType for 'this'.
        semantic_type = new NamedType(d->GetId());
        semantic_type->SetSelfType();
    }
}

void This::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    }
}

void This::Emit() {
    asm_loc = CG->ThisPtr;
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this);
    (subscript=s)->SetParent(this);
}

void ArrayAccess::CheckType() {
    Type *t;
    int err = 0;

    subscript->Check(E_CheckType);
    t = subscript->GetType();
    if (t == NULL) {
        // some error accur in subscript, so skip it.
    } else if (t != Type::intType) {
        semantic_error = 1;
    return;
    }

    base->Check(E_CheckType);
    t = base->GetType();
    if (t == NULL) {
        // some error accur in base, so skip it.
        err++;
    } else if (!t->IsArrayType()) {
        semantic_error = 1;
    return;
        err++;
    }

    if (!err) {
        // the error of subscript will not affect the type of array access.
        semantic_type = (dynamic_cast<ArrayType*>(t))->GetElemType();
    }
}

void ArrayAccess::Check(checkT c) {
    if (c == E_CheckType) {
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
    Location *t8 = CG->GenLoadConstant(err_arr_out_of_bounds);
    CG->GenBuiltInCall(PrintString, t8);
    CG->GenBuiltInCall(Halt);
    CG->GenLabel(l);

    Location *t9 = CG->GenLoadConstant(semantic_type->GetTypeSize());
    Location *t10 = CG->GenBinaryOp("*", t9, t0);
    Location *t11 = CG->GenBinaryOp("+", t3, t10);
    asm_loc = t11;
}

Location * ArrayAccess::GetEmitLocDeref() {
    Location *t = CG->GenLoad(asm_loc, 0);
    return t;
}

FieldAccess::FieldAccess(Expr *b, Identifier *f)
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
}

void FieldAccess::CheckDecl() {
    if (!base) {
        Decl *d = symtab->Lookup(field);
        if (d == NULL) {
            semantic_error = 1;
    return;
            return;
        } else {
            field->SetDecl(d);
        }
    } else {
        // if has base, then leave the work to CheckType.
        base->Check(E_CheckDecl);
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

    // must check the base expr's class type, and find in that class.
    base->Check(E_CheckType);
    Type *base_t = base->GetType();
    if (base_t != NULL) {
        if (!base_t->IsNamedType()) {
            semantic_error = 1;
    return;
            return;
        }

        Decl *d = symtab->LookupField(
                dynamic_cast<NamedType*>(base_t)->GetId(), field);

        if (d == NULL || !d->IsVariableDecl()) {
            semantic_error = 1;
    return;
        } else {//amir here we check the accessibility of the field. to include public private protected, change here
            // If base is 'this' or any instances of current class,
            // then all the private variable members are accessible.
            // Otherwise the variable members are not accessible.
            // Note: If base is a subclass of current class, and the
            // field is belong to current class, then this field is
            // accessible.
            Decl *cur_class = symtab->LookupThis();
            if (!cur_class || !cur_class->IsClassDecl()) {
                // not in a class scope, all the variable members are
                // not accessible.
                semantic_error = 1;
    return;
                return;
            }
            // in a class scope, the variable members can be
            // accessed by 'this' or the compatible class instance.
            Type *cur_t = cur_class->GetType();
            d = symtab->LookupField(
                    dynamic_cast<NamedType*>(cur_t)->GetId(), field);

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

void FieldAccess::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:;
            // do not check anything.
    }
}

void FieldAccess::Emit() {
    if (base) base->Emit();
    field->Emit();
    asm_loc = field->GetEmitLocDeref();

    // can access a var member in a class scope, so set the base.
    if (base)
        asm_loc = new Location(fpRelative, asm_loc->GetOffset(),
                asm_loc->GetName(), base->GetEmitLocDeref());
}

Location * FieldAccess::GetEmitLocDeref() {
    Location *t = asm_loc;
    if (t->GetBase() != NULL) {
        // this or some class instances.
        t = CG->GenLoad(t->GetBase(), t->GetOffset());
    }
    return t;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::CheckDecl() {
    if (!base) {
        Decl *d = symtab->Lookup(field);
        if (d == NULL || !d->IsFunctionDecl()) {
            semantic_error = 1;
    return;
            return;
        } else {
            field->SetDecl(d);
            // if function is defined after this, here semantic_type is NULL.
            semantic_type = d->GetType();
        }
    } else {
        // if has base, then leave the work to CheckType.
        base->Check(E_CheckDecl);
    }
    actuals->CheckAll(E_CheckDecl);
}

void Call::CheckType() {
    if (!base) {
        if (field->GetDecl() && !semantic_type) {
            // if function is defined after this, then assign semantic_type again.
            semantic_type = field->GetDecl()->GetType();
        }
    } else {
        // must check the base expr's class type, and find in that class.
        base->Check(E_CheckType);
        Type * t = base->GetType();
        if (t != NULL) { // base defined.
            if (t->IsArrayType() && !strcmp(field->GetIdName(), "length")) {
                // support the length() method of array type.
                // length must have no argument.
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
                Decl *d = symtab->LookupField(
                        dynamic_cast<NamedType*>(t)->GetId(), field);
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
    actuals->CheckAll(E_CheckType);
    this->CheckFuncArgs();
}

void Call::CheckFuncArgs() {
    Decl *f = field->GetDecl();
    if (!f || !f->IsFunctionDecl()) return; // skip the error decl
    FunctionDecl *fun = dynamic_cast<FunctionDecl*>(f);
    List<VariableDecl*> *formals = fun->GetFormals();

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

void Call::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:;
            // do not check anything.
    }
}

void Call::Emit() {
    PrintDebug("tac+", "Emit Call %s.", field->GetIdName());
    // TODO: in class scope, methon without base should be ACall.

    if (base) base->Emit();
    field->Emit();
    actuals->EmitAll();

    // deal with array.length().
    if (base && base->GetType()->IsArrayType() &&
            !strcmp(field->GetIdName(), "length")) {
        Location *t0 = base->GetEmitLocDeref();
        Location *t1 = CG->GenLoad(t0, -4);
        asm_loc = t1;
        return;
    }

    FunctionDecl *fn = dynamic_cast<FunctionDecl*>(field->GetDecl());
    Assert(fn);
    bool is_ACall = (base != NULL) || (fn->IsClassMember());

    // get VTable entry.
    Location *this_loc;
    if (base) {
        this_loc = base->GetEmitLocDeref(); // VTable entry.
    } else if (fn->IsClassMember()) {
        this_loc = CG->ThisPtr; // in a class scope.
    }

    Location *t;
    if (is_ACall) {
        t = CG->GenLoad(this_loc, 0);
        t = CG->GenLoad(t, fn->GetVTableOffset());
    }

    // PushParam
    for (int i = actuals->NumElements() - 1; i >= 0; i--) {
        Location *l = actuals->Nth(i)->GetEmitLocDeref();
        CG->GenPushParam(l);
    }

    // generate call.
    if (is_ACall) {
        // Push this.
        CG->GenPushParam(this_loc);
        // ACall
        asm_loc = CG->GenACall(t, fn->HasReturnValue());
        // PopParams
        CG->GenPopParams(actuals->NumElements() * 4 + 4);
    } else {
        // LCall
        field->AddPrefix("_"); // main?
        asm_loc = CG->GenLCall(field->GetIdName(),
                semantic_type != Type::voidType);
        // PopParams
        CG->GenPopParams(actuals->NumElements() * 4);
    }
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) {
    Assert(c != NULL);
    (cType=c)->SetParent(this);
}

void NewExpr::CheckDecl() {
    cType->Check(E_CheckDecl, LookingForClass);
}

void NewExpr::CheckType() {
    cType->Check(E_CheckType);
    // cType is NamedType.
    if (cType->GetType()) { // correct cType
        semantic_type = cType;
    }
}

void NewExpr::Check(checkT c) {
    switch (c) {
        case E_CheckDecl:
            this->CheckDecl(); break;
        case E_CheckType:
            this->CheckType(); break;
        default:
            cType->Check(c);
    }
}

void NewExpr::Emit() {
    ClassDecl *d = dynamic_cast<ClassDecl*>(cType->GetId()->GetDecl());
    Assert(d);
    int size = d->GetInstanceSize();
    Location *t = CG->GenLoadConstant(size);
    asm_loc = CG->GenBuiltInCall(Alloc, t);
    Location *l = CG->GenLoadLabel(d->GetId()->GetIdName());
    CG->GenStore(asm_loc, l, 0);
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this);
    (elemType=et)->SetParent(this);
}

void NewArrayExpr::CheckType() {
    Type *t;

    size->Check(E_CheckType);
    t = size->GetType();
    if (t == NULL) {
        // some error accur in size, so skip it.
    } else if (t != Type::intType) {
        semantic_error = 1;
    return;
    }

    elemType->Check(E_CheckType);
    if (!elemType->GetType()) {
        // skip the error elemType.
        return;
    } else {
        // the error size will not affect the type of new array.
        semantic_type = new ArrayType(*location, elemType);
        semantic_type->Check(E_CheckDecl);
    }
}

void NewArrayExpr::Check(checkT c) {
    if (c == E_CheckType) {
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
    Location *t2 = CG->GenBinaryOp("<=", t0, t1);

    const char *l = CG->NewLabel();
    CG->GenIfZ(t2, l);
    Location *t3 = CG->GenLoadConstant(err_arr_bad_size);
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

void ReadIntegerExpr::Check(checkT c) {
    if (c == E_CheckType) {
        semantic_type = Type::intType;
    }
}

void ReadIntegerExpr::Emit() {
    asm_loc = CG->GenBuiltInCall(ReadInteger);
}

void ReadLineExpr::Check(checkT c) {
    if (c == E_CheckType) {
        semantic_type = Type::stringType;
    }
}

void ReadLineExpr::Emit() {
    asm_loc = CG->GenBuiltInCall(ReadLine);
}

PostfixExpr::PostfixExpr(LValue *lv, Operator *o)
    : Expr(Join(lv->GetLocation(), o->GetLocation())) {
    Assert(lv != NULL && o != NULL);
    (lvalue=lv)->SetParent(this);
    (op=o)->SetParent(this);
}

void PostfixExpr::CheckType() {
    lvalue->Check(E_CheckType);
    op->Check(E_CheckType);
    Type *t = lvalue->GetType();
    if (t == NULL) {
        // some error accur in lvalue, so skip it.
    } else if (t != Type::intType) {
        semantic_error = 1;
    return;
    } else {
        semantic_type = t;
    }
}

void PostfixExpr::Check(checkT c) {
    if (c == E_CheckType) {
        this->CheckType();
    } else {
        lvalue->Check(c);
        op->Check(c);
    }
}

void PostfixExpr::Emit() {
    lvalue->Emit();
    // lvalue can be class var member, array access, or any variables.
    Location *l1 = lvalue->GetEmitLoc();
    Location *l2 = lvalue->GetEmitLocDeref();

    // save the original lvalue.
    Location *t0 = CG->GenTempVar();
    CG->GenAssign(t0, l2);

    // postfix expr should emit ++ or -- at the end of itself.
    l2 = CG->GenBinaryOp(strcmp(op->GetOpStr(), "++") ? "-" : "+",
            l2, CG->GenLoadConstant(1));

    // change the value of lvalue.
    if (l1->GetBase() != NULL) {
        CG->GenStore(l1->GetBase(), l2, l1->GetOffset());
    } else if (lvalue->IsArrayAccessRef()) {
        CG->GenStore(l1, l2);
    } else {
        CG->GenAssign(l1, l2);
    }

    // the value of postfix expr is its original lvalue.
    asm_loc = t0;
}

