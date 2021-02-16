/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "globals.h"

// the global code generator class.
CodeGenerator *CG = new CodeGenerator();

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

/* The Print method is used to print the parse tree nodes.
 * If this node has a location (most nodes do, but some do not), it
 * will first print the line number to help you match the parse tree
 * back to the source text. It then indents the proper number of levels
 * and prints the "print name" of the node. It then will invoke the
 * virtual function PrintChildren which is expected to print the
 * internals of the node (itself & children) as appropriate.
 */


Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
}

void Identifier::CheckDecl() {
    Decl *d = symtab->Lookup(this);
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
    char *s = (char *)malloc(strlen(name) + strlen(prefix) + 1);
    sprintf(s, "%s%s", prefix, name);
    name = s;
}

