#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/ast/type.hpp"
#include "include/lexer/token.hpp"
#include "include/io/source_location.hpp"

namespace c {
namespace ast {

// Forward declaration because MSVC is retarded
class Stmt;
class BlockStmt;
class DeclStmt;
class TypeDecl;
class VarDecl;
class FunctionDecl;
class ImplDecl;
class StructDecl;
class ExprStmt;
class CallExpr;
class TypeExpr;
class VarRef;
class ImplRef;
class BinaryOperator;
class UnaryOperator;
class IntLiteral;
class FloatLiteral;
class StringLiteral;
class CastStmt;
class IfStmt;
class WhileStmt;

class Stmt {
public:
    enum StmtClass {
        SC_BlockStmt,

        SC_CastStmt,

        SC_ReturnStmt,

        SC_IfStmt,
        SC_WhileStmt,

        SC_TypeDecl,
        SC_VarDecl,
        SC_FunctionDecl,
        SC_ImplDecl,
        SC_StructDecl,

        SC_CallExpr,
        SC_MoveExpr,

        SC_NameRef,
        SC_TypeExpr,
        SC_VarRef,
        SC_ImplRef,

        SC_BinaryOperator,
        SC_UnaryOperator,

        SC_IntLiteral,
        SC_FloatLiteral,
        SC_StringLiteral,

        SC_NamedTypeExpr,
        SC_PointerTypeExpr,
        SC_TemplateTypeExpr,
    };
private:
    SrcSpan Span;
    StmtClass Class;
protected:
    Stmt(StmtClass cls, SrcSpan span) 
        : Class(cls), Span(span) {}
public:
    Stmt() = delete;
    Stmt(Stmt&&) = delete;
    Stmt(const Stmt&) = delete;
    virtual ~Stmt() = default;

    template <typename T>
    inline bool isa() const {
        static_assert(std::is_base_of<Stmt, T>::value, "T must be a subclass of Stmt.");
        return Class == T::ClassID;
    }

    inline bool isa(StmtClass cls) const {
        return Class == cls;
    }

    inline bool isa(const Stmt& other) const {
        return Class == other.getStmtClass();
    }

    inline StmtClass getStmtClass() const {
        return Class;
    }

    inline const SrcSpan& getSpan() const {
        return Span; 
    }

    inline SrcLoc getStartLoc() const {
        return Span.getStartLoc(); 
    }

