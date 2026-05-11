#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Type.hpp"

namespace c {
namespace codegen {

class CodeGenModule;

class CodeGenTypes {
private:
    CodeGenModule& CGM;
    llvm::DenseMap<const StructType*, llvm::StructType*> Structs;
public:
    CodeGenTypes(CodeGenModule& module)
        : CGM(module) {}
    ~CodeGenTypes() = default;

    CodeGenModule& getCodeGenModule() {
        return CGM;
    }

    llvm::LLVMContext& getLLVMContext();

    llvm::Type* convertType(const Type* type);
private:
    llvm::Type* getBuiltinType(const BuiltinType* type);

    llvm::ArrayType* createArrayType(const ArrayType* type);

    llvm::StructType* createStructType(const StructType* type);
};

} // namespace codegen
} // namespace c
