/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

void Program::BuildST() {
    if (IsDebugOn("ast")) { this->Print(0); }
    symtab = new SymbolTable();

    /* Pass 1: Traverse the AST and build the symbol table. Report the
     * errors of declaration conflict in any local scopes. */
    decls->BuildSTAll();
    if (IsDebugOn("st")) { symtab->Print(); }
    PrintDebug("ast+", "BuildST finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }
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
    decls->CheckAll(E_CheckDecl);
    PrintDebug("ast+", "CheckDecl finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }

    /* Pass 3: Traverse the AST and report errors related to the class and
     * interface inheritance. */
    symtab->ResetSymbolTable();
    decls->CheckAll(E_CheckInherit);
    PrintDebug("ast+", "CheckInherit finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }

    /* Pass 4: Traverse the AST and report errors related to types, function
     * calls and field access. Actually, check all the remaining errors. */
    symtab->ResetSymbolTable();
    decls->CheckAll(E_CheckType);
    PrintDebug("ast+", "CheckType finished.");
    if (IsDebugOn("ast+")) { this->Print(0); }
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

    PrintDebug("tac+", "Assign offset for class/interface members & global.");
    // Assign offset for global var, class/interface members.
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AssignOffset();
    }
    // Add prefix for functions.
    for (int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->AddPrefixToMethods();
    }
    if (IsDebugOn("tac+")) { this->Print(0); }

    PrintDebug("tac+", "Begin Emitting TAC for Program.");
    decls->EmitAll();
    if (IsDebugOn("tac+")) { this->Print(0); }

    if(semantic_error != 0)
        return;

    // Emit the TAC or final MIPS assembly code.
    CG->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VariableDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

void StmtBlock::BuildST() {
    symtab->BuildScope();
    decls->BuildSTAll();
    stmts->BuildSTAll();
    symtab->ExitScope();
}

void StmtBlock::Check(checkT c) {
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
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this);
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) {
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void ForStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
}

void ForStmt::CheckType() {
    init->Check(E_CheckType);
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    step->Check(E_CheckType);
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
}

void ForStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
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

void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
}

void WhileStmt::CheckType() {
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
}

void WhileStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
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
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}

void IfStmt::BuildST() {
    symtab->BuildScope();
    body->BuildST();
    symtab->ExitScope();
    if (elseBody) {
        symtab->BuildScope();
        elseBody->BuildST();
        symtab->ExitScope();
    }
}

void IfStmt::CheckType() {
    test->Check(E_CheckType);
    if (test->GetType() && test->GetType() != Type::boolType) {
        semantic_error = 1;
        return;
    }
    symtab->EnterScope();
    body->Check(E_CheckType);
    symtab->ExitScope();
    if (elseBody) {
        symtab->EnterScope();
        elseBody->Check(E_CheckType);
        symtab->ExitScope();
    }
}

void IfStmt::Check(checkT c) {
    switch (c) {
        case E_CheckType:
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

void BreakStmt::Check(checkT c) {
    if (c == E_CheckType) {
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
            PrintDebug("tac+", "endloop label %s.", l);
            CG->GenGoto(l);
            return;
        } else if (n->IsSwitchStmt()) {
            const char *l = dynamic_cast<SwitchStmt*>(n)->GetEndSwitchLabel();
            PrintDebug("tac+", "endswitch label %s.", l);
            CG->GenGoto(l);
            return;
        }
        n = n->GetParent();
    }
}

CaseStmt::CaseStmt(IntLiteral *v, List<Stmt*> *s) {
    Assert(s != NULL);
    value = v;
    if (value) value->SetParent(this);
    (stmts=s)->SetParentAll(this);
    case_label = NULL;
}

void CaseStmt::PrintChildren(int indentLevel) {
    if (value) value->Print(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

void CaseStmt::BuildST() {
    symtab->BuildScope();
    stmts->BuildSTAll();
    symtab->ExitScope();
}

void CaseStmt::Check(checkT c) {
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
    Assert(e != NULL && c != NULL);
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    end_switch_label = NULL;
}

void SwitchStmt::PrintChildren(int indentLevel) {
    expr->Print(indentLevel+1);
    cases->PrintAll(indentLevel+1);
}

void SwitchStmt::BuildST() {
    symtab->BuildScope();
    cases->BuildSTAll();
    symtab->ExitScope();
}

void SwitchStmt::Check(checkT c) {
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
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    expr->Print(indentLevel+1);
}

void ReturnStmt::Check(checkT c) {
    expr->Check(c);
    if (c == E_CheckType) {
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
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::PrintChildren(int indentLevel) {
    args->PrintAll(indentLevel+1, "(args) ");
}

void PrintStmt::Check(checkT c) {
    args->CheckAll(c);
    if (c == E_CheckType) {
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
        Assert(l);
        CG->GenBuiltInCall(f, l);
    }

    BuiltIn f = PrintString;
    char const * str = "\\n";
    Location *l = CG->GenLoadConstant(str);
    Assert(l);
    CG->GenBuiltInCall(f, l);
}