    inline SrcLoc getEndLoc() const {
        return Span.getEndLoc();
    }
};

template <typename T> constexpr void _do_isa_assert() {
    static_assert(std::is_base_of<Stmt, T>::value, "T must be a subclass of Stmt.");
}

template <typename T> inline bool isa(const Stmt* s) {
    _do_isa_assert<T>();
    return s->getStmtClass() == T::ClassID;
}

template <typename T> inline bool isa(const Stmt& s) {
    _do_isa_assert<T>();
    return s.getStmtClass() == T::ClassID;
}

template <typename T> inline bool isa(const std::unique_ptr<Stmt>& s) {
    _do_isa_assert<T>();
    return s->getStmtClass() == T::ClassID;
}

template <typename T> constexpr void _do_cast_assert() {
    static_assert(std::is_base_of<Stmt, T>::value, "T must be a subclass of Stmt.");
}

template <typename T> inline T* cast(Stmt* s) { 
    _do_cast_assert<T>();
    return static_cast<T*>(s);
}

template <typename T> inline const T* cast(const Stmt* s) {
    _do_cast_assert<T>();
    return static_cast<const T*>(s);
}

template <typename T> inline T& cast(Stmt& s) { 
    _do_cast_assert<T>();
    return static_cast<T&>(s);
}

template <typename T> inline const T& cast(const Stmt& s) { 
    _do_cast_assert<T>();
    return static_cast<const T&>(s);
}

template <typename T> inline std::unique_ptr<T>& cast(std::unique_ptr<Stmt>& s) { 
    _do_cast_assert<T>(); 
    return static_cast<std::unique_ptr<T>&>(s);
}

template <typename T> inline std::unique_ptr<T>& cast(const std::unique_ptr<Stmt>& s) {
    _do_cast_assert<T>(); 
    return static_cast<const std::unique_ptr<T>&>(s);
}

template <typename T>
inline T* dyn_cast(Stmt* s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

template <typename T>
inline const T* dyn_cast(const Stmt* s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

template <typename T>
inline T& dyn_cast(Stmt& s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

template <typename T>
inline const T& dyn_cast(const Stmt& s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

template <typename T>
inline std::unique_ptr<T>& dyn_cast(std::unique_ptr<Stmt>& s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

template <typename T>
inline const std::unique_ptr<T>& dyn_cast(const std::unique_ptr<Stmt>& s) {
    return isa<T>(s) ? static_cast<T*>(s) : nullptr;
}

class ControlStmtTrait {
private:
    // Value: BinaryOperator or UnaryOperator
    std::unique_ptr<ExprStmt> Condition;

    // Value: ScopeStmt or ExprStmt
    std::unique_ptr<Stmt> Body; 
protected:
    ControlStmtTrait(std::unique_ptr<ExprStmt>&& condition, std::unique_ptr<Stmt>&& body)
        : Condition(std::move(condition)), Body(std::move(body)) {}
public:
    inline const std::unique_ptr<ExprStmt>& getCondition() const {
        // FIXME: For some reason asserts as false even though a condition exists
        // assert(Condition && "This statement has no condition.");
        return Condition; 
    }

    inline const std::unique_ptr<Stmt>& getBody() const { 
        assert(Body && "This statement has no body");
        return Body; 
    }

    inline SrcSpan getBodySpan() const {
        assert(Body && "This statement has no body");
        return Body->getSpan(); 
    }
};

class NamedStmtTrait {
private:
    llvm::StringRef Name;
public:
    NamedStmtTrait(llvm::StringRef name)
        : Name(name) {}
    
    inline llvm::StringRef getName() const {
        return Name;
    }
};

// TODO: Maybe add a method to check if visibility is disabled. 
// Don't know what to call it right now...
class VisibilityTrait {
protected:
    enum VisibilityKind : int8_t {
        VK_Disabled = -1,
        VK_Private = 0,
        VK_Public = 1
    };
private:
    VisibilityKind Visibility;
public:
    VisibilityTrait() : Visibility(VK_Disabled) {}
    VisibilityTrait(bool v) : VisibilityTrait((int8_t)v) {}
    VisibilityTrait(VisibilityKind v) : Visibility(v) {}
    VisibilityTrait(int8_t v) : Visibility(static_cast<VisibilityKind>(v)) {
        assert(-1 <= v && v <= 1 && "Visibility can only be '-1', '0' or '1'");
    }

    inline bool isPublic() const {
        return Visibility == VK_Public;
    }

    inline bool isPrivate() const {
        return Visibility == VK_Private;
    }

    inline VisibilityKind getVisibility() const {
        return Visibility;
    }
};

// ##################################################
// #                Scope Statement                 #
// ##################################################

class BlockStmt : public Stmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_BlockStmt;
private:
    llvm::SmallVector<std::unique_ptr<Stmt>> Stmts;
public:
    BlockStmt(SrcSpan span, llvm::SmallVector<std::unique_ptr<Stmt>>&& stmts)
        : Stmt(ClassID, span), Stmts(std::move(stmts)) {}

    inline size_t getSize() const {
        return Stmts.size();
    }

    inline const llvm::SmallVector<std::unique_ptr<Stmt>>& getStmts() const {
        return Stmts;
    }
};

// ##################################################
// #            Declaration Statements              #
// ##################################################

class DeclStmt : public Stmt, public NamedStmtTrait {
protected:
    DeclStmt(StmtClass cls, SrcSpan span, llvm::StringRef name)
        : Stmt(cls, span), NamedStmtTrait(name) {}
};

class TypeDecl : public DeclStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_TypeDecl;
private:
    std::unique_ptr<TypeExpr> Value;
public:
    TypeDecl(SrcSpan span, llvm::StringRef name, std::unique_ptr<TypeExpr>&& type)
        : DeclStmt(ClassID, span, name), Value(std::move(type)) {}

    inline const std::unique_ptr<TypeExpr>& getTypeExpr() const {
        return Value; 
    }

    inline std::unique_ptr<TypeExpr>& getTypeExpr() {
        return Value;
    }

    inline bool isResolved() const {
        return Value->isResolved();
    }

    inline const Type* getResolved() const {
        return Value->getResolved();
    }
};

class VarDecl : public DeclStmt, public VisibilityTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_VarDecl;
private:
    // Values:
    // Const: 
    //      Compile time constant
    //      Context: Declaration
    // Let: 
    //      Immutable variable declaration or immutable reference
    //      Context: Declaration, attribute or argument
    // Mut: 
    //      Mutable variable declaration or mutable reference
    //      Context: Declaration, attribute or argument
    // Move:
    //      Takes ownership of the value passed into a function argument.
    //      Context: Argument
    TokenType Kind;

    std::unique_ptr<TypeExpr> TypePtr;
    std::unique_ptr<ExprStmt> Value;
public:
    VarDecl(SrcSpan span, llvm::StringRef name, TokenType kind, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<ExprStmt>&& value)
        : DeclStmt(ClassID, span, name), Kind(kind), TypePtr(std::move(type)), Value(std::move(value)) {}

    VarDecl(SrcSpan span, llvm::StringRef name, bool pub, TokenType kind, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<ExprStmt>&& value)
        : DeclStmt(ClassID, span, name), VisibilityTrait(pub), Kind(kind), TypePtr(std::move(type)), Value(std::move(value)) {}

    inline bool isAttribute() const { 
        return getVisibility() != VK_Disabled;
    }

    inline bool isDefined() const { 
        return Value != nullptr; 
    }

    inline bool isMutable() const {
        return Kind == TokenType::Mut;
    }

    inline bool isAutoTyped() const {
        TypePtr != nullptr;
    }

    inline const std::unique_ptr<TypeExpr>& getTypeExpr() const {
        return TypePtr;
    }

    inline std::unique_ptr<ast::TypeExpr>& getTypeExpr() {
        return TypePtr;
    }
    
    inline TokenType getVarKind() const {
        return Kind; 
    }

    inline const std::unique_ptr<ExprStmt>& getValue() const { 
        return Value; 
    }
};

class FunctionDecl : public DeclStmt, public VisibilityTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_FunctionDecl;
private:
    std::unique_ptr<TypeExpr> TypePtr;
    llvm::SmallVector<std::unique_ptr<VarDecl>> Args;
    std::unique_ptr<BlockStmt> Body;
public:
    FunctionDecl(SrcSpan span, llvm::StringRef name, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<BlockStmt>&& body, llvm::SmallVector<std::unique_ptr<VarDecl>>&& args)
        : DeclStmt(ClassID, span, name), TypePtr(std::move(type)), Body(std::move(body)), Args(std::move(args)) {}

    FunctionDecl(SrcSpan span, llvm::StringRef name, bool pub, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<BlockStmt>&& body, llvm::SmallVector<std::unique_ptr<VarDecl>>&& args)
        : DeclStmt(ClassID, span, name), VisibilityTrait(pub), TypePtr(std::move(type)), Body(std::move(body)), Args(std::move(args)) {}

    inline bool isMethod() const { 
        return getVisibility() != VK_Disabled; 
    }
    
    inline bool isAbstract() const { 
        return Body != nullptr; 
    }

    inline bool hasReturn() const {
        return TypePtr != nullptr;
    }

    inline bool hasArgs() const {
        return Args.size();
    }

    inline const std::unique_ptr<TypeExpr>& getTypeExpr() const {
        assert(hasReturn() && "This function has no type.");
        return TypePtr;
    }

    inline const std::unique_ptr<BlockStmt>& getBody() const { 
        return Body; 
    }

    inline const llvm::SmallVector<std::unique_ptr<VarDecl>>& getArgs() const { 
        return Args; 
    }
};

class ImplDecl : public DeclStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_ImplDecl;
private:
    llvm::SmallVector<std::unique_ptr<FunctionDecl>> Methods;
public:
    ImplDecl(SrcSpan span, llvm::StringRef name, llvm::SmallVector<std::unique_ptr<FunctionDecl>>&& methods)
        : DeclStmt(ClassID, span, name), Methods(std::move(methods)) {}

    inline const llvm::SmallVector<std::unique_ptr<FunctionDecl>>& getMethods() const { 
        return Methods; 
    }

    bool hasMethod(llvm::StringRef name) {
        for (const std::unique_ptr<FunctionDecl>& m : Methods)
            if (m->getName() == name)
                return true;
        return false;
    }

    bool hasAllMethods(llvm::SmallVector<llvm::StringRef> names) const {
        for (llvm::StringRef n : names) 
            for (const std::unique_ptr<FunctionDecl>& m : Methods)
                if (m->getName() == n)
                    return true;
        return false;
    }
};

class StructDecl : public DeclStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_StructDecl;
private:
    std::unique_ptr<ImplDecl> Impl;
    llvm::SmallVector<std::unique_ptr<ImplRef>> Inherited;
    llvm::SmallVector<std::unique_ptr<VarDecl>> Attrs;
public:
    StructDecl(SrcSpan span, llvm::StringRef name, llvm::SmallVector<std::unique_ptr<VarDecl>>&& attrs)
        : DeclStmt(ClassID, span, name), Attrs(std::move(attrs)) {}

