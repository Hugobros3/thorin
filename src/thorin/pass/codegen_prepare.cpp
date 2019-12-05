#include "thorin/pass/codegen_prepare.h"

namespace thorin {

bool CodegenPrepare::scope(Def* nom) {
    world().DLOG("!!!");
    if (auto entry = nom->isa<Lam>()) {
        // new wrapper that calls the return continuation
        old_param_ = entry->param();
        auto ret_param = entry->ret_param();
        auto ret_cont = world().lam(ret_param->type()->as<Pi>(), ret_param->debug());
        ret_cont->app(ret_param, ret_cont->param(), ret_param->debug());

        // rebuild a new "param" that substitutes the actual ret_param with ret_cont
        auto ops = entry->param()->split();
        assert(ops.back() == ret_param && "we assume that the last element is the ret_param");
        ops.back() = ret_cont;
        new_param_ = world().tuple(ops);
        world().DLOG("old_param_: {}/{}", old_param_->gid(), old_param_);
        world().DLOG("new_param_: {}/{}", new_param_->gid(), new_param_);
        return true;
    }
    return false;
}

const Def* CodegenPrepare::rewrite(const Def* def) {
    world().DLOG("{}/{}", def->gid(), def);
    if (def == old_param_) return new_param_;
    return def;
}

}
