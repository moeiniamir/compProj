/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "globals.h"
#include "list.h"


Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
    idx = -1;
    semantic_type = NULL;
}


VariableDecl::VariableDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    class_member_ofst = -1;
}

void VariableDecl::PrintChildren(int indentLevel) {
   if (semantic_type) std::cout << " <" << semantic_type << ">";
   if (asm_loc) asm_loc->Print();
   if (class_member_ofst != -1)
       std::cout << " ~~[Ofst: " << class_member_ofst << "]";
   type->Print(indentLevel+1);
   id->Print(indentLevel+1);
   if (id->GetDecl()) printf(" ........ {def}");
}

void VariableDecl::BuildSymTable() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = symtab->InsertSymbol(this);
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
            this->CheckDecl(); break;
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
    // set location for var members of class.
    asm_loc = new Location(fpRelative, offset, id->GetIdName(), CG->ThisPtr);
}

void VariableDecl::Emit() {
    if (type == Type::doubleType) {
        semantic_error = 1;
        return;
    }

    if (!asm_loc) {
        // some auto variables.
        asm_loc = new Location(fpRelative, CG->GetNextLocalLoc(),
                id->GetIdName());
    }
}

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
    instance_size = 4;
    vtable_size = 0;
}

void ClassDecl::PrintChildren(int indentLevel) {
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    if (extends) extends->Print(indentLevel+1, "(extends) ");
    implements->PrintAll(indentLevel+1, "(implements) ");
    members->PrintAll(indentLevel+1);
}

