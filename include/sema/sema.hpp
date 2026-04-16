#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include "include/Frontend/ASTAllocator.hpp"
#include "include/Lexer/utils.hpp"
#include "include/Sema/Lookup.hpp"
#include "include/Sema/Scope.hpp"

namespace c {

class Sema {
private:
    ASTAllocator& Alloc;

    // Set inside analyzeFunctionDecl. Only inside analyzeReturnStmt to 
    // validate that the return value matches the functions return type.
    const ast::FunctionDecl* CurrentFunction;
public:
    Sema(ASTAllocator& alloc)
        : Alloc(alloc) {}

    bool analyze(ast::ModuleDecl* topModule) {
        sema::Scope scope(nullptr, sema::Scope::ModuleScope);
        scope.setEntity(topModule);

        for (ast::Decl* DC : topModule->decls())
            if (analyzeDecl(DC, &scope))
                return true;
        return false;
    }

    sema::Lookup lookup(sema::Scope* scope, llvm::StringRef name, 
                        bool localOnly = false,
                        sema::Lookup::LookupKind kind = sema::Lookup::Any) {
        return sema::Lookup(scope, name, kind, localOnly);
    }

    bool validateTypeCast(const Type* from, const Type* to, bool isExplicit) {
        // This also captures struct types since they can't be converted.
        if (to == from) // Pointer comparison
            return false;

        if (to->isBuiltinTy()) {
            // TODO: This will have to be changed when character builtins are added.
            if (!isExplicit)
                return false;

            auto to_bt = llvm::cast<BuiltinType>(to);
            if (from->isPointerTy()) {
                // TODO: Give a warning that information may be lost if "to" 
                // has less bytes than the builtin pointer type.
                return false;
            } else if (!from->isBuiltinTy())
                return true;

            auto from_bt = llvm::cast<BuiltinType>(from);
            
            // Casting from unsigned to signed or from signed to unsigned
            if (from_bt->isUnsigned() && to_bt->isSigned())
                llvm::outs() << "Casting from usigned to signed migth result in overflow.\n";
            if (from_bt->isSigned() && to_bt->isUnsigned())
                llvm::outs() << "Casting from signed to unsigned mugth result in underflow.\n";

            // TODO: Give a warning that information may be lost if "from" has more bits than "to"

            return false;
        }

        if (to->isPointerTy()) {
            if (!from->isPointerTy())
                return true;

            auto to_ptr = llvm::cast<PointerType>(to);
            auto from_ptr = llvm::cast<PointerType>(from);
            if (to_ptr->isRaw() != from_ptr->isRaw())
                return true;

            return validateTypeCast(
                from_ptr->getPointee(), to_ptr->getPointee(), isExplicit
            );
        }

        if (to->isArrayTy()) {
            if (!from->isArrayTy())
                return true;

            auto to_arr = llvm::cast<ArrayType>(to);
            auto from_arr = llvm::cast<ArrayType>(from);
            if (to_arr->getInitSize() < from_arr->getInitSize())
                return true;
            
            return validateTypeCast(
                from_arr->getElemType(), to_arr->getElemType(), isExplicit
            );
        }

        if (to->isStructTy())
            // One struct type can't be cast to another struct type (currently)
            // So this will always be true since struct types will be captured
            // with a pointer comparison.
            return true;

        // Either triggered because of an error in the logic or because a new
        // subclass of Type has been created without adding a condition for it.
        llvm_unreachable("No condition exists for that type.");
    }

