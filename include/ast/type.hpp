#pragma once

#include "llvm/ADT/StringRef.h"

namespace c {

bool isBuiltinType(llvm::StringRef name) {
    return BuiltinType::toBuiltinKind(name).has_value();
}

class Type {
public:
    enum TypeClass : uint8_t {
        TK_Builtin,
        TK_Pointer,
    };
private:
    TypeClass Class;
protected:
    Type(TypeClass cls)
        : Class(cls) {}
public:
    inline TypeClass getTypeClass() const {
        return Class;
    }

    inline bool isBuiltin() const {
        return Class == TK_Builtin;
    }

    inline bool isPointer() const {
        return Class == TK_Pointer;
    }
};

class BuiltinType : public Type {
public:
    static constexpr TypeClass ClassID = TK_Builtin;

    enum BuiltinKind : uint8_t {
        BK_i8,
        BK_i16,
        BK_i32,
        BK_i64,

        BK_u8,
        BK_u16,
        BK_u32,
        BK_u64,

        BK_f32,
        BK_f64,

        BK_bool
    };
private:
    BuiltinKind Kind;
public:
    BuiltinType(BuiltinKind kind)
        : Type(ClassID), Kind(kind) {}

    static std::optional<BuiltinKind> toBuiltinKind(llvm::StringRef name) {
        return llvm::StringSwitch<std::optional<BuiltinKind>>(name)
            .Case("i8",  BK_i8)
            .Case("i16", BK_i16)
            .Case("i32", BK_i32)
            .Case("i64", BK_i64)
            .Case("u8",  BK_u8)
            .Case("u16", BK_u16)
            .Case("u32", BK_u32)
            .Case("u64", BK_u64)
            .Case("f32", BK_f32)
            .Case("f64", BK_f64)
            .Case("bool", BK_bool)
            .Default(std::nullopt);
    }

    inline BuiltinKind getKind() const {
        return Kind;
    }

    inline bool isIntiger() const {
        return BK_i8 <= Kind && Kind <= BK_i64;
    }

    inline bool isUnsigned() const {
        return BK_u8 <= Kind && Kind <= BK_u64;
    }

    inline bool isFloat() const {
        return BK_f32 <= Kind && Kind <= BK_f64;
    }
};

class PointerType : public Type {
public:
    static constexpr TypeClass ClassID = TK_Builtin;
private:
    const Type* Pointee;
public:
    PointerType(const Type* pointee) 
        : Type(ClassID), Pointee(pointee) {}
    
    const Type* getPointee() const {
        return Pointee;
    }
};

} // namespace c