void ClassDecl::BuildSymTable() {
    if (symtab->LocalLookup(this->GetId())) {
        // if two local symbols have the same name, then report an error.
        Decl *d = symtab->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    // record the owner of the current class scope.
    symtab->BuildScope(this->GetId()->GetIdName());
    if (extends) {
        // record the parent of the current class.
        symtab->SetScopeParent(extends->GetId()->GetIdName());
    }
    // record the implements of the current class.
    for (int i = 0; i < implements->NumElements(); i++) {
        symtab->SetInterface(implements->Nth(i)->GetId()->GetIdName());
    }
    members->BuildSymTableAll();
    symtab->ExitScope();
}

void ClassDecl::CheckDecl() {
    id->Check(sem_decl);
    // check extends.
    if (extends) {
        extends->Check(sem_decl, classReason);
    }
    // check interface.
    for (int i = 0; i < implements->NumElements(); i++) {
        implements->Nth(i)->Check(sem_decl, interfaceReason);
    }
    symtab->EnterScope();
    members->CheckAll(sem_decl);
    symtab->ExitScope();

    semantic_type = new NamedType(id);
    semantic_type->SetSelfType();
}

void ClassDecl::CheckInherit() {
    symtab->EnterScope();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        Assert(d != NULL); // already inserted into symbol table.

        if (d->IsVariableDecl()) {
            // check class inheritance of variables.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                // subclass cannot override inherited variables.
                semantic_error = 1;
                return;
            }
            // check interface inheritance of variables.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // variable names conflict with interface method names.
                semantic_error = 1;
                return;
            }

        } else if (d->IsFunctionDecl()) {
            // check class inheritance of functions.
            Decl *t = symtab->LookupParent(d->GetId());
            if (t != NULL) {
                if (!t->IsFunctionDecl()) {
                    semantic_error = 1;
                    return;
                } else {
                    // compare the function signature.
                    FunctionDecl *fn1 = dynamic_cast<FunctionDecl*>(d);
                    FunctionDecl *fn2 = dynamic_cast<FunctionDecl*>(t);
                    if (fn1->GetType() && fn2->GetType() // correct return type
                            && !fn1->IsEquivalentTo(fn2)) {
                        // subclass cannot overload inherited functions.
                        semantic_error = 1;
                        return;
                    }
                }
            }
            // check interface inheritance of functions.
            t = symtab->LookupInterface(d->GetId());
            if (t != NULL) {
                // compare the function signature.
                FunctionDecl *fn1 = dynamic_cast<FunctionDecl*>(d);
                FunctionDecl *fn2 = dynamic_cast<FunctionDecl*>(t);
                if (fn1->GetType() && fn2->GetType() // correct return type
                        && !fn1->IsEquivalentTo(fn2)) {
                    // subclass cannot overload inherited functions.
                    semantic_error = 1;
                    return;
                }
            }
            // check scopes within the function and keep active scope correct.
            d->Check(sem_inh);
        }
    }

    // Check the full implementation of interface methods.
    for (int i = 0; i < implements->NumElements(); i++) {
        Decl *d = implements->Nth(i)->GetId()->GetDecl();
        if (d != NULL) {
            List<Decl*> *m = dynamic_cast<InterfaceDecl*>(d)->GetMembers();
            // check the members of each interface.
            for (int j = 0; j < m->NumElements(); j++) {
                Identifier *mid = m->Nth(j)->GetId();
                Decl *t = symtab->LookupField(this->id, mid);
                if (t == NULL) {
                    semantic_error = 1;
                    return;
                    break;
                } else {
                    // check the function signature.
                    FunctionDecl *fn1 = dynamic_cast<FunctionDecl*>(m->Nth(j));
                    FunctionDecl *fn2 = dynamic_cast<FunctionDecl*>(t);
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
    symtab->ExitScope();
}

void ClassDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            this->CheckDecl(); break;
        case sem_inh:
            this->CheckInherit(); break;
        default:
            id->Check(c);
            if (extends) extends->Check(c);
            implements->CheckAll(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
}

bool ClassDecl::IsChildOf(Decl *other) {
    if (other->IsClassDecl()) {
        if (id->IsEquivalentTo(other->GetId())) {
            // self.
            return true;
        } else if (!extends) {
            return false;
        } else {
            // look up all the parent classes.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
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
            // look up all the parent classes' interfaces.
            Decl *d = extends->GetId()->GetDecl();
            return dynamic_cast<ClassDecl*>(d)->IsChildOf(other);
        }
    } else {
        return false;
    }
}

void ClassDecl::AddMembersToList(List<VariableDecl*> *vars, List<FunctionDecl*> *fns) {
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVariableDecl()) {
            vars->InsertAt(dynamic_cast<VariableDecl*>(d), 0);
        } else if (d->IsFunctionDecl()) {
            fns->InsertAt(dynamic_cast<FunctionDecl*>(d), 0);
        }
    }
}

void ClassDecl::AssignOffset() {
    // deal with class inheritance.
    // add all parents' methods.
    var_members = new List<VariableDecl*>;
    methods = new List<FunctionDecl*>;
    ClassDecl *c = this;
    while (c) {
        c->AddMembersToList(var_members, methods);
        NamedType *t = c->GetExtends();
        if (!t) break;
        c = dynamic_cast<ClassDecl*>(t->GetId()->GetDecl());
    }

    // delete override methods.
    for (int i = 0; i < methods->NumElements(); i++) {
        FunctionDecl *f1 = methods->Nth(i);
        for (int j = i + 1; j < methods->NumElements(); j++) {
            FunctionDecl *f2 = methods->Nth(j);
            if (!strcmp(f1->GetId()->GetIdName(), f2->GetId()->GetIdName())) {
                // replace the parent's method with child's method, then the
                // order of the methods can be compatible with both of them.
                methods->RemoveAt(i);
                methods->InsertAt(f2, i);
                methods->RemoveAt(j);
                j--;
            }
        }
    }

    PrintDebug("tac+", "Class Methods of %s:", id->GetIdName());
    for (int i = 0; i < methods->NumElements(); i++) {
        PrintDebug("tac+", "%s", methods->Nth(i)->GetId()->GetIdName());
    }
    PrintDebug("tac+", "Class Vars of %s:", id->GetIdName());
    for (int i = 0; i < var_members->NumElements(); i++) {
        PrintDebug("tac+", "%s", var_members->Nth(i)->GetId()->GetIdName());
    }

    // assign offset for fn members.
    instance_size = var_members->NumElements() * 4 + 4;
    vtable_size = methods->NumElements() * 4;

    int var_offset = instance_size;
    for (int i = members->NumElements() - 1; i >= 0; i--) {
        Decl *d = members->Nth(i);
        if (d->IsVariableDecl()) {
            var_offset -= 4;
            d->AssignMemberOffset(true, var_offset);
        } else if (d->IsFunctionDecl()) {
            // find the right offset.
            for (int i = 0; i < methods->NumElements(); i++) {
                FunctionDecl *f1 = methods->Nth(i);
                if (!strcmp(f1->GetId()->GetIdName(), d->GetId()->GetIdName()))
                    d->AssignMemberOffset(true, i * 4);
            }
        }
    }
}

void ClassDecl::AddPrefixToMethods() {
    // add prefix to method names.
    for (int i = 0; i < members->NumElements(); i++) {
        members->Nth(i)->AddPrefixToMethods();
    }
}

void ClassDecl::Emit() {
    PrintDebug("tac+", "Begin Emitting TAC in ClassDecl.");

    members->EmitAll();

    // Emit VTable.
    List<const char*> *method_labels = new List<const char*>;
    for (int i = 0; i < methods->NumElements(); i++) {
        FunctionDecl* fn = methods->Nth(i);
        PrintDebug("tac+", "Insert %s into VTable.", fn->GetId()->GetIdName());
        method_labels->Append(fn->GetId()->GetIdName());
    }
    CG->GenVTable(id->GetIdName(), method_labels);
}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::PrintChildren(int indentLevel) {
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    members->PrintAll(indentLevel+1);
}

void InterfaceDecl::BuildSymTable() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope(this->GetId()->GetIdName());
    members->BuildSymTableAll();
    symtab->ExitScope();
}