    bool analyzeDecl(ast::Decl* DC, sema::Scope* scope) {
        switch (DC->getKind()) {
            default:
                if (llvm::isa<ast::ParamDecl>(DC) ||
                    llvm::isa<ast::FieldDecl>(DC))
                    llvm_unreachable("ParamDecl and FieldDecl are proprietary to FunctionDecl and "
                                     "StructDecl and is therefore handled by their sema methods.");
                llvm_unreachable("Decl subclass is missing a case statement.");

            case ast::Decl::TypeAliasDeclKind:
                return analyzeTypeAliasDecl(llvm::cast<ast::TypeAliasDecl>(DC), scope);
            case ast::Decl::StructDeclKind:
                return analyzeStructDecl(llvm::cast<ast::StructDecl>(DC), scope);

            // Don't need cases for ParamDecl and FieldDecl since those are 
            // handled in analyzeStructDecl() and analyzeFunctionDecl().
            case ast::Decl::VarDeclKind:
                return analyzeInitDecl(llvm::cast<ast::InitDecl>(DC), scope, true);
            
            case ast::Decl::FieldDeclKind:
                return analyzeFuncionDecl(llvm::cast<ast::FunctionDecl>(DC), scope);

            case ast::Decl::ModuleDeclKind:
                llvm_unreachable("Not implemented");
        }
    }

    bool analyzeStmt(ast::Stmt* ST, sema::Scope* scope) {
        assert(!llvm::isa<ast::Expr>(ST) && "Statement is an expression.");
        switch (ST->getKind()) {
            default:
                llvm_unreachable("Stmt subclass missing a case statement.");

            case ast::Stmt::DeclStmtKind:
                ast::DeclStmt* s = llvm::cast<ast::DeclStmt>(ST);
                if (!llvm::isa<ast::VarDecl>(s->getDecl())) {
                    llvm::outs() << "Currently only VarDecl is allowed for DeclStmt.\n";
                    return true;
                }

                return analyzeInitDecl(llvm::cast<ast::InitDecl>(s->getDecl()), scope, true);
            
            case ast::Stmt::IfStmtKind:
                return analyzeIfStmt(llvm::cast<ast::IfStmt>(ST), scope);
            case ast::Stmt::WhileStmtKind:
                return analyzeWhileStmt(llvm::cast<ast::WhileStmt>(ST), scope);
                
            case ast::Stmt::ReturnStmtKind:
                return analyzeReturnStmt(llvm::cast<ast::ReturnStmt>(ST), scope);

            case ast::Stmt::BlockStmtKind:
                return analyzeBlockStmt(llvm::cast<ast::BlockStmt>(ST), scope);
        }
    }

    bool analyzeExpr(ast::Expr* EX, sema::Scope* scope, bool allowAssign) {
        switch (EX->getKind()) {
            default:
                llvm_unreachable("Expr subclass missing a case statement.");

            // Expressions that can be both lvalue and rvalue
            case ast::Stmt::BinaryOperatorKind:
                auto binop = llvm::cast<ast::BinaryOperator>(EX);
                if (binop->isAssignOp()) {
                    if (!allowAssign)
                        goto assignment_disallowed_error;

                    return analyzeBinaryAssign(binop, scope);
                }

                return analyzeBinaryOperator(binop, scope);
            case ast::Stmt::UnaryOperatorKind:
                auto unaryop = llvm::cast<ast::UnaryOperator>(EX);
                if (unaryop->isAssignOp()) {
                    if (!allowAssign)
                        goto assignment_disallowed_error;
                    
                    return analyzeUnaryAssign(unaryop, scope);
                } else if (unaryop->isDerefOp())
                    return analyzeDereference(unaryop, scope);

                return analyzeUnaryOperator(unaryop, scope);

            // TODO: Remove the note when function pointers and struct with functions are implemented.
            // NOTE: These expressions currently can't point to a FunctionDecl
            // Expressions that can point to an lvalue or a FunctionDecl (which is rvalue)
            case ast::Stmt::DeclRefExprKind:
                return analyzeDeclRef(llvm::cast<ast::DeclRefExpr>(EX), scope);
            case ast::Stmt::AccessExprKind:
                return analyzeAccessExpr(llvm::cast<ast::AccessExpr>(EX), scope);
            
            // Expressions that are lvalues
            case ast::Stmt::SliceExprKind:
                return analyzeSliceExpr(llvm::cast<ast::SliceExpr>(EX), scope);
            
            // Expressions that are rvalues
            case ast::Stmt::CallExprKind:
                return analyzeCallExpr(llvm::cast<ast::CallExpr>(EX), scope);
            case ast::Stmt::ArrayLiteralKind:
            case ast::Stmt::IntegerLiteralKind:
            case ast::Stmt::FloatingLiteralKind:
            case ast::Stmt::BooleanLiteralKind:
                llvm_unreachable("Not implemented");
        }

    // Labels are retarded but let's use them anyway!
    assignment_disallowed_error:
        llvm::outs() << "Assignment dissallowed.\n";
        return true;
    }
private:
    bool resolveTypeInfo(ast::TypeInfo* info, sema::Scope* scope) {
        auto result = lookup(scope, info->getName(), false, sema::Lookup::Type).find();
        if (!result) {
            llvm::outs() << "Type definition not found.\n";
            return true;
        }

        const Type* type = result.getAsSingle<ast::TypeDecl>()->getDeclType();
        if (!info->isTypeCompatible(type)) {
            llvm::outs() << "Incompatible types.\n";
            return true;
        }

        info->setType(type);
        return false;
    }

