/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    ;
    (decls=d)->SetParentAll(this);
}

void Program::BuildSymTable() {
    symtab = new SymbolTable();

    /* Pass 1: Traverse the AST and build the symbol table. Report the
     * errors of declaration conflict in any local scopes. */
    decls->BuildSymTableAll();
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */

    /* Pass 2: Traverse the AST and report any errors of undeclared
     * identifiers except the field access and function calls. */
    symtab->ResetSymbolTable();
    decls->CheckAll(sem_decl);

    /* Pass 3: Traverse the AST and report errors related to the class and
     * interface inheritance. */
    symtab->ResetSymbolTable();
    decls->CheckAll(sem_inh);

    /* Pass 4: Traverse the AST and report errors related to types, function
     * calls and field access. Actually, check all the remaining errors. */
    symtab->ResetSymbolTable();
    decls->CheckAll(sem_type);
}

void Program::Emit() {
    /* pp5: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */

    // Check if there exists a global main function.
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

//    PrintDebug("tac+", "Assign offset for class/interface members & global.");
    // Assign offset for global var, class/interface members.
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AssignOffset();
    }
    // Add prefix for functions.
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AddPrefixToMethods();
    }
//    if (IsDebugOn("tac+")) { this->Print(0); }

//    PrintDebug("tac+", "Begin Emitting TAC for Program.");
    decls->EmitAll();
//    if (IsDebugOn("tac+")) { this->Print(0); }

    if(semantic_error != 0)
        return;

    // Emit the TAC or final MIPS assembly code.
    CG->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VariableDecl*> *d, List<Stmt*> *s) {
    ;
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}



void StmtBlock::BuildSymTable() {
    symtab->BuildScope();
    decls->BuildSymTableAll();
    stmts->BuildSymTableAll();
    symtab->ExitScope();
}

void StmtBlock::Check(checkStep c) {
    symtab->EnterScope();
    decls->CheckAll(c);
    stmts->CheckAll(c);
    symtab->ExitScope();
}

void StmtBlock::Emit() {
    decls->EmitAll();
    stmts->EmitAll();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) {
    ;
    (test=t)->SetParent(this);
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) {
    ;
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}



void ForStmt::BuildSymTable() {
    symtab->BuildScope();
    body->BuildSymTable();
    symtab->ExitScope();
}

void ForStmt::CheckType() {
    init->Check(sem_type);
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    step->Check(sem_type);
    symtab->EnterScope();
    body->Check(sem_type);
    symtab->ExitScope();
}

void ForStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType(); break;
        default:
            init->Check(c);
            test->Check(c);
            step->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
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
    symtab->BuildScope();
    body->BuildSymTable();
    symtab->ExitScope();
}

void WhileStmt::CheckType() {
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    symtab->EnterScope();
    body->Check(sem_type);
    symtab->ExitScope();
}

void WhileStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType(); break;
        default:
            test->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
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

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) {
    ; // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}



void IfStmt::BuildSymTable() {
    symtab->BuildScope();
    body->BuildSymTable();
    symtab->ExitScope();
    if (elseBody) {
        symtab->BuildScope();
        elseBody->BuildSymTable();
        symtab->ExitScope();
    }
}

void IfStmt::CheckType() {
    test->Check(sem_type);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    symtab->EnterScope();
    body->Check(sem_type);
    symtab->ExitScope();
    if (elseBody) {
        symtab->EnterScope();
        elseBody->Check(sem_type);
        symtab->ExitScope();
    }
}

void IfStmt::Check(checkStep c) {
    switch (c) {
        case sem_type:
            this->CheckType(); break;
        default:
            test->Check(c);
            symtab->EnterScope();
            body->Check(c);
            symtab->ExitScope();
            if (elseBody) {
                symtab->EnterScope();
                elseBody->Check(c);
                symtab->ExitScope();
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
    // break can jump out of a while, for, or a switch.
    Node *n = this;
    while (n->GetParent()) {
        if (n->IsLoopStmt()) {
            const char *l = dynamic_cast<LoopStmt*>(n)->GetEndLoopLabel();
//            PrintDebug("tac+", "endloop label %s.", l);
            CG->GenGoto(l);
            return;
        } else if (n->IsSwitchStmt()) {
            const char *l = dynamic_cast<SwitchStmt*>(n)->GetEndSwitchLabel();
//            PrintDebug("tac+", "endswitch label %s.", l);
            CG->GenGoto(l);
            return;
        }
        n = n->GetParent();
    }
}

CaseStmt::CaseStmt(IntLiteral *v, List<Stmt*> *s) {
    ;
    value = v;
    if (value) value->SetParent(this);
    (stmts=s)->SetParentAll(this);
    case_label = NULL;
}



void CaseStmt::BuildSymTable() {
    symtab->BuildScope();
    stmts->BuildSymTableAll();
    symtab->ExitScope();
}

void CaseStmt::Check(checkStep c) {
    if (value) value->Check(c);
    symtab->EnterScope();
    stmts->CheckAll(c);
    symtab->ExitScope();
}

void CaseStmt::GenCaseLabel() {
    case_label = CG->NewLabel();
}

void CaseStmt::Emit() {
    CG->GenLabel(case_label);
    stmts->EmitAll();
}

SwitchStmt::SwitchStmt(Expr *e, List<CaseStmt*> *c) {
    ;
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    end_switch_label = NULL;
}



void SwitchStmt::BuildSymTable() {
    symtab->BuildScope();
    cases->BuildSymTableAll();
    symtab->ExitScope();
}

void SwitchStmt::Check(checkStep c) {
    expr->Check(c);
    symtab->EnterScope();
    cases->CheckAll(c);
    symtab->ExitScope();
}

void SwitchStmt::Emit() {
    expr->Emit();

    // the end_switch_label is used by break.
    end_switch_label = CG->NewLabel();

    Location *switch_value = expr->GetEmitLocDeref();

    // here use a series of if instead of an address table.
    // case statement is optional, default statement is optional.
    // default statement is always at the end of the cases list.
    for (int i = 0; i < cases->NumElements(); i++) {
        CaseStmt *c = cases->Nth(i);

        // get case label.
        c->GenCaseLabel();
        const char *cl = c->GetCaseLabel();

        // get case value.
        IntLiteral *cv = c->GetCaseValue();

        // gen branches.
        if (cv) {
            // case
            cv->Emit();
            Location *cvl = cv->GetEmitLocDeref();
            Location *t = CG->GenBinaryOp("!=", switch_value, cvl);
            CG->GenIfZ(t, cl);
        } else {
            // default
            CG->GenGoto(cl);
        }
    }

    // emit case statements.
    cases->EmitAll();

    // gen end_switch_label.
    CG->GenLabel(end_switch_label);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {
    ;
    (expr=e)->SetParent(this);
}



void ReturnStmt::Check(checkStep c) {
    expr->Check(c);
    if (c == sem_type) {
        Node *n = this;
        // find the FunctionDecl.
        while (n->GetParent()) {
            if (dynamic_cast<FunctionDecl*>(n) != NULL) break;
            n = n->GetParent();
        }
        Type *t_given = expr->GetType();
        Type *t_expected = dynamic_cast<FunctionDecl*>(n)->GetType();
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

PrintStmt::PrintStmt(List<Expr*> *a) {
    ;
    (args=a)->SetParentAll(this);
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
        // BuiltInCall
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
        ;
        CG->GenBuiltInCall(f, l);
    }

    BuiltIn f = PrintString;
    char const * str = "\\n";
    Location *l = CG->GenLoadConstant(str);
    ;
    CG->GenBuiltInCall(f, l);
}

