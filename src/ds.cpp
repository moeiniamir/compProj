


template<class Value>
void Hashtable<Value>::Enter(const char *key,
                             Value val, bool overwrite) {
    Value prev;
    if (overwrite && (prev = Lookup(key)))
        Remove(key, prev);
    mmap.insert(std::make_pair(strdup(key), val));
}


template<class Value>
void Hashtable<Value>::Remove(const char *key,
                              Value val) {
    if (mmap.count(key) == 0)
        return;

    typename std::multimap<const char *, Value>::iterator itr;
    itr = mmap.find(key);
    while (itr != mmap.upper_bound(key)) {
        if (itr->second == val) {
            mmap.erase(itr);
            break;
        }
        ++itr;
    }
}


template<class Value>
Value Hashtable<Value>::Lookup(const char *key) {
    Value found = NULL;

    if (mmap.count(key) > 0) {
        typename std::multimap<const char *, Value>::iterator cur, last, prev;
        cur = mmap.find(key);
        last = mmap.upper_bound(key);
        while (cur != last) {
            prev = cur;
            if (++cur == mmap.upper_bound(key)) {
                found = prev->second;
                break;
            }
        }
    }
    return found;
}


template<class Value>
int Hashtable<Value>::NumEntries() const {
    return mmap.size();
}


template<class Value>
Iterator <Value> Hashtable<Value>::GetIterator() {
    return Iterator<Value>(mmap);
}


template<class Value>
Value Iterator<Value>::GetNextValue() {
    return (cur == end ? NULL : (*cur++).second);
}