    StructDecl(SrcSpan span, llvm::StringRef name, llvm::SmallVector<std::unique_ptr<VarDecl>>&& attrs, std::unique_ptr<ImplDecl>&& impl, llvm::SmallVector<std::unique_ptr<ImplRef>>&& derived)
        : DeclStmt(ClassID, span, name) , Attrs(std::move(attrs)), Impl(std::move(impl)), Inherited(std::move(derived)) {}

    inline bool hasImpl() const { 
        return Impl != nullptr; 
    }

    inline bool hasDerived() const {
        return Inherited.size();
    }

    inline const std::unique_ptr<ImplDecl>& getImpl() const { 
        assert(hasImpl() && "No impl exists for this struct.");
        return Impl; 
    }
    
    inline const llvm::SmallVector<std::unique_ptr<VarDecl>>& getAttrs() const { 
        return Attrs; 
    }

    inline llvm::SmallVector<std::unique_ptr<VarDecl>>& getAttrs() { 
        return Attrs; 
    }

    inline const llvm::SmallVector<std::unique_ptr<ImplRef>>& getDerived() const { 
        assert(hasDerived() && "This struct has no inherited traits.");
        return Inherited; 
    }
};

// ##################################################
// #            Expression Statements               #
// ##################################################

class ExprStmt : public Stmt {
protected:
    ExprStmt(StmtClass cls, SrcSpan span)
        : Stmt(cls, std::move(span)) {}
};

// class QualNameExpr : public ExprStmt, public NamedStmtTrait {
// private:
//     std::unique_ptr<>
// };

class CallExpr : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_CallExpr;
private:
    std::string Callee;