    bool analyzeModuleDecl(ast::ModuleDecl* DC, sema::Scope* scope) {
        // FIXME:
    }

    bool analyzeInitDecl(ast::InitDecl* DC, sema::Scope* scope, bool isVarDecl = false) {
        // LookupKind::Value also looks for FunctionDecl but that's fine since
        // function declarations within function bodies is not allowed. We do 
        // not use Lookup::getKind() here since we want to look for both 
        // VarDecl and ParamVarDecl.
        if (lookup(scope, DC->getName(), true, sema::Lookup::Value).find()) {
            llvm::outs() << "A variable called '" << DC->getName().str() << "' already exists.\n";
            return true;
        }

        if (!DC->hasInit()) {
            if (isVarDecl) {
                llvm::outs() << "Variable declarations must have an init value.\n";
                return true;
            }

            if (DC->isAutoTyped()) {
                llvm::outs() << "Automatically type evalueated Decl must have a value.\n";
                return true;
            }

            return false;
        }

        // Allow assign to allow expressions like "int x = y = z"
        if (analyzeExpr(DC->getInit(), scope, true))
            return true;

        if (DC->isAutoTyped()) {
            if (DC->getInit()->isTypeDependant()) {
                llvm::outs() << "Type dependant expression can't be automatically evaluated.\n";
                return true;
            }

            // FIXME: Maybe check for void function call

            DC->getTypeInfo()->setType(DC->getInit()->getType());
            scope->pushDecl(DC);
            return false;
        }

        if (DC->getInit()->isTypeDependant()) {
            // FIXME: 
            llvm_unreachable("Currently no type dependant expressions (i think).");
        } else if (validateTypeCast(DC->getInit()->getType(), DC->getType(), false)) {
            llvm::outs() << "Invalid type conversion\n";
            return true;
        }

        return false;
    }

    bool analyzeFuncionDecl(ast::FunctionDecl* DC, sema::Scope* scope) {
        if (lookup(scope, DC->getName(), true).findKind(DC->getKind())) {
            llvm::outs() << "A function called '" << DC->getName().str() << "' already exists.\n";
            return true;
        }

        sema::Scope proto(scope, sema::Scope::FnPrototypeScope);
        proto.setFnPrototype(&proto);
        proto.setEntity(DC);

        if (DC->hasParams())
            for (ast::ParamDecl* param : DC->parameters()) {
                if (analyzeInitDecl(param, &proto))
                    return true;
                proto.pushDecl(param);
            }

        if (!DC->isVoid() && resolveTypeInfo(DC->getTypeInfo(), &proto))
            return true;
            
        CurrentFunction = DC;
        if (analyzeBlockStmt(DC->getBody(), &proto))
            return true;

        scope->pushDecl(DC);
        return false;
    }

