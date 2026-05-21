#include "include/Sema/Lookup.hpp"
#include "include/Sema/Sema.hpp"

#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/Lexer/utils.hpp"

#include "src/Debug.hpp"

using namespace c::ast;
using namespace c::sema;

void printScopeStackDecls(Scope* scope) {
    llvm::outs() << "Scope: ";
    if (scope->isModuleScope()) {
        llvm::outs() << "ModuleScope";
    } else if (scope->isStructScope()) {
        llvm::outs() << "StructScope";
    } else if (scope->isFunctionScope()) {
        llvm::outs() << "FunctionScope";
    } else if (scope->isFunctionPrototypeScope()) {
        llvm::outs() << "FunctionPrototypeScope";
    } else
        llvm_unreachable("");
    llvm::outs() << "\n";

    for (auto DC : scope->decls()) 
        llvm::outs() << DC->getDeclKindName() << " " << DC->getName() << "\n";
    
    if (scope->hasParent())
        printScopeStackDecls(scope->getParent());
}

namespace c {

namespace sema {

template <typename T>
T* LookupResult::getAsSingle() const {
    // Check T is a valid subclass of NamedDecl.
    static_assert(std::is_base_of_v<ast::Decl, T> && "T must be subclass of Decl.");
    static_assert(std::is_base_of_v<ast::NamedDecl, T> && "T must be a subclass of NamedDecl.");

    for (ast::NamedDecl* DC : Decls)
        if (llvm::isa<T>(DC))
            return llvm::cast<T>(DC);
    return nullptr;
}

LookupResult Lookup::findKind(ast::Decl::Kind kind) const {
    Scope* current = LookupScope;
    while (current != nullptr) {
        for (ast::NamedDecl* DC : current->decls())
            if (DC->getKind() == kind && DC->getName() == Name)
                return LookupResult(DC);

        if (current->isFunctionPrototypeScope() && LocalOnly)
            return LookupResult();
        current = current->getParent();
    }

    return LookupResult();
}

LookupResult Lookup::doLookup(bool isSingle) const {
    LookupResult result;

    LookupFilter is_lookup_kind = getLookupFilter();
    Scope* current = getStartScope();
    while (current != nullptr) {
        for (ast::NamedDecl* DC : current->decls()) {
            if (is_lookup_kind(DC) && DC->getName() == Name) {
                if (isSingle)
                    return LookupResult(DC);

                result.pushDecl(DC);
            }
        }

        if (current->isFunctionPrototypeScope() && LocalOnly)
            break;

        current = current->getParent();
    }

    return result;
}

Scope* Lookup::getStartScope() const {
    switch (Kind) {
        default:
            llvm_unreachable("LookupKind flag is missing a case.");
        case Any:
        case Value:
            return LookupScope;
        case Type:
            if (LookupScope->isFunctionScope())
                // TODO: Remove the note when templates are implemented.
                // NOTE: Functions currently can't have templates. 
                // Get  the prototype of the function instead of the functions 
                // parent scope because the function can have a template. 
                return LookupScope->getFnPrototype();
            return LookupScope;
    }
}

// This language is too stupid to understand you can just use llvm::isa.
// So here I am implementing my own wrapper function around llvm::isa... 
// I love my life...
template <typename T>
bool isaWrapper(ast::Decl* DC) {
    return llvm::isa<T>(DC);
}

Lookup::LookupFilter Lookup::getLookupFilter() const {
    switch (Kind) {
        default:
            // If you reach this point then congratulations, you are officially stupid.
            llvm_unreachable("LookupKind flag is missing a case.");

        case Any:
            // Lambda function are stupid but let's use them anyway!
            return [](ast::Decl* DC) {
                return true;
            };
        case Value:
            // Captures VarDecl, ParamDecl, FieldDecl and FunctionDecl.
            // Because function pointers are fun... (that's what *she* said)
            return &isaWrapper<ast::ValueDecl>;
        case Type:
            // TODO: Update this when more TypeDecl subclasses are added
            // Captures TypeAliasDecl and StructDecl
            // Because type are just... built different... or somthing...
            return &isaWrapper<ast::TypeDecl>;
    }
}

} // namespace sema

bool Sema::analyze(ast::ModuleDecl* topModule) {
    return analyzeModuleDecl(topModule, nullptr);
}

// Returns true if casting will cause an error.
bool Sema::validateTypeCast(const Type* from, const Type* to, bool isExplicit) {
    DEBUG("Called: validateTypeCast");
    assert(from != nullptr);
    assert(to != nullptr);

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
            llvm::outs() << "Casting from signed to unsigned migth result in underflow.\n";

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

bool Sema::analyzeDecl(Decl* DC, Scope* scope) {
    DEBUG("Called: analyzeDecl");
    switch (DC->getKind()) {
        default:
            if (llvm::isa<ParamDecl>(DC) ||
                llvm::isa<FieldDecl>(DC))
                llvm_unreachable("ParamDecl and FieldDecl are proprietary to FunctionDecl and "
                                    "StructDecl and is therefore handled by their sema methods.");
            llvm_unreachable("Decl subclass is missing a case statement.");

        case Decl::TypeAliasDeclKind:
            return analyzeTypeAliasDecl(llvm::cast<TypeAliasDecl>(DC), scope);
        case Decl::StructDeclKind:
            return analyzeStructDecl(llvm::cast<StructDecl>(DC), scope);

        // Don't need cases for ParamDecl and FieldDecl since those are 
        // handled in analyzeStructDecl() and analyzeFunctionDecl().
        case Decl::VarDeclKind:
            return analyzeInitDecl(llvm::cast<InitDecl>(DC), scope, true);
        
        case Decl::FunctionDeclKind:
            return analyzeFuncionDecl(llvm::cast<FunctionDecl>(DC), scope);

        case Decl::ModuleDeclKind:
            llvm_unreachable("Not implemented");
    }
}

bool Sema::analyzeStmt(Stmt* ST, Scope* scope) {
    DEBUG("Called: analyzeStmt");
    assert(!llvm::isa<Expr>(ST) && "Statement is an expression.");
    switch (ST->getKind()) {
        default:
            llvm_unreachable("Stmt subclass missing a case statement.");

        case Stmt::DeclStmtKind: {
            DeclStmt* s = llvm::cast<DeclStmt>(ST);
            if (!llvm::isa<VarDecl>(s->getDecl())) {
                llvm::outs() << "Currently only VarDecl is allowed for DeclStmt.\n";
                return true;
            }

            return analyzeInitDecl(llvm::cast<InitDecl>(s->getDecl()), scope, true);
        }

        case Stmt::IfStmtKind:
            return analyzeIfStmt(llvm::cast<IfStmt>(ST), scope);
        case Stmt::WhileStmtKind:
            return analyzeWhileStmt(llvm::cast<WhileStmt>(ST), scope);
            
        case Stmt::ReturnStmtKind:
            return analyzeReturnStmt(llvm::cast<ReturnStmt>(ST), scope);

        case Stmt::BlockStmtKind:
            return analyzeBlockStmt(llvm::cast<BlockStmt>(ST), scope);
    }
}

bool Sema::analyzeExpr(Expr* EX, Scope* scope, bool allowAssign) {
    DEBUG("Called: analyzeExpr");
    switch (EX->getKind()) {
        default:
            llvm_unreachable("Expr subclass missing a case statement.");

        // Expressions that can be both lvalue and rvalue
        case Stmt::BinaryOperatorKind: {
            auto binop = llvm::cast<BinaryOperator>(EX);
            if (binop->isAssignOp()) {
                if (!allowAssign)
                    goto assignment_disallowed_error;

                return analyzeBinaryAssign(binop, scope);
            }

            return analyzeBinaryOperator(binop, scope);
        }
        case Stmt::UnaryOperatorKind: {
            auto unaryop = llvm::cast<UnaryOperator>(EX);
            if (unaryop->isAssignOp()) {
                if (!allowAssign)
                    goto assignment_disallowed_error;
                
                return analyzeUnaryAssign(unaryop, scope);
            } else if (unaryop->isDerefOp())
                return analyzeDereference(unaryop, scope);

            return analyzeUnaryOperator(unaryop, scope);
        }

        // TODO: Remove the note when function pointers and struct with functions are implemented.
        // NOTE: These expressions currently can't point to a FunctionDecl
        // Expressions that can point to an lvalue or a FunctionDecl (which is rvalue)
        case Stmt::DeclRefExprKind:
            return analyzeDeclRef(llvm::cast<DeclRefExpr>(EX), scope);
        case Stmt::AccessExprKind:
            return analyzeAccessExpr(llvm::cast<AccessExpr>(EX), scope);
        
        // Expressions that are lvalues
        case Stmt::SliceExprKind:
            return analyzeSliceExpr(llvm::cast<SliceExpr>(EX), scope);
        
        // Expressions that are rvalues
        case Stmt::CallExprKind:
            return analyzeCallExpr(llvm::cast<CallExpr>(EX), scope);

        case Stmt::ArrayLiteralKind:
            return analyzeArrayLiteral(llvm::cast<ArrayLiteral>(EX), scope);
        case Stmt::IntegerLiteralKind:
            DEBUG("Case:   analyzeIntegerLiteral");
            EX->setType(Alloc.getIntegerType());
            return false;
        case Stmt::FloatingLiteralKind:
            DEBUG("Case:   analyzeFloatingLiteral");
            EX->setType(Alloc.getFloatingType());
            return false;
        case Stmt::BooleanLiteralKind:
            DEBUG("Case:   analyzeBooleanLiteral");
            EX->setType(Alloc.getBooleanType());
            return false;
    }

// Labels are retarded but let's use them anyway!
assignment_disallowed_error:
    llvm::outs() << "Assignment dissallowed.\n";
    return true;
}

bool Sema::resolveTypeInfo(TypeInfo* info, Scope* scope) {
    DEBUG("Called: resolveTypeInfo");
    assert(!info->isImplicitTy() && "Can't resolve implicit types.");
    if (info->isPointerTy()) {
        if (resolveTypeInfo(info->getPointee(), scope))
            return true;
        
        info->setType(Alloc.Create<PointerType>(
            info->isRawPointerTy(), info->getPointee()->getType()
        ));
        return false;
    } else if (info->isArrayTy()) {
        if (resolveTypeInfo(info->getPointee(), scope))
            return true;

        info->setType(Alloc.Create<ArrayType>(
            info->getPointee()->getType(), info->getArraySize()
        ));
        return false;
    }
    
    assert(info->isNamedTy() && "Unnamed type missing condition.");

    auto builtins = lexer::getBuiltinMap();
    auto builtin_result = builtins.find(info->getName());
    if (builtin_result != builtins.end()) {
        info->setType(Alloc.getBuiltinType(builtin_result->second));
        return false;
    }

    auto result = lookup(scope, info->getName(), false, Lookup::Type).find();
    if (!result) {
        llvm::outs() << "Type definition not found.\n";
        return true;
    }

    const Type* type = result.getAsSingle<TypeDecl>()->getDeclType();
    if (!info->isTypeCompatible(type)) {
        llvm::outs() << "Incompatible types.\n";
        return true;
    }

    info->setType(type);
    return false;
}

bool Sema::analyzeModuleDecl(ModuleDecl* DC, Scope* scope) {
    Scope mod(scope, Scope::ModuleScope);
    // mod.setEntity(DC);

    for (Decl* member : DC->decls())
        if (analyzeDecl(member, &mod))
            return true;
    return false;
}

bool Sema::analyzeInitDecl(InitDecl* DC, Scope* scope, bool isVarDecl) {
    DEBUG("Called: analyzeInitDecl");
    // LookupKind::Value also looks for FunctionDecl but that's fine since
    // function declarations within function bodies is not allowed. We do 
    // not use Lookup::getKind() here since we want to look for both 
    // VarDecl and ParamVarDecl.

    if (lookup(scope, DC->getName(), true, Lookup::Value).find()) {
        llvm::outs() << "A variable called '" << DC->getName().str() << "' already exists.\n";
        return true;
    }

    scope->pushDecl(DC);

    if (!DC->hasInit()) {
        if (isVarDecl) {
            llvm::outs() << "Variable declarations must have an init value.\n";
            return true;
        }

        if (DC->isAutoTyped()) {
            llvm::outs() << "Automatically type evalueated Decl must have a value.\n";
            return true;
        }

        if (resolveTypeInfo(DC->getTypeInfo(), scope))
            return true;

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

        // FIXME: Maybe check if the init is a CallExpr and validate that it isn't void
        
        DC->getTypeInfo()->setType(DC->getInit()->getType());
        return false;
    } else if (resolveTypeInfo(DC->getTypeInfo(), scope))
        return true;

    if (DC->getInit()->isTypeDependant()) {
        // FIXME: Implement
        llvm_unreachable("Currently no type dependant expressions (i think).");
    } else if (validateTypeCast(DC->getInit()->getType(), DC->getType(), false)) {
        llvm::outs() << "Invalid type conversion\n";
        return true;
    }

    // To assure both have the same type in codegen
    DC->getInit()->setType(DC->getType());
    return false;
}

bool Sema::analyzeFuncionDecl(FunctionDecl* DC, Scope* scope) {
    DEBUG("Called: analyzeFunctionDecl");
    if (lookup(scope, DC->getName(), true).findKind(DC->getKind())) {
        llvm::outs() << "A function called '" << DC->getName().str() << "' already exists.\n";
        return true;
    }

    // Push DC at the start. Functions can be called recursively.
    scope->pushDecl(DC);
    
    Scope proto(scope, Scope::FnPrototypeScope);
    proto.setFnPrototype(&proto);
    // proto.setEntity(DC);

    if (DC->hasParams())
        for (ParamDecl* param : DC->parameters()) {
            if (analyzeInitDecl(param, &proto))
                return true;
            proto.pushDecl(param);
        }

    if (!DC->isVoid() && resolveTypeInfo(DC->getTypeInfo(), &proto))
        return true;
        
    CurrentFunction = DC;
    if (analyzeBlockStmt(DC->getBody(), &proto))
        return true;

    return false;
}

bool Sema::validateTypeDefName(llvm::StringRef name, Scope* scope) {
    DEBUG("Called: validateTypeDefName");
    auto builtins = lexer::getBuiltinMap();
    if (builtins.find(name) != builtins.end()) {
        llvm::outs() << "Type declration name conflicts with builtin type.\n";
        return true;
    }

    if (lookup(scope, name, false, Lookup::Type).find()) {
        llvm::outs() << "A type called '" << name.str() << "' already exists.\n";
        return true;
    }

    return false;
}

bool Sema::analyzeTypeAliasDecl(TypeAliasDecl* DC, Scope* scope) {
    DEBUG("Called: analyzeTypeAliasDecl");
    if (validateTypeDefName(DC->getName(), scope))
        return true;

    if (resolveTypeInfo(DC->getTypeInfo(), scope))
        return true;

    DC->setDeclType(DC->getTypeInfo()->getType());
    scope->pushDecl(DC);
    return false;
}

bool Sema::analyzeStructDecl(StructDecl* DC, Scope* scope) {
    DEBUG("Called: analyzeStructDecl");
    if (validateTypeDefName(DC->getName(), scope))
        return true;

    Scope body(scope, Scope::StructScope);
    // body.setEntity(DC);

    for (Decl* d : DC->decls()) {
        DEBUG("loop struct start");
        if (auto FD = llvm::dyn_cast<FieldDecl>(d)) {
            if (analyzeInitDecl(FD, &body))
                return true;
        } else if (auto TD = llvm::dyn_cast<TypeAliasDecl>(d)) {
            if (analyzeTypeAliasDecl(TD, &body))
                return true;
        } else {
            llvm::outs() << "Only field declarations and type aliases are allowed in struct declarations.\n";
            return true;
        }

        // Can only be FieldDecl or TypeAliasDecl, which are both NamedDecl.
        body.pushDecl(llvm::cast<NamedDecl>(d));
        DEBUG("loop struct end");
    }

    DC->setDeclType(Alloc.Create<StructType>(DC));
    scope->pushDecl(DC);
    return false;
}

bool Sema::analyzeBlockStmt(BlockStmt* ST, Scope* scope) {
    DEBUG("Called: analyzeBlockStmt");
    // BlockStmt can only be within the scope of a function. There's currently 
    // also no flags for different scopes within a function body so therefore 
    // we just hardcode the flag to be FunctionScope.
    assert(scope->getFnPrototype() != nullptr);
    Scope body(scope, Scope::FunctionScope);
    body.setFnPrototype(scope->getFnPrototype());

    for (Stmt* s : ST->stmts()) {
        if (auto DC = llvm::dyn_cast<DeclStmt>(s)) {
            if (!llvm::isa<VarDecl>(DC->getDecl())) {
                llvm::outs() << "Only variable declarations are allowed inside the body of a function.\n";
                return true;
            }

            if (analyzeDecl(DC->getDecl(), &body))
                return true;
        } else if (auto EX = llvm::dyn_cast<Expr>(s)) {
            if (analyzeExpr(EX, &body, true))
                return true;
        } else if (analyzeStmt(s, &body))
            return true;
    }

    return false;
}

bool Sema::analyzeReturnStmt(ReturnStmt* ST, Scope* scope) {
    DEBUG("Called: analyzeReturnStmt");
    assert(CurrentFunction != nullptr);
    if (ST->isVoid()) {
        if (CurrentFunction->isVoid())
            return false;
        
        llvm::outs() << "Can't have a void return in a non void function.\n";
        return true;
    } else if (CurrentFunction->isVoid()) {
        llvm::outs() << "Return value can't be void for functuion '" << CurrentFunction->getName() << ".\n";
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

bool Sema::analyzeConditionalStmtBody(Stmt* body, Scope* scope) {
    DEBUG("Called: analyzeConditionalStmtBody");
    // TODO: Add checks to check that the body acually does somthing.
    // i.e isn't just somthing like a variable declarations that'll 
    // just immediatly get destroyed.

    if (auto EX = llvm::dyn_cast<Expr>(body)) {
        if (analyzeExpr(EX, scope, true))
            return true;

        return false;
    }

    if (analyzeStmt(body, scope))
        return true;

    return false;
}

bool Sema::analyzeIfStmt(IfStmt* ST, Scope* scope) {
    DEBUG("Called: analyzeIfStmt");
    Scope if_scope(scope, Scope::FunctionScope);
    if_scope.setFnPrototype(scope->getFnPrototype());

    IfStmt* current = ST;
    while (true) {
        if (analyzeCondition(current->getCondition(), &if_scope))
            return true;

        if (analyzeConditionalStmtBody(current->getBody(), &if_scope))
            return true;

        if (!current->hasElse())
            break;
        
        auto next = llvm::dyn_cast<IfStmt>(current->getElse());
        if (next == nullptr) {
            if (analyzeConditionalStmtBody(current->getElse(), &if_scope))
                return true;
            break;
        }

        current = next;
    }

    return false;
}

bool Sema::analyzeWhileStmt(WhileStmt* ST, Scope* scope) {
    DEBUG("Called: analyzeWhileStmt");
    Scope while_scope(scope, Scope::FunctionScope);
    while_scope.setFnPrototype(scope->getFnPrototype());

    if (analyzeCondition(ST->getCondition(), scope))
        return true;

    if (analyzeConditionalStmtBody(ST->getBody(), &while_scope))
        return true;

    return false;
}

bool Sema::analyzeCondition(Expr* EX, Scope* scope) {
    DEBUG("Called: analyzeCondition");
    // Assigning is not allowed inside of conditions
    if (analyzeExpr(EX, scope, false))
        return true;
    
    // Can only be false for arithmetic and non-binary expressions
    if (validateTypeCast(EX->getType(), Alloc.getBuiltinType(BuiltinType::Bool), false)) {
        llvm::outs() << "Invalid type conversion to bool inside condition.\n";
        return false;
    }

    return false;
}

bool Sema::validateExprIsMutable(Expr* EX) {
    DEBUG("Called: validateExprIsMutable");
    assert(EX->isPlace() && "Check if the Expression is an LValue before calling valudateExprIsMutable.\n");
    if (auto ref = llvm::dyn_cast<DeclRefExpr>(EX)) {
        // Will always be either VarDecl or ParamDecl
        bool is_mutable;
        if (auto VD = llvm::dyn_cast<VarDecl>(ref->getDecl())) {
            is_mutable =  VD->isMutable();
        } else if (auto PD = llvm::dyn_cast<ParamDecl>(ref->getDecl())) {
            is_mutable = PD->isMutable();
        } else llvm_unreachable("");

        if (!is_mutable) {
            llvm::outs() << "Can't assign to an immutable value.\n";
            return true;
        }

        return false;
    } else if (llvm::isa<UnaryOperator>(EX)) {
        return validateExprIsMutable(llvm::cast<UnaryOperator>(EX)->getSubExpr());
    } else if (llvm::isa<QualExpr>(EX)) { // AccessExpr and SliceExpr
        return validateExprIsMutable(llvm::cast<QualExpr>(EX)->getBase());
    }

    llvm_unreachable("");
}

bool Sema::analyzeBinaryAssign(BinaryOperator* EX, Scope* scope) {
    DEBUG("Called: analyzeBinaryAssign");
    if (!EX->getLHS()->isPlace()) {
        llvm::outs() << "Can only assign to an identifiable memory location.\n";
        return true;
    }

    if (analyzeExpr(EX->getLHS(), scope, false))
        return true;
    
    // We allow assignment here to allow expressions like "a = b = c"
    if (analyzeExpr(EX->getRHS(), scope, true))
        return true;

    if (!EX->getLHS()->isMutablePlace()) {
        llvm::outs() << "Memory location is not assignable.\n";
        return true;
    }

    if (validateTypeCast(EX->getRHS()->getType(), EX->getLHS()->getType(), false)) {
        llvm::outs() << "Invalid type conversion in assignment expression.\n";
        return true;
    }

    // Set operator type and assure both sides have the same type in codegen
    EX->setType(EX->getLHS()->getType());
    EX->getRHS()->setType(EX->getType());
    return false;
}

bool Sema::analyzeBinaryOperator(BinaryOperator* EX, Scope* scope) {
    DEBUG("Called: analyzeBinaryOperator");
    // Assigning is not allowed in conditional and arithmetic binary operators
    if (analyzeExpr(EX->getLHS(), scope, false))
        return true;
    if (analyzeExpr(EX->getRHS(), scope, false))
        return true;

    if (validateTypeCast(EX->getRHS()->getType(), EX->getRHS()->getType(), false)) {
        llvm::outs() << "Invalid type conversion.\n";
        return true;
    }

    // FIXME: Take the type with the most (i think) bytes instead
    EX->getRHS()->setType(EX->getLHS()->getType());
 
    if (EX->isComparisonOp()) {
        EX->setType(Alloc.getBooleanType());
    } else { // Arithmetic operator
        if (EX->getLHS()->getType()->isStructTy()) {
            llvm::outs() << "Can't perform arithmetic on a struct type.\n";
            return true;
        }

        // Set type of operator and assure both sides have the same type for codegen
        EX->setType(EX->getLHS()->getType());
        EX->getRHS()->setType(EX->getType());
    }

    return false;
}

bool Sema::analyzeDereference(UnaryOperator* EX, Scope* scope) {
    DEBUG("Called: analyzeDereference");
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

bool Sema::analyzeUnaryAssign(UnaryOperator* EX, Scope* scope) {
    DEBUG("Called: analyzeUnarAssign");
    if (analyzeExpr(EX->getSubExpr(), scope, false))
        return true;

    if (!EX->getSubExpr()->isMutablePlace()) {
        llvm::outs() << "Memory location is not assignable.\n";
        return true;
    }
    
    EX->setType(EX->getSubExpr()->getType());

    // TODO: This will have to be changed when operator methods are implemented.
    auto int_ty = Alloc.getBuiltinType(BuiltinType::U64); // FIXME: Don't know which int type to pick...
    if (validateTypeCast(EX->getType(), int_ty, false)) {
        llvm::outs() << "'++' and '--' operators not valid for expression.\n";
        return true;
    }

    return false;
}

bool Sema::analyzeUnaryOperator(UnaryOperator* EX, Scope* scope) {
    DEBUG("Called: analyzeUnaryOperator");
    if (analyzeExpr(EX->getSubExpr(), scope, false))
        return true;

    if (EX->isAdressOp()) {
        if (auto DC = llvm::dyn_cast<DeclRefExpr>(EX)) {
            if (!llvm::isa<ValueDecl>(DC->getDecl())) {
                llvm::outs() << "Can only get a pointer to a value or a function.\n";
                return true;
            } else if (llvm::isa<FunctionDecl>(DC->getDecl())) {
                // TODO: Remove this else statement when function pointers are implemented
                llvm::outs() << "Function pointers are not yet supported.\n";
                return true;
            }
        }
        EX->setType(Alloc.Create<PointerType>(
            EX->isRawPtrOp(), EX->getSubExpr()->getType()
        ));
    } if (EX->isNegateOp()) {
        auto bool_ty = Alloc.getBuiltinType(BuiltinType::Bool);
        if (validateTypeCast(EX->getType(), bool_ty, false)) {
            llvm::outs() << "Type is not bool convertable.\n";
            return true;
        }

        EX->setType(bool_ty);
    } else if (EX->isArithmeticOp()) {
        auto int_ty = Alloc.getBuiltinType(BuiltinType::U64); // FIXME: Don't know which int type to pick...
        if (validateTypeCast(EX->getSubExpr()->getType(), int_ty, false)) {
            llvm::outs() << "'+' and '-' prefixes are only valid for numbers.\n";
            return true;
        }

        EX->setType(EX->getSubExpr()->getType());
    } else
        // Probably means you've called the wrong function.
        llvm_unreachable("Invalid unary operator.");

    return false;
}

bool Sema::analyzeDeclRef(DeclRefExpr* EX, Scope* scope) {
    DEBUG("Called: analyzeDeclRef");

    auto result = lookup(scope, EX->getName(), false, Lookup::Value).find().getAsSingle();
    if (!result) {
        llvm::outs() << "Could not find a Decl called '" << EX->getName() << "'\n";
        return true;
    }

    if (auto DC = llvm::dyn_cast<InitDecl>(result)) {
        EX->setType(DC->getType());
    } else 
        // TODO: Change when function pointers are implemented
        llvm_unreachable("Function pointers are currently not supported in Sema");

    EX->setDecl(result);
    return false;
}

bool Sema::analyzeSliceExpr(SliceExpr* EX, Scope* scope) {
    DEBUG("Called: analyzeSliceExpr");
    if (analyzeExpr(EX->getBase(), scope, false))
        return true;

    if (!EX->getBase()->getType()->isArrayTy()) {
        llvm::outs() << "Must be array type to access an index.\n";
        return true;
    }

    // TODO: Should eventually support python-like slicing syntax.

    if (analyzeExpr(EX->getStart(), scope, false))
        return true;

    auto int_ty = Alloc.getBuiltinType(BuiltinType::I64); // FIXME: Don't know which int type to pick...
    if (validateTypeCast(EX->getStart()->getType(), int_ty, false)) {
        llvm::outs() << "Array intdex must be convertable to an integer.\n";
        return true;
    }

    EX->setType(llvm::cast<ArrayType>(EX->getBase()->getType())->getElemType());
    return false;
}

bool Sema::analyzeAccessExpr(AccessExpr* EX, Scope* scope) {
    DEBUG("Called: analyzeAccessExpr");
    if (analyzeExpr(EX->getBase(), scope, false))
        return true;

    if (!EX->getBase()->getType()->isStructTy()) {
        llvm::outs() << "Only struct types have attributes.\n";
        return true;
    }

    auto type = llvm::cast<StructType>(EX->getBase()->getType());
    for (FieldDecl* FD : type->getDecl()->fields())
        if (FD->getName() == EX->getFieldName()) {
            EX->setType(FD->getType());
            EX->setFieldDecl(FD);
            return false;
        }

    llvm::outs() << "Struct has not attribute called '" << EX->getFieldName() << "'.\n";
    return true;
}

bool Sema::analyzeCallExpr(CallExpr* EX, Scope* scope) {
    DEBUG("Called: analyzeCallExpr");
    // TODO: Find a better way of doing this. This if statement is very ugly.
    llvm::SmallVector<InitDecl*> params;
    if (auto fn_result = lookup(scope, EX->getCallee()).findKind(Decl::FunctionDeclKind)) {
        auto result = fn_result.getAsSingle<FunctionDecl>();
        params = llvm::SmallVector<InitDecl*>(result->parameters());
        EX->setCalleeDecl(result);
        EX->setType(result->getType());
    } else if (auto st_result = lookup(scope, EX->getCallee()).findKind(Decl::StructDeclKind)) {
        auto result = st_result.getAsSingle<StructDecl>();
        params = llvm::SmallVector<InitDecl*>(result->fields());
        EX->setCalleeDecl(result);
        EX->setType(result->getDeclType());
    } else {
        llvm::outs() << "No function or struct called '" << EX->getCallee() << "' was found.\n";
        return true;
    }
    
    for (size_t i = 0; i < params.size(); i++) {
        if (i < EX->getAmntArgs()) {
            Expr* arg = EX->getArg(i);
            // TODO: Condiser adding logic to create a Scope with a flag for 
            // conditions and calls since both disallow assignment, this to 
            // differentiate them in error messages.
            if (analyzeExpr(arg, scope, false))
                return true;

            auto param = params[i];
            if (validateTypeCast(arg->getType(), param->getType(), false)) {
                llvm::outs() << "Invalid type conversion.\n";
                return true;
            }

            // Assure the arg is the same type as the param for codegen
            arg->setType(param->getType());
        } else {
            llvm::outs() << "Mismatched call to .\n";
            return true;
        }
    }

    return false;
}

bool ArrayItemIsConstant(const Expr* EX) {
    if (auto call = llvm::dyn_cast<CallExpr>(EX)) {
        if (!llvm::isa<StructDecl>(call->getCalleeDecl()))
            return false;

        for (const Expr* it : call->arguments())
            if (!ArrayItemIsConstant(it))
                return false;
    } else if (auto ref = llvm::dyn_cast<DeclRefExpr>(EX)) {
        if (auto DC = llvm::dyn_cast<VarDecl>(ref->getDecl()))
            if (!DC->isConstant())
                return false;
    } else if (!EX->isLiteralExpr())
        return false;
    
    return true;
}

bool Sema::analyzeArrayLiteral(ArrayLiteral* EX, Scope* scope) {
    const Type* ty = nullptr;
    bool all_constant = true;
    for (Expr* it : EX->items()) {
        if (analyzeExpr(it, scope, false))
            return true;

        if (ty == nullptr) {
            ty = it->getType();
        } else if (validateTypeCast(it->getType(), ty, false)) {
            llvm::outs() << "All items in the array must be of the same type.\n";
            return true;
        }
        
        it->setType(ty);
        if (all_constant)
            all_constant = ArrayItemIsConstant(it);
    }

    EX->setType(Alloc.Create<ArrayType>(ty, EX->getAmntItems()));
    return false;
}

} // namespace c
