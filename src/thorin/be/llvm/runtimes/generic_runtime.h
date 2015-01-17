#ifndef THORIN_BE_LLVM_RUNTIMES_GENERIC_H
#define THORIN_BE_LLVM_RUNTIMES_GENERIC_H

#include "thorin/be/llvm/runtime.h"

namespace thorin {

class GenericRuntime : public Runtime {
public:
    GenericRuntime(llvm::LLVMContext& context, llvm::Module* target, llvm::IRBuilder<>& builder);
    virtual ~GenericRuntime() {}

    virtual llvm::Value* mmap(uint32_t device, uint32_t addr_space, llvm::Value* ptr,
                             llvm::Value* mem_offset, llvm::Value* mem_size, llvm::Value* elem_size);
    virtual llvm::Value* munmap(llvm::Value* mem);
    virtual llvm::Value* parallel_create(llvm::Value* num_threads, llvm::Value* closure_ptr,
                                         uint64_t closure_size, llvm::Value* fun_ptr);
    virtual llvm::Value* parallel_join(llvm::Value* handle);
};

}

#endif

