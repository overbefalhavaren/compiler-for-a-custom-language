#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"

#include "include/lexer/token.hpp"

namespace c {
namespace ast {

std::unique_ptr<llvm::LLVMContext> context;
std::unique_ptr<llvm::Module> module_;
std::unique_ptr<llvm::IRBuilder<>> builder;
llvm::StringMap<llvm::Value*> namedValues;

class Expr {
public:
    using ExprPtr = std::unique_ptr<Expr>;
    // enum ExprType {
    //     Block,
    //     Variable,
    //     VariableRef,
    //     BinaryOp,
    //     IntLiteral,
    //     FloatLiteral,
    //     StringLiteral,
    //     CharLiteral,
    //     Function,
    // };

    virtual ~Expr() = default;
    virtual llvm::Value* codegen() = 0;

    virtual constexpr std::string str() const = 0;
// protected:
//     ExprType kind;
};

/*
bool isVariable(TokenType type) {
    switch (type) {
        case TokenType::Const:
        case TokenType::Let:
        case TokenType::Mut:
        case TokenType::Temp:
        case TokenType::Move:
            return true;

        default:
            return false;
    }
}

bool isLiteral(TokenType type) {
    switch (type) {
        case TokenType::Int:
        case TokenType::Float:
        case TokenType::String:
        case TokenType::Character:
            return true;
        
        default:
            return false;
    }
}

bool isBinaryOp(TokenType type) {
    // FIXME: Bit operators not included for now
    switch (type) {
        case TokenType::Equal:

        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:

        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::TimesEqual:
        case TokenType::DivideEqual:

        case TokenType::DoubleEqual:
        case TokenType::NotEqual:
        case TokenType::MTEqual:
        case TokenType::LTEqual:
        case TokenType::RAngle: // More than
        case TokenType::LAngle: // Less than

        case TokenType::LogAND:
        case TokenType::LogOR:
            return true;

        default:
            return false;
    }
}
*/

// TODO: Not yet needed. We don't have scopes yet
// class Block : Expr {
// private:
//     llvm::SmallVector<ExprPtr> stmts;
// public:
//     Block(llvm::SmallVector<ExprPtr>&& stmts) : stmts(std::move(stmts)) {}

//     llvm::Value* codegen() {
//         llvm::outs() << "Scopes or 'block statements' are not yet supported.\n";
//         return nullptr;
//     }
// };

class Variable : public Expr {
private:
    TokenType mutability; // Const, Let, Mut, Temp or Move if it's a moved function arg
    llvm::StringRef name;
    std::unique_ptr<Expr> value;
public:
    // Variable(TokenType kind, llvm::StringRef name) : mutability(kind), name(name) {}
    Variable(TokenType kind, llvm::StringRef name, ExprPtr&& value)
        : mutability(kind), name(std::move(name)), value(std::move(value)) {}

    llvm::Value* codegen() {
        if (namedValues.contains(name)) {
            llvm::outs() << "Variable '" << name << "' already exists.\n";
            return nullptr;
        }

        llvm::Type* var_type = llvm::Type::getDoubleTy(*context);
        llvm::AllocaInst* alloca = builder->CreateAlloca(var_type, nullptr, name);
        if (value) {
            llvm::Value* v = value->codegen();
            if (!v) return nullptr;

            builder->CreateStore(v, alloca);
        } else
            builder->CreateStore(llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), 0.0), alloca);

        namedValues.try_emplace(name.str(), alloca);
        return alloca;
    }

    constexpr std::string str() const { return "Variable"; }
};

class VariableRef : public Expr {
private:
    llvm::StringRef name;
public:
    VariableRef(llvm::StringRef name) : name(std::move(name)) {}

    llvm::Value* codegen() {
        if (namedValues.contains(name))
            return namedValues[name];
        
        llvm::outs() << "Use of undeclared variable: '" << name << "'\n";
        return nullptr;
    }

