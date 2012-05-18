#ifndef ANYDSL_SUPPORT_BINDING_H
#define ANYDSL_SUPPORT_BINDING_H

#include <map>

#include "anydsl/support/symbol.h"

namespace anydsl {

class Def;

struct Binding {
    Binding() {}
    Binding(const Symbol sym, const Def* def)
        : sym(sym)
        , def(def)
    {}

    bool operator < (const Binding& bind) { 
        return Symbol::FastLess()(sym, bind.sym);
    }

    std::ostream& error() const;

    const Symbol sym;
    const Def* def;
};

} // namespace anydsl

#endif // ANYDSL_SUPPORT_BINDING_H