void InterfaceDecl::Check(checkStep c) {
    switch (c) {
        case sem_decl:
            semantic_type = new NamedType(id);
            semantic_type->SetSelfType();
            // fall through.
        default:
            id->Check(c);
            symtab->EnterScope();
            members->CheckAll(c);
            symtab->ExitScope();
    }
}

void InterfaceDecl::Emit() {
    semantic_error = 1;
    return;
    Assert(0);
}

FunctionDecl::FunctionDecl(Identifier *n, Type *r, List<VariableDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    vtable_ofst = -1;
}

void FunctionDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}

void FunctionDecl::PrintChildren(int indentLevel) {
    if (semantic_type) std::cout << " <" << semantic_type << ">";
    if (asm_loc) asm_loc->Print();
    if (vtable_ofst != -1)
        std::cout << " ~~[VTable: " << vtable_ofst << "]";
    returnType->Print(indentLevel+1, "(return type) ");
    id->Print(indentLevel+1);
    if (id->GetDecl()) printf(" ........ {def}");
    formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

void FunctionDecl::BuildSymTable() {
    if (symtab->LocalLookup(this->GetId())) {
        Decl *d = symtab->Lookup(this->GetId());
        semantic_error = 1;
        return;
    } else {
        idx = symtab->InsertSymbol(this);
        id->SetDecl(this);
    }
    symtab->BuildScope();
    formals->BuildSymTableAll();
    if (body) body->BuildSymTable(); // function body must be a StmtBlock.
    symtab->ExitScope();
}

void FunctionDecl::CheckDecl() {
    returnType->Check(sem_decl);
    id->Check(sem_decl);
    symtab->EnterScope();
    formals->CheckAll(sem_decl);
    if (body) body->Check(sem_decl);
    symtab->ExitScope();

    // check the signature of the main function.
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
            this->CheckDecl(); break;
        default:
            returnType->Check(c);
            id->Check(c);
            symtab->EnterScope();
            formals->CheckAll(c);
            if (body) body->Check(c);
            symtab->ExitScope();
    }
}

bool FunctionDecl::IsEquivalentTo(Decl *other) {
    Assert(this->GetType() && other->GetType());

    if (!other->IsFunctionDecl()) {
        return false;
    }
    FunctionDecl *fn = dynamic_cast<FunctionDecl*>(other);
    if (!returnType->IsEquivalentTo(fn->GetType())) {
        return false;
    }
    if (formals->NumElements() != fn->GetFormals()->NumElements()) {
        return false;
    }
    for (int i = 0; i < formals->NumElements(); i++) {
        // must be VariableDecls.
        Type *var_type1 =
            (dynamic_cast<VariableDecl*>(formals->Nth(i)))->GetType();
        Type *var_type2 =
            (dynamic_cast<VariableDecl*>(fn->GetFormals()->Nth(i)))->GetType();
        if (!var_type1->IsEquivalentTo(var_type2)) {
            return false;
        }
    }
    return true;
}

void FunctionDecl::AddPrefixToMethods() {
    // add prefix for all functions.
    // Add prefix to all the function name except the global main.
    Decl *d = dynamic_cast<Decl*>(this->GetParent());
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
    PrintDebug("tac+", "Begin Emitting TAC in FunctionDecl.");
    if (returnType == Type::doubleType) {
        semantic_error = 1;
        return;
        Assert(0);
    }

    Decl *d = dynamic_cast<Decl*>(this->GetParent());
    CG->GenLabel(id->GetIdName());

    // BeginFunc will reset the FP offset counter.
    BeginFunc *f = CG->GenBeginFunc();

    // Add 4 to the offset of the 1st param for class member.
    if (d && d->IsClassDecl()) {
        CG->GetNextParamLoc();
    }

    // Generate all the Locations for formals.
    for (int i = 0; i < formals->NumElements(); i++) {
        VariableDecl *v = formals->Nth(i);
        if (v->GetType() == Type::doubleType) {
            semantic_error = 1;
            return;
            Assert(0);
        }
        Location *l = new Location(fpRelative, CG->GetNextParamLoc(),
                v->GetId()->GetIdName());
        v->SetEmitLoc(l);
    }

    if (body) body->Emit();

    // Backpatch the frame size.
    f->SetFrameSize(CG->GetFrameSize());

    CG->GenEndFunc();
}