    constexpr std::string str() const { return "VariableRef"; }
};

// NOTE: 
// We're going to temporarily treat IntLiterals the same 
// as floats until types are introduced

/*
class IntLiteral : public Expr {
private:
    size_t value;
public:
    IntLiteral(size_t value) : value(std::move(value)) {}

    llvm::Value* codegen() {
        assert(false && "Int literals are not yet supported");
        return nullptr;
    }

    constexpr std::string str() const { return "IntLiteral"; }
};
*/

class FloatLiteral : public Expr {
private:
    double value;
public:
    FloatLiteral(double value) : value(std::move(value)) {}

    llvm::Value* codegen() {
        return llvm::ConstantFP::get(
            llvm::Type::getDoubleTy(*context), llvm::APFloat(value)
        );
    }

    constexpr std::string str() const { return "FloatLiteral"; }
};

// NOTE: Temporary IntLiteral class to treat int literals the same as floats
class IntLiteral : public FloatLiteral {
public:
    constexpr std::string str() const { return "IntLiteral"; }
};

class BinaryOp : public Expr {
private:
    TokenType Op; // The operator used
    ExprPtr LHS;
    ExprPtr RHS;
public:
    BinaryOp(TokenType kind, ExprPtr&& lhs, ExprPtr&& rhs) 
        : Op(kind), LHS(std::move(lhs)), RHS(std::move(rhs)) {}

    llvm::Value* codegen() {
        llvm::Value* L = LHS->codegen();
        llvm::Value* R = RHS->codegen();
        if (!L || !R) return nullptr;

        switch (Op) {
            // Comparison operators
            case TokenType::EqualEqual:
                return builder->CreateICmpEQ(L, R, "eqtmp");
            case TokenType::ExclEqual:
                return builder->CreateICmpNE(L, R, "netmp");
            case TokenType::More:
                return builder->CreateICmpSLT(L, R, "gttmp");
            case TokenType::MoreEqual:
                return builder->CreateICmpSLE(L, R, "getmp");
            case TokenType::Less:
                return builder->CreateICmpSLT(L, R, "lttmp");
            case TokenType::LessEqual:
                return builder->CreateICmpSLE(L, R, "letmp");

            // Mathematical operators
            case TokenType::Plus:
                return builder->CreateAdd(L, R, "addtmp");
            case TokenType::Minus:
                return builder->CreateSub(L, R, "subtmp");
            case TokenType::Star: // Times
                return builder->CreateMul(L, R, "multmp");
            case TokenType::Slash: // Divide
                // if (L->getType()->isIntegerTy()) { // Signed or unsigned
                //     return builder->CreateSDiv(L, R, "divtmp");  
                // } else
                //     return builder->CreateFDiv(L, R, "fdivtmp");
                // NOTE: Currently only works with float
                return builder->CreateFDiv(L, R, "fdivtmp");

            // TODO: Implement assignment operators

            // Incremental/decremental operators
            case TokenType::PlusPlus:
                break;
            case TokenType::MinusMinus:
                break;
            case TokenType::PlusEqual:
                break;
            case TokenType::MinusEqual:
                break;
            case TokenType::StarEqual:
                break;
            case TokenType::SlashEqual:
                break;

            // Assignment
            case TokenType::Equal:
                break;
        }

        llvm::outs() << "Unsupported operator type: " << strTokenType(Op) << "\n";
        return nullptr;
    }

    constexpr std::string str() const { return "BinaryOp"; }
};

// TODO: Functions not yet supported
// class Function : Decl {
// private:
//     llvm::StringRef name;
//     llvm::SmallVector<Variable, 0> args;
//     llvm::StringRef body;
// public: 
//     Function(llvm::StringRef name, llvm::SmallVector<Variable, 0>&& args, llvm::StringRef body)
//     : name(std::move(name)), args(std::move(args)), body(body) {}
// };

} // namespace ast
} // namespace c
