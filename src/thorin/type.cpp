#include "thorin/type.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "thorin/lambda.h"
#include "thorin/literal.h"
#include "thorin/world.h"
#include "thorin/be/thorin.h"

namespace thorin {

//------------------------------------------------------------------------------

void TypeNode::bind(TypeVar type_var) const {
    assert(!type_var->is_unified());
    type_vars_.push_back(type_var); 
    type_var->bound_at_ = this; 
}

int TypeNode::order() const {
    if (kind() == Node_PtrType)
        return 0;

    int result = 0;
    for (auto elem : elems())
        result = std::max(result, elem->order());

    if (kind() == Node_FnType)
        return result + 1;

    return result;
}

bool TypeNode::is_closed() const {
    for (auto elem : elems()) {
        if (!elem->is_closed())
            return false;
    }
    return true;
}

void TypeNode::dump() const { emit_type(Type(this)); std::cout << std::endl; }
size_t TypeNode::length() const { return as<VectorTypeNode>()->length(); }
Type TypeNode::elem_via_lit(const Def& def) const { return elem(def->primlit_value<size_t>()); }
const TypeNode* TypeNode::unify() const { return world().unify_base(this); }
TypeVarSet TypeNode::free_type_vars() const { TypeVarSet bound, free; free_type_vars(bound, free); return free; }

void TypeNode::free_type_vars(TypeVarSet& bound, TypeVarSet& free) const {
    for (auto type_var : type_vars())
        bound.insert(*type_var);

    for (auto elem : elems()) {
        if (auto type_var = elem->isa<TypeVarNode>()) {
            if (!bound.contains(type_var))
                free.insert(type_var);
        } else
            elem->free_type_vars(bound, free);
    }
}

VectorType VectorTypeNode::scalarize() const {
    if (auto ptr = isa<PtrTypeNode>())
        return world().ptr_type(ptr->referenced_type());
    return world().type(as<PrimTypeNode>()->primtype_kind());
}

bool FnTypeNode::is_returning() const {
    bool ret = false;
    for (auto elem : elems()) {
        switch (elem->order()) {
            case 0: continue;
            case 1:
                if (!ret) {
                    ret = true;
                    continue;
                } // else fall-through
            default:
                return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------

/*
 * hash
 */

size_t TypeNode::hash() const {
    size_t seed = hash_combine(hash_combine(hash_begin((int) kind()), size()), num_type_vars());
    for (auto elem : elems_)
        seed = hash_combine(seed, elem->hash());
    return seed;
}

size_t PtrTypeNode::hash() const {
    return hash_combine(hash_combine(VectorTypeNode::hash(), (size_t)device()), (size_t)addr_space());
}

//------------------------------------------------------------------------------

/*
 * equal
 */

bool TypeNode::equal(const TypeNode* other) const {
    bool result = this->kind() == other->kind() && this->size() == other->size() 
        && this->num_type_vars() == other->num_type_vars();

    if (result) {
        for (size_t i = 0, e = num_type_vars(); result && i != e; ++i) {
            assert(this->type_var(i)->equiv_ == nullptr);
            this->type_var(i)->equiv_ = *other->type_var(i);
        }

        for (size_t i = 0, e = size(); result && i != e; ++i)
            result &= this->elems_[i] == other->elems_[i];

        for (auto var : type_vars())
            var->equiv_ = nullptr;
    }

    return result;
}

bool PtrTypeNode::equal(const TypeNode* other) const {
    if(!VectorTypeNode::equal(other))
        return false;
    auto ptr = other->as<PtrTypeNode>();
    return ptr->device() == device() && ptr->addr_space() == addr_space();
}

bool TypeVarNode::equal(const TypeNode* other) const {
    if (auto typevar = other->isa<TypeVarNode>())
        return this->equiv_ == typevar;
    return false;
}

//------------------------------------------------------------------------------

/*
 * specialize and instantiate
 */

Type TypeNode::instantiate(ArrayRef<Type> types) const {
    assert(types.size() == num_type_vars());
    Type2Type map;
    for (size_t i = 0, e = types.size(); i != e; ++i)
        map[*type_var(i)] = *types[i];
    return instantiate(map);
}

Type TypeNode::instantiate(Type2Type& map) const {
#ifndef NDEBUG
    for (auto type_var : type_vars())
        assert(map.contains(*type_var));
#endif
    return vinstantiate(map);
}

Type TypeNode::specialize(Type2Type& map) const {
    if (auto result = find(map, this))
        return result;

    for (auto type_var : type_vars()) {
        assert(!map.contains(*type_var));
        map[*type_var] = *world().type_var();
    }

    auto t = instantiate(map);
    for (auto type_var : type_vars())
        t->bind(map[*type_var]->as<TypeVarNode>());

    return t;
}

Array<Type> TypeNode::specialize_elems(Type2Type& map) const {
    Array<Type> result(size());
    for (size_t i = 0, e = size(); i != e; ++i)
        result[i] = elem(i)->specialize(map);
    return result;
}

Type FrameTypeNode::vinstantiate(Type2Type& map) const { return map[this] = this; }
Type MemTypeNode::vinstantiate(Type2Type& map) const { return map[this] = this; }
Type PrimTypeNode::vinstantiate(Type2Type& map) const { return map[this] = this; }
Type TypeVarNode::vinstantiate(Type2Type& map) const { return map[this] = this; }

Type DefiniteArrayTypeNode::vinstantiate(Type2Type& map) const { 
    return map[this] = *world().definite_array_type(elem_type()->specialize(map), dim()); 
}

Type FnTypeNode::vinstantiate(Type2Type& map) const { 
    return map[this] = *world().fn_type(specialize_elems(map)); 
}

Type IndefiniteArrayTypeNode::vinstantiate(Type2Type& map) const { 
    return map[this] = *world().indefinite_array_type(elem_type()->specialize(map)); 
}

Type PtrTypeNode::vinstantiate(Type2Type& map) const { 
    return map[this] = *world().ptr_type(referenced_type()->specialize(map), length(), device(), addr_space()); 
}

Type TupleTypeNode::vinstantiate(Type2Type& map) const {
    return map[this] = *world().tuple_type(specialize_elems(map)); 
}

Type StructTypeNode::vinstantiate(Type2Type& map) const {
    return map[this] = *world().struct_type(size()); // TODO
}

}
