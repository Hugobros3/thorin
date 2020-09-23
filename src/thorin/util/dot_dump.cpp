#ifndef DOT_DUMP_H
#define DOT_DUMP_H

#include "thorin/world.h"

/// Outputs the raw thorin IR as a graph without performing any scope or scheduling analysis
struct DotPrinter {
    DotPrinter(World& world, const char* filename = "world.dot") : world(world) {
        file = std::ofstream(filename);
    }

private:
    void dump_def(const Def* def);
    void dump_def_generic(const Def* def, const char* color, const char* shape);
    void dump_literal(const Literal* cont);
    void dump_primop(const PrimOp* cont);
    void dump_continuation(const Continuation* cont);

public:
    void print() {
        file << "digraph " << world.name() << " {" << up;
        file << endl << "bgcolor=transparent;";
        for (Continuation* continuation : world.continuations()) {
            // Ignore those if they are not referenced elsewhere...
            if (continuation == world.branch() || continuation == world.end_scope())
                continue;
            dump_def(continuation);
        }
        file << down << endl << "}" << endl;
    }

private:
    thorin::World& world;

    DefSet done;
    std::ofstream file;
};

void DotPrinter::dump_def(const Def* def) {
    if (done.contains(def))
        return;

    if (def->isa_continuation())
        dump_continuation(def->as_continuation());
    else if (def->isa<Literal>())
        dump_literal(def->as<Literal>());
    else if (def->isa<PrimOp>())
        dump_primop(def->as<PrimOp>());
    else if (def->isa<Param>())
        dump_def_generic(def, "grey", "oval");
    else
        dump_def_generic(def, "red", "star");
}

void DotPrinter::dump_def_generic(const Def* def, const char* color, const char* shape) {
    file << endl << def->unique_name() << " [" << up;

    file << endl << "label = \"";

    file << def->unique_name() << " : " << def->type();

    file << "\";";

    file << endl << "shape = " << shape << ";";
    file << endl << "color = " << color << ";";

    file << down << endl << "]";

    done.emplace(def);

    for (size_t i = 0; i < def->num_ops(); i++) {
        const auto& op = def->op(i);
        dump_def(op);
        file << endl << def->unique_name() << " -> " << op->unique_name() << " [arrowhead=vee,label=\"o" << i << "\",fontsize=8,fontcolor=grey];";
    }
}

void DotPrinter::dump_literal(const Literal* def) {
    file << endl << def->unique_name() << " [" << up;

    file << endl << "label = \"";
    file << def;
    file << "\";";

    file << endl << "style = dotted;";

    file << down << endl << "]";

    done.emplace(def);

    assert(def->num_ops() == 0);
}

void DotPrinter::dump_primop(const PrimOp* def) {
    file << endl << def->unique_name() << " [" << up;

    file << endl << "label = \"";

    file << def->op_name();

    auto variant = def->isa<Variant>();
    auto variant_extract = def->isa<VariantExtract>();
    if (variant || variant_extract )
        file << "(" << (variant ? variant->index() : variant_extract->index()) << ")";

    file << "\";";

    file << endl << "color = darkseagreen1;";
    file << endl << "style = filled;";

    file << down << endl << "]";

    done.emplace(def);

    for (size_t i = 0; i < def->num_ops(); i++) {
        const auto& op = def->op(i);
        dump_def(op);
        file << endl << def->unique_name() << " -> " << op->unique_name() << " [arrowhead=vee,label=\"o" << i << "\",fontsize=8,fontcolor=grey];";
    }
}

void DotPrinter::dump_continuation(const Continuation* cont) {
    done.emplace(cont);
    auto intrinsic = cont->intrinsic();
    file << endl << cont->unique_name() << " [" << up;

    file << endl << "label = \"";
    if (cont->is_external())
        file << "[extern]\\n";
    auto name = cont->name();
    if (name.empty())
        name = cont->unique_name();
    file << name << "(";
    for (size_t i = 0; i < cont->num_params(); i++) {
        file << cont->param(i)->type() << (i + 1 == cont->num_params() ? "" : ", ");
    }
    file << ")";
    file << "\";";

    file << endl << "shape = rectangle;";
    if (intrinsic != Intrinsic::None) {
        file << endl << "color = lightblue;";
        file << endl << "style = filled;";
    }
    if (cont->is_external()) {
        file << endl << "color = pink;";
        file << endl << "style = filled;";
    }

    file << down << endl << "]";

    for (size_t i = 0; i < cont->num_args(); i++) {
        auto arg = cont->arg(i);
        dump_def(arg);
        file << endl << arg->unique_name() << " -> " << cont->callee()->unique_name() << " [arrowhead=onormal,label=\"a" << i << "\",fontsize=8,fontcolor=grey];";
    }

    switch (intrinsic) {
        // We don't care about the params for these, or the callee
        case Intrinsic::Match:
        case Intrinsic::Branch:
            return;
        default:
            break;
    }

    for (size_t i = 0; i < cont->num_params(); i++) {
        auto param = cont->param(i);
        dump_def(param);
        file << endl << param->unique_name() << " -> " << cont->unique_name() << " [arrowhead=none,label=\"p" << i << "\",fontsize=8,fontcolor=grey];";
    }
    dump_def(cont->callee());
    file << endl << cont->unique_name() << " -> " << cont->callee()->unique_name() << " [arrowhead=normal];";
}

#endif //DOT_DUMP_H
