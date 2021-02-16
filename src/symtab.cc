/* File: symtab.cc
 * ---------------
 * Build the symbol table.
 * Author: Deyuan Guo
 */

#include <iostream>
#include "string.h"

#include "symtab.h"
#include "ast.h"
#include "ast_decl.h"


/* Global Symbol Table, used by all nodes.
 */
SymbolTable *symtab;


/* Scope class, maintain the necessary information of scope structure.
 */
class Scope {
  protected:
    Hashtable<Decl*> *ht;
    const char *parent;                 // record the class inheritance
    std::list<const char *> *interface; // record the interface of class
    const char *owner;                  // record the scope owner for class

  public:
    Scope() {
        ht = NULL;
        parent = NULL;
        interface = new std::list<const char *>;
        interface->clear();
        owner = NULL;
    }

    bool HasHT() { return ht == NULL ? false : true; }
    void BuildHT() { ht = new Hashtable<Decl*>; }
    Hashtable<Decl*> * GetHT() { return ht; }

    bool HasParent() { return parent == NULL ? false : true; }
    void SetParent(const char *p) { parent = p; }
    const char * GetParent() { return parent; }

    bool HasInterface() { return !interface->empty(); }
    void AddInterface(const char *p) { interface->push_back(p); }
    std::list<const char *> * GetInterface() { return interface; }

    bool HasOwner() { return owner == NULL ? false : true; }
    void SetOwner(const char *o) { owner = o; }
    const char * GetOwner() { return owner; }
};

/* Implementation of Symbol Table
 */
SymbolTable::SymbolTable()
{
//    PrintDebug("sttrace", "SymbolTable constructor.\n");
    /* Init the global scope. */
    scopes = new std::vector<Scope *>;
    scopes->clear();
    scopes->push_back(new Scope());

    /* Init the active scopes, and active global scope 0. */
    activeScopes = new std::vector<int>;
    activeScopes->clear();
    activeScopes->push_back(0);

    /* Init scope counter and identifier counter. */
    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}

/*
 * Resert symbol table counter and active scopes for another pass.
 */
void
SymbolTable::ResetSymbolTable() {
//    PrintDebug("sttrace", "======== Reset SymbolTable ========\n");
    activeScopes->clear();
    activeScopes->push_back(0);

    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}

/*
 * Enter a new scope.
 */