    // Positional Arguments
    // Values: CallExpr, VarRef, IntLiteral, FloatLiteral, StringLiteral
    llvm::SmallVector<std::unique_ptr<ExprStmt>> Args;

    // Keyword Arguments
    // Values: CallExpr, VarRef, IntLiteral, FloatLiteral, StringLiteral
    llvm::StringMap<std::unique_ptr<ExprStmt>> KWArgs;
public:
    CallExpr(SrcSpan span, llvm::StringRef callee)
        : ExprStmt(ClassID, span), Callee(callee) {}
    CallExpr(SrcSpan span, llvm::StringRef callee, llvm::SmallVector<std::unique_ptr<ExprStmt>>&& args, llvm::StringMap<std::unique_ptr<ExprStmt>>&& kwargs)
        : ExprStmt(ClassID, span), Callee(callee), Args(std::move(args)), KWArgs(std::move(kwargs)) {}
    
    inline llvm::StringRef getCalleeName() const { 
        return Callee;
    }

    inline bool hasParams() const {
        return Args.size() || KWArgs.size();
    }

    inline const llvm::SmallVector<std::unique_ptr<ExprStmt>>& getArgs() const {
        return Args; 
    }

    inline const llvm::StringMap<std::unique_ptr<ExprStmt>>& getKWArgs() const {
        return KWArgs; 
    }
};

