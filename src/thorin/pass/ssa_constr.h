#ifndef THORIN_PASS_SSA_CONSTR_H
#define THORIN_PASS_SSA_CONST_RH

#include <set>

#include "thorin/pass/pass.h"
#include "thorin/util/bitset.h"

namespace thorin {

/// SSA construction algorithm that promotes @p Slot%s, @p Load%s, and @p Store%s to SSA values.
/// This is loosely based upon:
/// "Simple and Efficient Construction of Static Single Assignment Form"
/// by Braun, Buchwald, Hack, Leißa, Mallon, Zwinkau.
class SSAConstr : public Pass<SSAConstr> {
public:
    SSAConstr(PassMan& man, size_t index)
        : Pass(man, index, "ssa_constr")
    {}

    const Def* rewrite(Def*, const Def*) override;
    undo_t analyze(Def*, const Def*) override;

    struct Visit {
        Lam* pred = nullptr;
        enum { Preds0, Preds1 } preds;
        bool got_value = false;
    };

    struct Enter {
        GIDMap<const Proxy*, const Def*> sloxy2val;
        GIDSet<const Proxy*> writable;
        uint32_t num_slots;
    };

    struct Info {
        std::set<const Proxy*, GIDLt<const Proxy*>> phis;
        Lam* phi_lam = nullptr;
    };

    using State = std::tuple<LamMap<Visit>, LamMap<Enter>>;

private:
    Lam* mem2phi(Def*);
    const Proxy* isa_sloxy(const Def*);
    const Proxy* isa_phixy(const Def*);
    const Def* get_val(Lam*, const Proxy*);
    const Def* set_val(Lam*, const Proxy*, const Def*);
#if 0
    void invalidate(Lam* phi_lam) {
        if (auto i = phi2mem_.find(phi_lam); i != phi2mem_.end()) {
            auto&& [visit, _] = get<Visit>(i->second);
            visit.phi_lam = nullptr;
            phi2mem_.erase(i);
        }
    }
#endif

    template<class T> // T = Visit or Enter
    std::pair<T&, undo_t> get(Lam* lam) { auto [i, undo, ins] = insert<LamMap<T>>(lam); return {i->second, undo}; }

    LamMap<Info> lam2info_;
    Lam2Lam phi2mem_;
    DefSet keep_;          ///< Contains Lams as well as sloxys we want to keep.
    LamSet preds_n_;       ///< Contains Lams with more than one preds.
};

}

#endif
