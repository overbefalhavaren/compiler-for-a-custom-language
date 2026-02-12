#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/ast/Type.hpp"
#include "include/lexer/Token.hpp"
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
class Expr;
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

        SC_ReturnStmt,

        SC_IfStmt,
        SC_WhileStmt,

        SC_TypeDecl,
        SC_VarDecl,
        SC_FunctionDecl,
        SC_ImplDecl,
        SC_StructDecl,

        SC_QualNameExpr,
        SC_AccessExpr,
        SC_CastExpr,
        SC_MoveExpr,
        SC_CallExpr,
        SC_NameExpr,
        SC_BinaryOperator,
        SC_UnaryOperator,

        SC_StringLiteral,
        SC_NumberLiteral,
        SC_FloatLiteral,
        SC_CharLiteral,
        SC_BoolLiteral,

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

class ControlStmtTrait {
private:
    // Value: BinaryOperator or UnaryOperator
    std::unique_ptr<Expr> Condition;

    // Value: ScopeStmt or Expr
    std::unique_ptr<Stmt> Body; 
protected:
    ControlStmtTrait(std::unique_ptr<Expr>&& condition, std::unique_ptr<Stmt>&& body)
        : Condition(std::move(condition)), Body(std::move(body)) {}
public:
    inline const std::unique_ptr<Expr>& getCondition() const {
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

class NameTrait {
private:
    llvm::StringRef Name;
public:
    NameTrait(llvm::StringRef name)
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

class DeclStmt : public Stmt, public NameTrait {
protected:
    DeclStmt(StmtClass cls, SrcSpan span, llvm::StringRef name)
        : Stmt(cls, span), NameTrait(name) {}
};

class TypeDecl : public DeclStmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_TypeDecl;
private:
    std::unique_ptr<TypeExpr> Value;
public:
    TypeDecl(SrcSpan span, llvm::StringRef name, std::unique_ptr<TypeExpr>&& type)
        : DeclStmt(ClassID, span, name), Value(std::move(type)) {}

    inline const std::unique_ptr<TypeExpr>& getType() const {
        return Value; 
    }

    inline std::unique_ptr<TypeExpr>& getType() {
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

    std::unique_ptr<TypeExpr> Ty;
    std::unique_ptr<Expr> Value;
public:
    VarDecl(SrcSpan span, llvm::StringRef name, TokenType kind, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<Expr>&& value)
        : DeclStmt(ClassID, span, name), Kind(kind), Ty(std::move(type)), Value(std::move(value)) {}

    VarDecl(SrcSpan span, llvm::StringRef name, bool pub, TokenType kind, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<Expr>&& value)
        : DeclStmt(ClassID, span, name), VisibilityTrait(pub), Kind(kind), Ty(std::move(type)), Value(std::move(value)) {}

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
        Ty != nullptr;
    }

    inline const std::unique_ptr<TypeExpr>& getType() const {
        return Ty;
    }

    inline std::unique_ptr<TypeExpr>& getType() {
        return Ty;
    }

    inline TokenType getVarKind() const {
        return Kind; 
    }

    inline const std::unique_ptr<Expr>& getValue() const { 
        return Value; 
    }

    inline std::unique_ptr<Expr>& getValue() { 
        return Value;
    }
};

class FunctionDecl : public DeclStmt, public VisibilityTrait {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_FunctionDecl;
private:
    std::unique_ptr<TypeExpr> Ty;
    llvm::SmallVector<std::unique_ptr<VarDecl>> Args;
    std::unique_ptr<BlockStmt> Body;
public:
    FunctionDecl(SrcSpan span, llvm::StringRef name, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<BlockStmt>&& body, llvm::SmallVector<std::unique_ptr<VarDecl>>&& args)
        : DeclStmt(ClassID, span, name), Ty(std::move(type)), Body(std::move(body)), Args(std::move(args)) {}

    FunctionDecl(SrcSpan span, llvm::StringRef name, bool pub, std::unique_ptr<TypeExpr>&& type, std::unique_ptr<BlockStmt>&& body, llvm::SmallVector<std::unique_ptr<VarDecl>>&& args)
        : DeclStmt(ClassID, span, name), VisibilityTrait(pub), Ty(std::move(type)), Body(std::move(body)), Args(std::move(args)) {}

    inline bool isMethod() const { 
        return getVisibility() != VK_Disabled; 
    }
    
    inline bool isAbstract() const { 
        return Body != nullptr; 
    }

    inline bool hasReturn() const {
        return Ty != nullptr;
    }

    inline bool hasArgs() const {
        return Args.size();
    }

    inline const std::unique_ptr<TypeExpr>& getType() const {
        assert(hasReturn() && "This function has no type.");
        return Ty;
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
// #                Type Expressions                #
// ##################################################

class TypeExpr : public NameTrait {
public:
    enum TypeKind {
        NamedType,
        PointerType,
        TemplateType
    };
private:
    TypeKind Kind;
    Type* Resolved;
public:
    inline bool isResolved() const {
        return Resolved != nullptr;
    }

    const Type* getResolved() const {
        assert(isResolved && "Type is not resolved yet.");
        return Resolved;
    }

    void setResolved(Type* T) {
        assert(!isResolved() && "Type is already resolved.");
        Resolved = T;
    }

    inline bool isNamedType() const {
        return Kind == NamedType;
    }

    inline bool isPointerType() const {
        return Kind == PointerType;
    }

    inline bool isTemplateType() const {
        return Kind == TemplateType;
    }
};



class TypeExpr : public Stmt, public NameTrait {
private:
    const Type* Resolved;
protected:
    TypeExpr(StmtClass cls, SrcSpan span, llvm::StringRef name) 
        : Stmt(cls, std::move(span)), NameTrait(name), Resolved(nullptr) {}
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

class ReturnStmt : public Stmt {
public:
    static constexpr StmtClass ClassID = StmtClass::SC_ReturnStmt;
private:
    std::unique_ptr<Expr> Value;
public:
    ReturnStmt(SrcSpan span, std::unique_ptr<Expr>&& value)
        : Stmt(ClassID, span), Value(std::move(value)) {}

    inline bool hasValue() const {
        return Value != nullptr;
    }

    inline const Expr& getValue() const {
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
    IfStmt(SrcSpan span, std::unique_ptr<Expr>&& condition, std::unique_ptr<Stmt>&& body, std::unique_ptr<IfStmt>&& else_)
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

    WhileStmt(SrcSpan span, std::unique_ptr<Expr>&& condition, std::unique_ptr<Stmt>&& body)
        : Stmt(ClassID, span), ControlStmtTrait(std::move(condition), std::move(body)) {}
};

} // namespace ast
} // namespace c
