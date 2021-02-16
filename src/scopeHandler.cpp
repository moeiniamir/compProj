

#include <iostream>
#include "string.h"

#include "scopeHandler.h"
#include "ast.h"



SymbolTable *scopeHandler;



class Scope {
  protected:
    Hashtable<Decl*> *ht;
    const char *parent;
    std::list<const char *> *interface;
    const char *owner;

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


SymbolTable::SymbolTable()
{


    scopes = new std::vector<Scope *>;
    scopes->clear();
    scopes->push_back(new Scope());


    activeScopes = new std::vector<int>;
    activeScopes->clear();
    activeScopes->push_back(0);


    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}


void
SymbolTable::ResetSymbolTable() {

    activeScopes->clear();
    activeScopes->push_back(0);

    cur_scope = 0;
    scope_cnt = 0;
    id_cnt = 0;
}


void
SymbolTable::BuildScope()
{

    scope_cnt++;
    scopes->push_back(new Scope());
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}


void
SymbolTable::BuildScope(const char *key)
{

    scope_cnt++;
    scopes->push_back(new Scope());
    scopes->at(scope_cnt)->SetOwner(key);
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}


void
SymbolTable::EnterScope()
{

    scope_cnt++;
    activeScopes->push_back(scope_cnt);
    cur_scope = scope_cnt;
}


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


    return scope;
}


Decl *
SymbolTable::Lookup(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();





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


Decl *
SymbolTable::LookupParent(Identifier *id)
{
    Decl *d = NULL;
    const char *parent = NULL;
    const char *key = id->GetIdName();
    Scope *s = scopes->at(cur_scope);



    while (s->HasParent()) {
        parent = s->GetParent();
        int scope = FindScopeFromOwnerName(parent);


        if (scope != -1) {
            if (scope == cur_scope) {

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


Decl *
SymbolTable::LookupInterface(Identifier *id)
{
    Decl *d = NULL;
    const char *key = id->GetIdName();
    int scope;
    Scope *s = scopes->at(cur_scope);



    if (s->HasInterface()) {

        std::list<const char *> * itfc = s->GetInterface();

        for (std::list<const char *>::iterator it = itfc->begin();
                it != itfc->end(); it++) {
            scope = FindScopeFromOwnerName(*it);


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


Decl * SymbolTable::LookupField(Identifier *base, Identifier *field) {
    Decl *d = NULL;
    const char *b = base->GetIdName();
    const char *f = field->GetIdName();



    int scope = FindScopeFromOwnerName(b);
    if (scope == -1) return NULL;


    Scope *s = scopes->at(scope);
    if (s->HasHT()) {
        d = s->GetHT()->Lookup(f);
    }
    if (d != NULL) return d;


    while (s->HasParent()) {
        b = s->GetParent();
        scope = FindScopeFromOwnerName(b);
        if (scope != -1) {
            if (scope == cur_scope) {

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


Decl * SymbolTable::LookupThis() {

    Decl *d = NULL;

    for (int i = activeScopes->size(); i > 0; --i) {

        int scope = activeScopes->at(i-1);
        Scope *s = scopes->at(scope);

        if (s->HasOwner()) {


            Scope *s0 = scopes->at(0);
            if (s0->HasHT()) {
                d = s0->GetHT()->Lookup(s->GetOwner());
            }
        }
        if (d) break;
    }
    return d;
}


int
SymbolTable::InsertSymbol(Decl *decl)
{
    const char *key = decl->GetId()->GetIdName();
    Scope *s = scopes->at(cur_scope);


    if (!s->HasHT()) {
        s->BuildHT();
    }

    s->GetHT()->Enter(key, decl);
    return id_cnt++;
}


bool
SymbolTable::LocalLookup(Identifier *id)
{
    Decl *d = NULL;
    const char *key = id->GetIdName();
    Scope *s = scopes->at(cur_scope);


    if (s->HasHT()) {
        d = s->GetHT()->Lookup(key);
    }

    return (d == NULL) ? false : true;
}


void
SymbolTable::ExitScope()
{

    activeScopes->pop_back();
    cur_scope = activeScopes->back();
}


void
SymbolTable::SetScopeParent(const char *key)
{
    scopes->at(cur_scope)->SetParent(key);
}


void
SymbolTable::SetInterface(const char *key)
{
    scopes->at(cur_scope)->AddInterface(key);
}

