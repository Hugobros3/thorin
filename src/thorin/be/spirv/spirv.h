#ifndef THORIN_SPIRV_H
#define THORIN_SPIRV_H

#include "thorin/continuation.h"

namespace thorin {

class SpirVCodeGen {
public:
    SpirVCodeGen(World& world);

    void emit();
protected:
    World& world_;

    void emit(const Scope& scope);
};

}

#endif //THORIN_SPIRV_H