class MoveExpr : public ExprStmt {
    static constexpr StmtClass ClassID = StmtClass::SC_MoveExpr;
private:
    std::unique_ptr<ExprStmt> Value;
public:
    MoveExpr(SrcSpan span, std::unique_ptr<ExprStmt>&& value)
        : ExprStmt(ClassID, span), Value(std::move(value)) {}

    const ExprStmt& getValue() const {
        return *Value;
    }
};

class NameRef : public ExprStmt, public NamedStmtTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_NameRef;
    
    NameRef(SrcSpan span, llvm::StringRef name) 
        : ExprStmt(ClassID, span), NamedStmtTrait(name) {}
};

class VarRef : public ExprStmt, public NamedStmtTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_VarRef;
public:
    VarRef(SrcSpan span, llvm::StringRef name)
        : ExprStmt(ClassID, span), NamedStmtTrait(name) {}
};

class ImplRef : public ExprStmt, public NamedStmtTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_ImplRef;

    ImplRef(SrcSpan span, llvm::StringRef name)
        : ExprStmt(ClassID, span), NamedStmtTrait(name) {}
};

class BinaryOperator : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_BinaryOperator;
private:
    TokenType Op;
    std::unique_ptr<ExprStmt> LHS;
    std::unique_ptr<ExprStmt> RHS;
public:
    BinaryOperator(SrcSpan span, TokenType op, std::unique_ptr<ExprStmt>&& lhs, std::unique_ptr<ExprStmt>&& rhs)
        : ExprStmt(ClassID, span), Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {
            assert(LHS && "'lhs' can't be nullptr.");
            assert(RHS && "'rhs' can't be nullptr.");
        }

    inline TokenType getOperator() const { 
        return Op; 
    }

    inline const std::unique_ptr<ExprStmt>& getLHS() const { 
        return LHS; 
    }

    inline const std::unique_ptr<ExprStmt>& getRHS() const { 
        return RHS; 
    }
};

class UnaryOperator : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_UnaryOperator;
private:
    TokenType Op;
    std::unique_ptr<ExprStmt> Expr;
public:
    UnaryOperator(SrcSpan span, TokenType op, std::unique_ptr<ExprStmt>&& expr)
        : ExprStmt(ClassID, span), Op(op), Expr(std::move(expr)) {
            assert(Expr && "'expr' can't be nullptr.");
        }

    inline TokenType getOperator() const { 
        return Op; 
    }

    inline const std::unique_ptr<ExprStmt>& getExpr() const { 
        return Expr; 
    }
};

class IntLiteral : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_IntLiteral;
private:
    size_t Value;
public:
    IntLiteral(SrcSpan span, size_t value)
        : ExprStmt(ClassID, span), Value(value) {}
    
    inline size_t getValue() const { 
        return Value; 
    }
};

class FloatLiteral : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_FloatLiteral;
private:
    double Value;
public:
    FloatLiteral(SrcSpan span, double value)
        : ExprStmt(ClassID, span), Value(value) {}
    
    inline double getValue() const { 
        return Value; 
    }
};

class StringLiteral : public ExprStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_StringLiteral;
private:
    std::string Value;
public:
    StringLiteral(SrcSpan span, llvm::StringRef value)
        : ExprStmt(ClassID, span), Value(value) {}
    
    inline llvm::StringRef getValue() const { 
        return Value; 
    }
};

// ##################################################
// #                Type Expressions                #
// ##################################################

