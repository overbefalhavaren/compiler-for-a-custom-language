#include "include/CodeGen/CodeGenTypes.hpp"

#include "include/CodeGen/CodeGenModule.hpp"

#include "src/Debug.hpp"

namespace c {
namespace codegen {

llvm::LLVMContext& CodeGenTypes::getLLVMContext() {
    return CGM.getContext();
}

llvm::Type* CodeGenTypes::convertType(const Type* type) {
    DEBUG("Called: convertType");
    assert(type != nullptr);
    if (auto bt = llvm::dyn_cast<BuiltinType>(type))
        return getBuiltinType(bt);

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
    llvm_unreachable("Type subclass missing case.");
}

llvm::Type* CodeGenTypes::getBuiltinType(const BuiltinType* type) {
    DEBUG("Called: getBuiltinType");
    switch (type->getBuiltinKind()) {
        default:
            llvm_unreachable("BuitingKind enum value missing a case.");
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

llvm::ArrayType* CodeGenTypes::createArrayType(const ArrayType* type) {
    DEBUG("Called: createArrayType");
    llvm::Type* element_type = convertType(type->getElemType());
    return llvm::ArrayType::get(element_type, type->getInitSize());
}

llvm::StructType* CodeGenTypes::createStructType(const StructType* type) {
    DEBUG("Called: createStructType");
    llvm::StructType* st = llvm::StructType::create(getLLVMContext(), type->getDecl()->getName());
    (void)Structs.insert({type, st});

    llvm::SmallVector<llvm::Type*> fields;
    for (ast::FieldDecl* field : type->getDecl()->fields())
        fields.push_back(convertType(field->getType()));
    
    st->setBody(std::move(fields));
    return st;
}

} // namespace codegen
} // namespace c