    bool validateTypeDefName(llvm::StringRef name, sema::Scope* scope) {
        auto builtins = lexer::getBuiltins();
        if (builtins.find(name) != builtins.end()) {
            llvm::outs() << "Type declration name conflicts with builtin type.\n";
            return true;
        }

        if (lookup(scope, name, false, sema::Lookup::Type).find()) {
            llvm::outs() << "A type called '" << name.str() << "' already exists.\n";
            return true;
        }

        return false;
    }

    bool analyzeTypeAliasDecl(ast::TypeAliasDecl* DC, sema::Scope* scope) {
        if (validateTypeDefName(DC->getName(), scope))
            return true;

        if (resolveTypeInfo(DC->getTypeInfo(), scope))
            return true;

        DC->setDeclType(DC->getTypeInfo()->getType());
        scope->pushDecl(DC);
        return false;
    }

    bool analyzeStructDecl(ast::StructDecl* DC, sema::Scope* scope) {
        if (validateTypeDefName(DC->getName(), scope))
            return true;

        sema::Scope body(scope, sema::Scope::StructScope);
        body.setEntity(DC);

        for (ast::Decl* d : DC->decls()) {
            if (auto FD = llvm::dyn_cast<ast::FieldDecl>(d)) {
                if (analyzeInitDecl(FD, &body))
                    return true;
            } else if (auto TD = llvm::dyn_cast<ast::TypeAliasDecl>(d)) {
                if (analyzeTypeAliasDecl(TD, &body))
                    return true;
            } else {
                llvm::outs() << "Only field declarations and type aliases are allowed in struct declarations.\n";
                return true;
            }

            // Can only be FieldDecl or TypeAliasDecl, which are both NamedDecl.
            body.pushDecl(llvm::cast<ast::NamedDecl>(d));
        }

        DC->setDeclType(Alloc.Create<StructType>(DC));
        scope->pushDecl(DC);
        return false;
    }

    bool analyzeBlockStmt(ast::BlockStmt* ST, sema::Scope* scope) {
        // BlockStmt can only be within the scope of a function. There's currently 
        // also no flags for different scopes within a function body so therefore 
        // we just hardcode the flag to be FunctionScope.
        sema::Scope body(scope, sema::Scope::FunctionScope);
        body.setFnPrototype(scope->getFnPrototype());

        for (ast::Stmt* s : ST->stmts()) {
            if (auto DC = llvm::dyn_cast<ast::DeclStmt>(s)) {
                if (!llvm::isa<ast::VarDecl>(DC)) {
                    llvm::outs() << "Only variable declarations are allowed inside the body of a function.\n";
                    return true;
                }

                if (analyzeDecl(DC->getDecl(), &body))
                    return true;

                body.pushDecl(DC->getDecl());
            } else if (analyzeStmt(s, &body))
                return true;
        }

        return false;
    }

    bool analyzeReturnStmt(ast::ReturnStmt* ST, sema::Scope* scope) {
        if (ST->isVoid()) {
            if (CurrentFunction->isVoid())
                return false;
            
            llvm::outs() << "Can't have a void return in a non void function.\n";
            return true;
        }

        if (analyzeExpr(ST->getValue(), scope, false))
            return true;

        if (validateTypeCast(ST->getValue()->getType(), CurrentFunction->getType(), false)) {
            llvm::outs() << "Type of returned value could not be converted to the functions return type.\n";
            return true;
        }
        
        return false;
    }

    bool analyzeConditionalStmtBody(ast::Stmt* body, sema::Scope* scope) {
        // TODO: Add checks to check that the body acually does somthing.

        if (auto EX = llvm::dyn_cast<ast::Expr>(body)) {
            if (analyzeExpr(EX, scope, true))
                return true;

            return false;
        }

        if (analyzeStmt(body, scope))
            return true;

        return false;
    }

