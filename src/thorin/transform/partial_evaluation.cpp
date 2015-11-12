#include "thorin/primop.h"
#include "thorin/world.h"
#include "thorin/analyses/cfg.h"
#include "thorin/analyses/domtree.h"
#include "thorin/transform/mangle.h"
#include "thorin/util/hash.h"
#include "thorin/util/log.h"
#include "thorin/util/queue.h"

namespace thorin {

class PartialEvaluator {
public:
    PartialEvaluator(Scope& top_scope)
        : top_scope_(top_scope)
    {}
    ~PartialEvaluator() {
        top_scope(); // trigger update if dirty
    }

    World& world() { return top_scope_.world(); }
    void run();
    void eval(Lambda* begin, Lambda* end);
    Lambda* postdom(Lambda*, const Scope&);
    Lambda* postdom(Lambda*);
    void enqueue(Lambda* lambda) {
        if (top_scope().outer_contains(lambda)) {
            auto p = visited_.insert(lambda);
            if (p.second)
                queue_.push(lambda);
        }
    }

    void init_cur_scope(Lambda* entry) {
        cur_scope_ = new Scope(entry);
        cur_dirty_ = false;
    }

    void release_cur_scope() {
        delete cur_scope_;
    }

    const Scope& cur_scope() {
        if (cur_dirty_) {
            cur_dirty_ = false;
            return cur_scope_->update();
        }
        return *cur_scope_;
    }

    const Scope& top_scope() {
        if (top_dirty_) {
            top_dirty_ = false;
            return top_scope_.update();
        }
        return top_scope_;
    }

    void mark_dirty() { top_dirty_ = cur_dirty_ = true; }

private:
    Scope* cur_scope_;
    Scope& top_scope_;
    LambdaSet done_;
    std::queue<Lambda*> queue_;
    LambdaSet visited_;
    HashMap<Array<Def>, Lambda*> cache_;
    bool cur_dirty_;
    bool top_dirty_ = false;
};

static Lambda* continuation(Lambda* lambda) {
    return lambda->num_args() != 0 ? lambda->args().back()->isa_lambda() : (Lambda*) nullptr;
}

void PartialEvaluator::run() {
    enqueue(top_scope().entry());

    while (!queue_.empty()) {
        auto lambda = pop(queue_);

        // due to the optimization below to eat up a call, we might see a new Run here
        while (lambda->to()->isa<Run>()) {
            init_cur_scope(lambda);
            eval(lambda, continuation(lambda));
            release_cur_scope();
        }

        for (auto succ : top_scope().f_cfg().succs(lambda))
            enqueue(succ->lambda());
    }
}

void PartialEvaluator::eval(Lambda* cur, Lambda* end) {
    if (end == nullptr)
        WLOG("no matching end: % at %", cur, cur->loc());
    else
        DLOG("eval: % -> %", cur, end);

    while (true) {
        if (cur == nullptr) {
            WLOG("cur is nullptr");
            return;
        } else if (cur->empty()) {
            WLOG("empty: %", cur);
            return;
        } else if (done_.contains(cur)) {
            DLOG("already done: %", cur);
            return;
        }

        done_.insert(cur);

        Lambda* dst = nullptr;
        if (auto run = cur->to()->isa<Run>()) {
            dst = run->def()->isa_lambda();
        } else if (cur->to()->isa<Hlt>()) {
            cur = continuation(cur);
            continue;
        } else {
            dst = cur->to()->isa_lambda();
        }

        if (dst == nullptr || dst->empty()) {
            cur = postdom(cur);
            continue;
        }

        Array<Def> call(cur->size());
        call.front() = dst;
        bool all = true;
        for (size_t i = 1, e = call.size(); i != e; ++i) {
            if (!cur->op(i)->isa<Hlt>())
                call[i] = cur->op(i);
            else
                all = false;
        }

        //DLOG("dst: %", dst);
        if (auto cached = find(cache_, call)) {             // check for cached version
            jump_to_cached_call(cur, cached, call);
            DLOG("using cached call: %", cur);
            return;
        } else {                                            // no cached version found... create a new one
            auto dropped = drop(cur, call);

            if (dropped->to() == world().branch()) {
                // TODO don't stupidly inline functions
                // TODO also don't peel inside functions with incoming back-edges
            }

            mark_dirty();
            cache_[call] = dropped;
            jump_to_cached_call(cur, dropped, call);
            if (all) {
                cur->jump(dropped->to(), dropped->args()); // eat up call
                done_.erase(cur);
            } else
                cur = dropped;
        }

        if (dst == end) {
            DLOG("end: %", end);
            return;
        }
    }
}

Lambda* PartialEvaluator::postdom(Lambda* cur) {
    auto is_valid = [&] (Lambda* lambda) {
        auto p = (lambda && !lambda->empty()) ? lambda : nullptr;
        if (p)
            DLOG("postdom: % -> %", cur, p);
        return p;
    };

    if (top_scope_.entry() != cur_scope_->entry()) {
        if (auto p = is_valid(postdom(cur, cur_scope())))
            return p;
    }

    if (auto p = is_valid(postdom(cur, top_scope())))
        return p;

    WLOG("no postdom found for % at %", cur, cur->loc());
    return nullptr;
}

Lambda* PartialEvaluator::postdom(Lambda* cur, const Scope& scope) {
    const auto& postdomtree = scope.b_cfg().domtree();
    if (auto n = scope.cfa(cur))
        return postdomtree.idom(n)->lambda();
    return nullptr;
}

//------------------------------------------------------------------------------

void eval(World& world) {
    Scope::for_each(world, [&] (Scope& scope) { PartialEvaluator(scope).run(); });
}

void partial_evaluation(World& world) {
    world.cleanup();
    ILOG_SCOPE(eval(world));

    for (auto primop : world.primops()) {
        if (auto evalop = Def(primop)->isa<EvalOp>())
            evalop->replace(evalop->def());
    }
}

//------------------------------------------------------------------------------

}