void
SymbolTable::BuildScope()
{
//    PrintDebug("sttrace", "Build new scope %d.\n", scope_cnt + 1);
    scope_cnt++;
    scopes->push_back(new Scope());
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Enter a new scope, and set owner for class and interface.
 */
void
SymbolTable::BuildScope(const char *key)
{
//    PrintDebug("sttrace", "Build new scope %d.\n", scope_cnt + 1);
    scope_cnt++;
    scopes->push_back(new Scope());
    scopes->at(scope_cnt)->SetOwner(key);
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Enter a new scope.
 */
void
SymbolTable::EnterScope()
{
//    PrintDebug("sttrace", "Enter scope %d.\n", scope_cnt + 1);
    scope_cnt++;
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}

/*
 * Find scope from owner name.
 */
int
SymbolTable::FindScopeFromOwnerName(const char *key)
{
    int scope = -1;

    for (int i = 0; i < scopes->size(); i++) {
        if (scopes->at(i)->HasOwner()) {
            if (!strcmp(key, scopes->at(i)->GetOwner())) {
                scope = i;;
                break;
            }
        }
    }

//    PrintDebug("sttrace", "From %s find scope %d.\n", key, scope);
    return scope;
}

/*
 * Look up symbol in all active scopes.
 */
Decl *
SymbolTable::Lookup(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();
//    PrintDebug("sttrace", "Lookup %s from active scopes %d.\n", key, cur_scope);

    //printf("Look up %s from scope %d\n", key, cur_scope);

    // Look up all the active scopes.
    for (int i = activeScopes->size(); i > 0; --i) {

        int scope = activeScopes->at(i-1);
        Scope *s = scopes->at(scope);

        if (s->HasHT()) {
            d = s->GetHT()->Lookup(key);
        }
        if (d != NULL) break;

        while (s->HasParent()) {
            parent = s->GetParent();
            scope = FindScopeFromOwnerName(parent);
            if (scope != -1) {
                if (scope == cur_scope) {
                    // If the parent relation has a loop, then report an error.
                    break;
                } else {
                    s = scopes->at(scope);
                    if (s->HasHT()) {
                        d = s->GetHT()->Lookup(key);
                    }
                    if (d != NULL) break;
                }
            } else {
                break;
            }
        }
        if (d != NULL) break;
    }

    return d;
}

/*
 * Look up symbol in parent scopes.
 */
Decl *
SymbolTable::LookupParent(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();
    Scope *s = scopes->at(cur_scope);
//    PrintDebug("sttrace", "Lookup %s in parent of %d.\n", key, cur_scope);

    // Look up parent scopes.
    while (s->HasParent()) {
        parent = s->GetParent();
        int scope = FindScopeFromOwnerName(parent);
        //printf("Look up %s from %s\n", key, parent);

        if (scope != -1) {
            if (scope == cur_scope) {
                // If the parent relation has a loop, then report an error.
                break;
            } else {
                s = scopes->at(scope);
                if (s->HasHT()) {
                    d = s->GetHT()->Lookup(key);
                }
                if (d != NULL) break;
            }
        }
    }

    return d;
}

/*
 * Look up symbol in interface scopes.
 */
Decl *
SymbolTable::LookupInterface(Identifier *id)
{
    Decl *d = NULL;
    const char *key = id->GetIdName();
    int scope;
    Scope *s = scopes->at(cur_scope);
//    PrintDebug("sttrace", "Lookup %s in interface of %d.\n", key, cur_scope);

    // Look up interface scopes.
    if (s->HasInterface()) {

        std::list<const char *> * itfc = s->GetInterface();

        for (std::list<const char *>::iterator it = itfc->begin();
                it != itfc->end(); it++) {
            scope = FindScopeFromOwnerName(*it);
            //printf("Look up %s from %s\n", key, *it);

            if (scope != -1) {
                Scope *sc = scopes->at(scope);
                if (sc->HasHT()) {
                    d = sc->GetHT()->Lookup(key);
                }
                if (d != NULL) break;
            }
        }
    }
    return d;
}

/*
 * Look up symbol in a given class/interface name.
 */
Decl * SymbolTable::LookupField(Identifier *base, Identifier *field) {
    Decl *d = NULL;
    const char *b = base->GetIdName();
    const char *f = field->GetIdName();
//    PrintDebug("sttrace", "Lookup %s from field %s\n", f, b);

    // find scope from field name.
    int scope = FindScopeFromOwnerName(b);
    if (scope == -1) return NULL;

    // lookup the given field.
    Scope *s = scopes->at(scope);
    if (s->HasHT()) {
        d = s->GetHT()->Lookup(f);
    }
    if (d != NULL) return d;

    // lookup the parent.
    while (s->HasParent()) {
        b = s->GetParent();
        scope = FindScopeFromOwnerName(b);
        if (scope != -1) {
            if (scope == cur_scope) {
                // If the parent relation has a loop, then report an error.
                break;
            } else {
                s = scopes->at(scope);
                if (s->HasHT()) {
                    d = s->GetHT()->Lookup(f);
                }
                if (d != NULL) break;
            }
        } else {
            break;
        }
    }
    return d;
}

/*
 * Look up the class decl for This.
 */
Decl * SymbolTable::LookupThis() {
//    PrintDebug("sttrace", "Lookup This\n");
    Decl *d = NULL;
    // Look up all the active scopes.
    for (int i = activeScopes->size(); i > 0; --i) {

        int scope = activeScopes->at(i-1);
        Scope *s = scopes->at(scope);

        if (s->HasOwner()) {
//            PrintDebug("sttrace", "Lookup This as %s\n", s->GetOwner());
            // Look up scope 0 to find the class decl.
            Scope *s0 = scopes->at(0);
            if (s0->HasHT()) {
                d = s0->GetHT()->Lookup(s->GetOwner());
            }
        }
        if (d) break;
    }
    return d;
}

/*
 * Insert new symbol into current scope.
 */
int
SymbolTable::InsertSymbol(Decl *decl)
{
    const char *key = decl->GetId()->GetIdName();
    Scope *s = scopes->at(cur_scope);
//    PrintDebug("sttrace", "Insert %s to scope %d\n", key, cur_scope);

    if (!s->HasHT()) {
        s->BuildHT();
    }

    s->GetHT()->Enter(key, decl);
    return id_cnt++;
}

/*
 * Look up symbol in current scope.
 */
bool
SymbolTable::LocalLookup(Identifier *id)
{
    Decl *d = NULL;
    const char *key = id->GetIdName();
    Scope *s = scopes->at(cur_scope);
//    PrintDebug("sttrace", "LocalLookup %s from scope %d\n", key, cur_scope);

    if (s->HasHT()) {
        d = s->GetHT()->Lookup(key);
    }

    return (d == NULL) ? false : true;
}

/*
 * Exit current scope and return to its uplevel scope.
 */
void
SymbolTable::ExitScope()
{
//    PrintDebug("sttrace", "Exit scope %d\n", cur_scope);
    activeScopes->pop_back();
    cur_scope = activeScopes->back();
}

/*
 * Deal with class inheritance, set parent for a subclass.
 */
void
SymbolTable::SetScopeParent(const char *key)
{
    scopes->at(cur_scope)->SetParent(key);
}

/*
 * Deal with class interface, set interfaces for a subclass.
 */
void
SymbolTable::SetInterface(const char *key)
{
    scopes->at(cur_scope)->AddInterface(key);
}

/*
 * Print the whole symbol table.
 */
void SymbolTable::Print()
{
    std::cout << std::endl << "======== Symbol Table ========" << std::endl;

    for (int i = 0; i < scopes->size(); i++) {
        Scope *s = scopes->at(i);

        if (!s->HasHT() && !s->HasOwner() && !s->HasParent()
                && !s->HasInterface()) continue;

        std::cout << "|- Scope " << i << ":";
        if (s->HasOwner())
            std::cout << " (owner: " << s->GetOwner() << ")";
        if (s->HasParent())
            std::cout << " (parent: " << s->GetParent() << ")";
        if (s->HasInterface()) {
            std::cout << " (interface: ";
            std::list<const char *> *interface = s->GetInterface();
            for (std::list<const char *>::iterator it = interface->begin();
                    it != interface->end(); it++) {
                std::cout << *it << " ";
            }
            std::cout << ")";
        }

        std::cout << std::endl;

        if (s->HasHT()) {
            Iterator<Decl*> iter = s->GetHT()->GetIterator();
            Decl *decl;
            while ((decl = iter.GetNextValue()) != NULL) {
                std::cout << "|  + " << decl << std::endl;
            }
        }
    }

    std::cout << "======== Symbol Table ========" << std::endl;
}