    bool analyzeIfStmt(ast::IfStmt* ST, sema::Scope* scope) {
        sema::Scope if_scope(scope, sema::Scope::FunctionScope);

        ast::IfStmt* current = ST;
        while (true) {
            if (analyzeCondition(ST->getCondition(), &if_scope))
                return true;

            if (analyzeConditionalStmtBody(ST->getBody(), &if_scope))
                return true;

            if (!current->hasElse())
                break;
            if (!llvm::isa<ast::IfStmt>(current->getElse()))
                if (analyzeConditionalStmtBody(ST->getBody(), &if_scope))
                    return true;

            current = llvm::cast<ast::IfStmt>(ST->getElse());
        }

        return false;
    }

    bool analyzeWhileStmt(ast::WhileStmt* ST, sema::Scope* scope) {
        sema::Scope while_scope(scope, sema::Scope::FunctionScope);
        if (analyzeCondition(ST->getCondition(), scope))
            return true;

        if (analyzeConditionalStmtBody(ST->getBody(), &while_scope))
            return true;

        return false;
    }

    bool analyzeCondition(ast::Expr* EX, sema::Scope* scope) {
        // Assigning is not allowed inside of conditions
        if (analyzeExpr(EX, scope, false))
            return true;
        
        // Can only be false for arithmetic and non-binary expressions
        if (!validateTypeCast(EX->getType(), Alloc.getBuiltinType(BuiltinType::Bool), false)) {
            llvm::outs() << "Invalid type conversion to bool inside condition.\n";
            return false;
        }

        return false;
    }

    bool validateExprIsMutable(ast::Expr* EX) {
        assert(EX->isStorageLocation() && "Check if the Expression is an LValue before calling valudateExprIsMutable.\n");
        if (auto ref = llvm::dyn_cast<ast::DeclRefExpr>(EX)) {
            // Will always be either VarDecl or ParamDecl
            bool is_mutable;
            if (auto VD = llvm::dyn_cast<ast::VarDecl>(ref->getDecl())) {
                is_mutable =  VD->isMutable();
            } else if (auto PD = llvm::dyn_cast<ast::ParamDecl>(ref->getDecl())) {
                is_mutable = PD->isMutable();
            } else llvm_unreachable("");

            if (!is_mutable) {
                llvm::outs() << "Can't assign to an immutable value.\n";
                return true;
            }

            return false;
        } else if (llvm::isa<ast::UnaryOperator>(EX)) {
            return validateExprIsMutable(llvm::cast<ast::UnaryOperator>(EX)->getSubExpr());
        } else if (llvm::isa<ast::QualExpr>(EX)) { // AccessExpr and SliceExpr
            return validateExprIsMutable(llvm::cast<ast::QualExpr>(EX)->getBase());
        }

        llvm_unreachable("");
    }

    bool analyzeBinaryAssign(ast::BinaryOperator* EX, sema::Scope* scope) {
        if (!EX->getLHS()->isStorageLocation()) {
            llvm::outs() << "Can only assign to an identifiable memory location.\n";
            return true;
        }

        if (analyzeExpr(EX->getLHS(), scope, false))
            return true;
        
        // We allow assignment here to allow expressions like "a = b = c"
        if (analyzeExpr(EX->getRHS(), scope, true))
            return true;

        if (!validateTypeCast(EX->getRHS()->getType(), EX->getLHS()->getType(), false)) {
            llvm::outs() << "Invalid type conversion in assignment expression.\n";
            return true;
        }

        EX->setType(EX->getLHS()->getType());
        return false;
    }