class TypeExpr : public Stmt, public NamedStmtTrait {
private:
    const Type* Resolved;
protected:
    TypeExpr(StmtClass cls, SrcSpan span, llvm::StringRef name) 
        : Stmt(cls, std::move(span)), NamedStmtTrait(name), Resolved(nullptr) {}
public:
    inline bool isResolved() const {
        return Resolved != nullptr;
    }
    
    const Type* getResolved() const {
        assert(isResolved && "Type is not resolved yet.");
        return Resolved;
    }

    void setResolved(const Type* T) {
        assert(!isResolved() && "Type is already resolved.");
        Resolved = T;
    }
};

class NamedTypeExpr : public TypeExpr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_NamedTypeExpr;

    NamedTypeExpr(SrcSpan span, llvm::StringRef name)
        : TypeExpr(ClassID, span, name) {}
};

class PointerTypeExpr : public TypeExpr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_NamedTypeExpr;
private:
    std::unique_ptr<TypeExpr> Pointee;
public:
    PointerTypeExpr(SrcSpan span, std::unique_ptr<TypeExpr>&& pointee)
        : TypeExpr(ClassID, span, pointee->getName()), Pointee(std::move(pointee)) {}

    const TypeExpr& getPointee() const {
        return *Pointee;
    }
};

class TemplateTypeExpr : public TypeExpr {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_NamedTypeExpr;
private:
    llvm::SmallVector<std::unique_ptr<TypeExpr>> Types;
public:
    TemplateTypeExpr(SrcSpan span, llvm::StringRef name, llvm::SmallVector<std::unique_ptr<TypeExpr>>&& types)
        : TypeExpr(ClassID, span, name), Types(std::move(types)) {}

    const llvm::SmallVector<std::unique_ptr<TypeExpr>>& getTypes() const {
        return Types;
    }
};

// ##################################################
// #                Other Statements                #
// ##################################################

class CastStmt : public Stmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_CastStmt;
private:
    std::unique_ptr<ExprStmt> Value;
    std::unique_ptr<TypeExpr> TypePtr;
public:
    CastStmt(SrcSpan span, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<ExprStmt>&& value)
        : Stmt(ClassID, span), TypePtr(std::move(type)), Value(std::move(value)) {}
    
    inline bool isVoidCast() const {
        return TypePtr != nullptr;
    }

    inline const std::unique_ptr<ExprStmt>& getValue() const {
        return Value;
    }

    inline const std::unique_ptr<TypeExpr>& getTypeExpr() const {
        assert(!isVoidCast() && "Can't get the type for a void cast.");
        return TypePtr;
    }
};

class ReturnStmt : public Stmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_ReturnStmt;
private:
    std::unique_ptr<ExprStmt> Value;
public:
    ReturnStmt(SrcSpan span, std::unique_ptr<ExprStmt>&& value)
        : Stmt(ClassID, span), Value(std::move(value)) {}

    inline bool hasValue() const {
        return Value != nullptr;
    }

    inline const ExprStmt& getValue() const {
        assert(hasValue() && "Can't get the value since none exists.");
        return *Value;
    }
};

class IfStmt : public Stmt, public ControlStmtTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_IfStmt;
private:        
    std::unique_ptr<IfStmt> Else;
public:
    IfStmt(SrcSpan span, std::unique_ptr<ExprStmt>&& condition, std::unique_ptr<Stmt>&& body, std::unique_ptr<IfStmt>&& else_)
        : Stmt(ClassID, span), ControlStmtTrait(std::move(condition), std::move(body)), Else(std::move(else_)) {}

    inline bool isElse() const { 
        return getCondition() == nullptr; 
    }

    inline bool hasElse() const { 
        return Else != nullptr; 
    }

    inline const std::unique_ptr<IfStmt>& getElse() const {
        assert(hasElse() && "No else statement exists for this if statement.");
        return Else;
    }
};

class WhileStmt : public Stmt, public ControlStmtTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_WhileStmt;

    WhileStmt(SrcSpan span, std::unique_ptr<ExprStmt>&& condition, std::unique_ptr<Stmt>&& body)
        : Stmt(ClassID, span), ControlStmtTrait(std::move(condition), std::move(body)) {}
};

} // namespace ast
} // namespace c
