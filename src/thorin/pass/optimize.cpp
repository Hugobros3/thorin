#include "thorin/pass/codegen_prepare.h"
#include "thorin/pass/copy_prop.h"
#include "thorin/pass/inliner.h"
#include "thorin/pass/mem2reg.h"
#include "thorin/pass/partial_eval.h"

#include "thorin/transform/compile_ptrns.h"

// old stuff
#include "thorin/transform/cleanup_world.h"
#include "thorin/transform/flatten_tuples.h"
#include "thorin/transform/partial_evaluation.h"

namespace thorin {

void optimize(World& world) {
    PassMan(world)
    //.create<PartialEval>()
    //.create<Inliner>()
    .create<Mem2Reg>()
    //.create<CopyProp>()
    .run();

    // TODO remove old stuff
    while (partial_evaluation(world, true)); // lower2cff
    flatten_tuples(world);
    cleanup_world(world);

    PassMan(world)
    .create<CodegenPrepare>()
    .run();
}

}