    bool analyzeBinaryOperator(ast::BinaryOperator* EX, sema::Scope* scope) {
        // Assigning is not allowed in conditional and arithmetic binary operators
        if (analyzeExpr(EX->getLHS(), scope, false))
            return true;
        if (analyzeExpr(EX->getRHS(), scope, false))
            return true;

        if (!validateTypeCast(EX->getRHS()->getType(), EX->getRHS()->getType(), false)) {
            llvm::outs() << "Invalid type conversion.\n";
            return true;
        }

        if (EX->isComparisonOp()) {
            EX->setType(Alloc.getBuiltinType(BuiltinType::Bool));
        } else { // Arithmetic operator
            EX->setType(EX->getLHS()->getType());
            if (EX->getType()->isStructTy()) {
                llvm::outs() << "Can't perform arithmetic on a struct type.\n";
                return true;
            }
        }

        return false;
    }

    bool analyzeDereference(ast::UnaryOperator* EX, sema::Scope* scope) {
        if (analyzeExpr(EX->getSubExpr(), scope, false))
            // Like in Rust we disallow manual pointer arithmetic.
            return true;

        auto type = llvm::dyn_cast<PointerType>(EX->getSubExpr()->getType());
        if (!type) {
            llvm::outs() << "Can't dereference a non-pointer type.\n";
            return true;
        }

        if (EX->isRawPtrOp() != type->isRaw()) {
            llvm::outs() << "Mismatched dereference operator.\n";
            return true;
        }

        EX->setType(type->getPointee());
        return false;
    }

    bool analyzeUnaryAssign(ast::UnaryOperator* EX, sema::Scope* scope) {
        if (analyzeExpr(EX->getSubExpr(), scope, false))
            return true;
        
        EX->setType(EX->getSubExpr()->getType());

        // TODO: This will have to be changed when operator methods are implemented.
        BuiltinType* int_ty = Alloc.getBuiltinType(BuiltinType::U64); // FIXME: Don't know which int type to pick...
        if (validateTypeCast(EX->getType(), int_ty, false)) {
            llvm::outs() << "'++' and '--' operators not valid for expression.\n";
            return true;
        }

        return false;
    }

    bool analyzeUnaryOperator(ast::UnaryOperator* EX, sema::Scope* scope) {
        if (analyzeExpr(EX->getSubExpr(), scope, false))
            return true;

        if (EX->isAdressOp()) {
            if (auto DC = llvm::dyn_cast<ast::DeclRefExpr>(EX))
                if (!llvm::isa<ast::ValueDecl>(DC->getDecl())) {
                    llvm::outs() << "Can only get a pointer to a value or a function.\n";
                    return true;
                } else if (llvm::isa<ast::FunctionDecl>(DC)) {
                    // TODO: Remove this else statement when function pointers are implemented
                    llvm::outs() << "Function pointers are not yet supported.\n";
                    return true;
                }
            EX->setType(Alloc.Create<PointerType>(
                EX->isRawPtrOp(), EX->getSubExpr()->getType()
            ));
        } if (EX->isNegateOp()) {
            Type* bool_ty = Alloc.getBuiltinType(BuiltinType::Bool);
            if (!validateTypeCast(EX->getType(), bool_ty, false)) {
                llvm::outs() << "Type is not bool convertable.\n";
                return true;
            }

            EX->setType(bool_ty);
        } else if (EX->isArithmeticOp()) {
            Type* int_ty = Alloc.getBuiltinType(BuiltinType::U64); // FIXME: Don't know which int type to pick...
            if (!validateTypeCast(EX->getSubExpr()->getType(), int_ty, false)) {
                llvm::outs() << "'+' and '-' prefixes are only valid for numbers.\n";
                return true;
            }

            EX->setType(EX->getSubExpr()->getType());
        } else
            // Probably means you've called the wrong function.
            llvm_unreachable("Invalid unary operator.");

        return false;
    }

