

#ifndef _H_hashtable
#define _H_hashtable

#include <map>
#include <string.h>

struct ltstr {
    bool operator()(const char *s1, const char *s2) const { return strcmp(s1, s2) < 0; }
};


template<class Value>
class Iterator;

template<class Value>
class Hashtable {

private:
    std::multimap<const char *, Value, ltstr> mmap;

public:

    Hashtable() {}


    int NumEntries() const;


    void Enter(const char *key, Value value,
               bool overwriteInsteadOfShadow = true);


    void Remove(const char *key, Value value);


    Value Lookup(const char *key);


    Iterator<Value> GetIterator();

};


template<class Value>
class Iterator {
    friend class Hashtable<Value>;

private:
    typename std::multimap<const char *, Value, ltstr>::iterator cur, end;

    Iterator(std::multimap<const char *, Value, ltstr> &t)
            : cur(t.begin()), end(t.end()) {}

public:


    Value GetNextValue();
};


#include "ds.cpp"


#include <deque>
#include "globals.h"

class Node;

template<class Element>
class List {

private:
    std::deque <Element> elems;

public:

    List() {}


    int NumElements() const { return elems.size(); }


    Element Nth(int index) const {
        ;
        return elems[index];
    }


    void InsertAt(const Element &elem, int index) {
        ;
        elems.insert(elems.begin() + index, elem);
    }


    void Append(const Element &elem) { elems.push_back(elem); }


    void RemoveAt(int index) {
        ;
        elems.erase(elems.begin() + index);
    }


    void SetParentAll(Node *p) {
        for (int i = 0; i < NumElements(); i++)
            Nth(i)->SetParent(p);
    }

    void PrintAll(int indentLevel, const char *label = NULL) {
        for (int i = 0; i < NumElements(); i++)
            Nth(i)->Print(indentLevel, label);
    }


    void BuildSymTableAll() {
        for (int i = 0; i < NumElements(); i++)
            Nth(i)->BuildSymTable();
    }

    void CheckAll(checkStep c) {
        for (int i = 0; i < NumElements(); i++)
            Nth(i)->Check(c);
    }

    void EmitAll() {
        for (int i = 0; i < NumElements(); i++)
            Nth(i)->Emit();
    }
};

#endif

