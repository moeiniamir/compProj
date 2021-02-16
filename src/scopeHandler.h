

#ifndef _H_SYMTAB
#define _H_SYMTAB

#include <list>
#include <vector>
#include "ds.h"



class Decl;
class Identifier;
class Scope;

class SymbolTable {
  protected:
    std::vector<Scope *> *scopes;
    std::vector<int> *activeScopes;
    int cur_scope;
    int scope_cnt;
    int id_cnt;

    int FindScopeFromOwnerName(const char *owner);

  public:
    SymbolTable();


    void BuildScope();

    void BuildScope(const char *key);

    void EnterScope();

    Decl *Lookup(Identifier *id);

    Decl *LookupParent(Identifier *id);

    Decl *LookupInterface(Identifier *id);

    Decl *LookupField(Identifier *base, Identifier *field);

    Decl *LookupThis();

    int InsertSymbol(Decl *decl);

    bool LocalLookup(Identifier *id);

    void ExitScope();


    void SetScopeParent(const char *key);

    void SetInterface(const char *key);


    void ResetSymbolTable();
};

extern SymbolTable *scopeHandler;

#endif

