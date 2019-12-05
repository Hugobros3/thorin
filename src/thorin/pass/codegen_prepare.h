#ifndef THORIN_PASS_CODEGEN_PREPARE_H
#define THORIN_PASS_CODEGEN_PREPARE_H

#include "thorin/pass/pass.h"

namespace thorin {

class CodegenPrepare : public Pass {
public:
    CodegenPrepare(PassMan& man, size_t index)
        : Pass(man, index, "codegen_prepare")
    {}

    bool scope(Def* def) override;
    const Def* rewrite(const Def* def) override;

private:
    const Param* old_param_ = nullptr;
    const Def* new_param_ = nullptr;
};

}

#endif
