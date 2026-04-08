#pragma once

#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Type.hpp"
#include "include/CodeGen/CodeGenModule.hpp"

namespace c {
namespace codegen {

class CodeGenTypes {
private:
    CodeGenModule& CGModule;
    llvm::DenseMap<const StructType*, llvm::StructType*> Structs;
public:
    CodeGenTypes(CodeGenModule& module)
        : CGModule(module) {}
    ~CodeGenTypes() = default;

    inline CodeGenModule& getCodeGenModule() {
        return CGModule;
    }

    inline llvm::LLVMContext& getLLVMContext() {
        return CGModule.getLLVMContext();
    }

    llvm::Type* convertType(const Type* type) {
        if (auto bt = llvm::dyn_cast<BuiltinType>(type))
            getBuiltinType(bt);

        if (auto pt = llvm::dyn_cast<PointerType>(type))
            return llvm::PointerType::get(
                convertType(pt->getPointee()), 0
            );

        if (auto at = llvm::dyn_cast<ArrayType>(type))
            return createArrayType(at);
        
        if (auto st = llvm::dyn_cast<StructType>(type)) {
            llvm::StructType* result = Structs.lookup(st);
            if (result == nullptr)
                result = createStructType(st);
            return result;
        }
        
        // This point should never be reached.
        assert(false);
    }
private:
    const llvm::Type* getBuiltinType(const BuiltinType* type) {
        switch (type->getKind()) {
            case BuiltinType::Bool:
                return llvm::Type::getInt1Ty(getLLVMContext());
            
            case BuiltinType::I8:
            case BuiltinType::U8:
                return llvm::Type::getInt8Ty(getLLVMContext());
            case BuiltinType::I16:
            case BuiltinType::U16:
                return llvm::Type::getInt16Ty(getLLVMContext());
            case BuiltinType::I32:
            case BuiltinType::U32:
                return llvm::Type::getInt32Ty(getLLVMContext());
            case BuiltinType::I64:
            case BuiltinType::U64:
                return llvm::Type::getInt64Ty(getLLVMContext());
            
            case BuiltinType::F32:
                return llvm::Type::getFloatTy(getLLVMContext());
            case BuiltinType::F64:
                return llvm::Type::getDoubleTy(getLLVMContext());
        }
    }

    llvm::ArrayType* createArrayType(const ArrayType* type) {
        llvm::Type* element_type = convertType(type->getElemType());
        return llvm::ArrayType::get(element_type, type->getInitSize());
    }

    llvm::StructType* createStructType(const StructType* type) {
        llvm::StructType* st = llvm::StructType::create(getLLVMContext(), type->getDecl()->getName());
        (void)Structs.insert({type, st});

        llvm::SmallVector<llvm::Type*> fields;
        for (ast::FieldDecl* field : type->getDecl()->fields())
            fields.push_back(convertType(field->getType()));
        
        st->setBody(std::move(fields));
        return st;
    }
};

} // namespace codegen
} // namespace c
