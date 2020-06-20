#include "thorin/pass/copy_prop.h"

#include "thorin/util.h"

namespace thorin {

Lam* CopyProp::param2prop(Def* nom) {
    auto lam = nom->isa<Lam>();
    if (lam == nullptr || lam->num_params() == 0 || !lam->is_set() || lam->is_external() || keep_.contains(lam)) return nullptr;
#if 0
    if (auto prop_lam = refine(lam)) return *prop_lam;

    auto [param_lam, prop_lam] = trace(lam);
    if (param_lam != prop_lam && prop_lam == lam) return nullptr; // we've already propagated param_lam -> prop_lam

    auto& args = args_[param_lam];
    args.resize(param_lam->num_params());
    std::vector<const Def*> types;
    for (size_t i = 0, e = args.size(); i != e; ++i) {
        if (auto arg = args[i]; arg && arg->isa<Top>())
            types.emplace_back(arg->type());
    }

    auto new_type = world().pi(world().sigma(types), param_lam->codomain());
    prop_lam = param_lam->stub(world(), new_type, param_lam->debug());
    man().mark_tainted(prop_lam);
    world().DLOG("param_lam => prop_lam: {}: {} => {}: {}", param_lam, param_lam->type(), prop_lam, prop_lam->type());
    size_t j = 0;
    Array<const Def*> new_params(args.size(), [&](size_t i) {
        if (args[i])
            return args[i]->isa<Top>() ? prop_lam->param(j++) : args[i];
        else
            return world().bot(param_lam->param(i)->type());
    });
    auto new_param = world().tuple(new_params);
    prop_lam->set(0_s, world().subst(param_lam->op(0), param_lam->param(), new_param));
    prop_lam->set(1_s, world().subst(param_lam->op(1), param_lam->param(), new_param));

    refine(param_lam, prop_lam);
    visit_undo(param_lam);

    return prop_lam;
#endif
    return nullptr;
}

const Def* CopyProp::rewrite(Def*, const Def* def) {
#if 0
    if (auto app = def->isa<App>()) {
        if (auto param_lam = app->callee()->isa_nominal<Lam>()) {
            if (auto prop_lam = param2prop(param_lam)) {
                auto& args = args_[param_lam];
                std::vector<const Def*> new_args;
                bool use_proxy = false;
                for (size_t i = 0, e = args.size(); !use_proxy && i != e; ++i) {
                    if (args[i] && args[i]->isa<Top>())
                        new_args.emplace_back(app->arg(i));
                    else
                        use_proxy |= args[i] != app->arg(i);
                }

                return use_proxy ? proxy(app->type(), app->ops()) : world().app(prop_lam, new_args);
            }
        }
    }

#endif
    return def;
}

void join(const Def*& a, const Def* b) {
    if (!a) {
        a = b;
    } else if (a == b) {
    } else {
        a = a->world().top(a->type());
    }
}

undo_t CopyProp::analyze(Def* cur_nom, const Def* def) {
    return No_Undo;
#if 0
    auto cur_lam = cur_nom->isa<Lam>();
    if (!cur_lam || def->isa<Param>()) return No_Undo;
    if (auto proxy = def->isa<Proxy>(); proxy && proxy->index() != index()) return No_Undo;

    if (auto proxy = isa_proxy(def)) {
        auto param_lam = proxy->op(0)->as_nominal<Lam>();
        auto proxy_args = proxy->op(1)->outs();
        auto& args = args_[param_lam];
        for (size_t i = 0, e = proxy_args.size(); i != e; ++i) {
            auto x = args[i];
            auto xx = x ? x->unique_name() : std::string("<null>");
            join(args[i], proxy_args[i]);
            world().DLOG("{} = {} join {}", args[i], xx, proxy_args[i]);
        }

        //invalidate(*refine(param_lam));
        return visit_undo(param_lam);
    }

    auto result = No_Undo;
    for (size_t i = 0, e = def->num_ops(); i != e; ++i) {
        auto op = def->op(i);
        if (auto lam = op->isa_nominal<Lam>()) {
            //auto [param_lam, prop_lam] = trace(lam);
            // if lam does not occur as callee - we can't do anything
            if ((!def->isa<App>() || i != 0)) {
                if (param_lam != prop_lam && keep_.emplace(lam).second) {
                    world().DLOG("keep: {}", lam);
                    result = std::min(result, visit_undo(param_lam));
                    invalidate(prop_lam);
                }
            }
        }
    }

    return result;
#endif
}

}