    // TODO: Currently doesn't support looking up anything else than
    // InitDecl. This is fine for now but will have to be updated
    // when function pointers among other things, are introduced.
    bool analyzeDeclRef(ast::DeclRefExpr* EX, sema::Scope* scope) {
        auto result = lookup(scope, EX->getName(), false, sema::Lookup::Value).find().getAsSingle();
        if (!result) {
            llvm::outs() << "Could not find a Decl called '" << EX->getName() << "'\n";
            return true;
        }

        if (auto VD = llvm::dyn_cast<ast::InitDecl>(result)) {
            EX->setType(VD->getType());
        } else 
            // TODO: Change when function pointers are implemented
            llvm_unreachable("Function pointers are currently not supported in Sema");

        EX->setDecl(result);
        return false;
    }

    bool analyzeSliceExpr(ast::SliceExpr* EX, sema::Scope* scope) {
        if (analyzeExpr(EX->getBase(), scope, false))
            return true;

        if (!EX->getBase()->getType()->isArrayTy()) {
            llvm::outs() << "Must be array type to access an index.\n";
            return true;
        }

        // TODO: Should eventually support python-like slicing syntax.

        if (analyzeExpr(EX->getStart(), scope, false))
            return true;

        Type* int_ty = Alloc.getBuiltinType(BuiltinType::I64); // FIXME: Don't know which int type to pick...
        if (!validateTypeCast(EX->getStart()->getType(), int_ty, false)) {
            llvm::outs() << "Array intdex must be convertable to an integer.\n";
            return true;
        }

        EX->setType(llvm::cast<ArrayType>(EX->getBase())->getElemType());
        return false;
    }

    bool analyzeAccessExpr(ast::AccessExpr* EX, sema::Scope* scope) {
        if (analyzeExpr(EX->getBase(), scope, false))
            return true;

        if (!EX->getBase()->getType()->isStructTy()) {
            llvm::outs() << "Only struct types have attributes.\n";
            return true;
        }

        auto type = llvm::cast<StructType>(EX->getBase()->getType());
        for (ast::FieldDecl* FD : type->getDecl()->fields())
            if (FD->getName() == EX->getFieldName()) {
                EX->setFieldDecl(FD);
                return false;
            }

        llvm::outs() << "Struct has not attribute called '" << EX->getFieldName() << "'.\n";
        return true;
    }

    bool analyzeCallExpr(ast::CallExpr* EX, sema::Scope* scope) {
        // TODO: Find a better way of doing this. This if statement if very ugly.
        llvm::SmallVector<ast::InitDecl*> params;
        if (auto fn_result = lookup(scope, EX->getCallee()).findKind(ast::Decl::FunctionDeclKind)) {
            auto result = fn_result.getAsSingle<ast::FunctionDecl>();
            params = llvm::SmallVector<ast::InitDecl*>(result->parameters());
            EX->setCalleeDecl(result);
            EX->setType(result->getType());
        } else if (auto st_result = lookup(scope, EX->getCallee()).findKind(ast::Decl::StructDeclKind)) {
            auto result = st_result.getAsSingle<ast::StructDecl>();
            params = llvm::SmallVector<ast::InitDecl*>(result->fields());
            EX->setCalleeDecl(result);
            EX->setType(result->getDeclType());
        } else {
            llvm::outs() << "No function or struct called '" << EX->getCallee() << "' was found.\n";
            return true;
        }
        
        for (size_t i = 0; i < params.size(); i++) {
            if (i < EX->getAmntArgs()) {
                ast::Expr* arg = EX->getArg(i);
                // TODO: Condiser adding logic to create a Scope with a flag for 
                // conditions and calls since both disallow assignment, this to 
                // differentiate them in error messages.
                if (analyzeExpr(arg, scope, false))
                    return true;

                if (!validateTypeCast(arg->getType(), params[i]->getType(), false)) {
                    llvm::outs() << "Ivalid type conversion.\n";
                    return true;
                }
            } else {
                llvm::outs() << "Mismatched call to .\n";
                return true;
            }
        }

        return false;
    }
};

} // namespace c
