/* File: symtab.h
 * --------------
 * Build the symbol table.
 * Author: Deyuan Guo
 */

#ifndef _H_SYMTAB
#define _H_SYMTAB

#include <list>
#include <vector>
#include "ds.h"

/* Symbol Table Implementation. */

class Decl;
class Identifier;
class Scope;

class SymbolTable {
  protected:
    std::vector<Scope *> *scopes;
    std::vector<int> *activeScopes;
    int cur_scope;  /* current scope */
    int scope_cnt;  /* scope counter */
    int id_cnt;

    int FindScopeFromOwnerName(const char *owner);

  public:
    SymbolTable();

    /* Enter a new scope. */
    void BuildScope();
    /* Enter a new scope, and set owner for class and interface. */
    void BuildScope(const char *key);
    /* Enter a new scope without build new hashtable. */
    void EnterScope();
    /* Look up symbol in all active scopes. */
    Decl *Lookup(Identifier *id);
    /* Look up symbol in parent scopes. */
    Decl *LookupParent(Identifier *id);
    /* Look up symbol in interface scopes. */
    Decl *LookupInterface(Identifier *id);
    /* Look up symbol in a given class/interface name. */
    Decl *LookupField(Identifier *base, Identifier *field);
    /* Look up the class decl for This. */
    Decl *LookupThis();
    /* Insert new symbol into current scope. */
    int InsertSymbol(Decl *decl);
    /* Look up symbol in current scope. */
    bool LocalLookup(Identifier *id);
    /* Exit current scope and return to its parent scope. */
    void ExitScope();

    /* Deal with class inheritance, set parent for a subclass. */
    void SetScopeParent(const char *key);
    /* Deal with class interface, set interfaces for a subclass. */
    void SetInterface(const char *key);

    /* Resert symbol table counter and active scopes for another pass. */
    void ResetSymbolTable();

    /* Print the whole symbol table. */
    void Print();
};

extern SymbolTable *symtab;

#endif

