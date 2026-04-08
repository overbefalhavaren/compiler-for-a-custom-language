#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/Type.hpp"
#include "include/AST/TypeInfo.hpp"
#include "include/Frontend/ASTAllocator.hpp"
#include "include/IO/SrcSpan.hpp"
#include "include/Lexer/Lexer.hpp"

namespace c {
namespace ast {

class Parser {
private:
    // Binding power for pratt parsing
    struct BindingPower {
        uint8_t lbp; // Left binding power
        uint8_t rbp; // Right binding power
    };

    struct InitDeclInfo {
        llvm::StringRef name;
        TypeInfo* type;
        Expr* init;

        // This would've just worked anyway in C
        // but this is C++ so here we are...
        InitDeclInfo() = default;
        InitDeclInfo(llvm::StringRef name, TypeInfo* type, Expr* init)
            : name(name), type(type), init(init) {}

        operator bool() {
            return name.data() && type && init;
        }
    };

    Lexer LexerSource;
    Token CurrentToken;
    ASTAllocator& Alloc;
public:
    Parser(ASTAllocator& alloc, Lexer&& lexer)
        : LexerSource(std::move(lexer)), Alloc(alloc) {
        (void)nextToken();
    }
    ~Parser() = default;

    bool parseSourceFile(ast::ModuleDecl& result) {
        if (peekToken().is(TokenType::Eof))
            return true;

        do {
            ast::Decl* next = parseNextTopLevel();
            if (!next) return true;

            result.pushDecl(next);
        } while (!peekToken().is(TokenType::Eof));
        return false;
    }
private:
    const Token& peekToken() const {
        return CurrentToken;
    }

    const Token& nextToken() {
        CurrentToken = LexerSource.next();
        return CurrentToken;
    }

    llvm::APInt stringToAPInt(llvm::StringRef numberLiteral) {
        // FIXME: Implement
        llvm_unreachable("Implement this function.");
    }

    llvm::APFloat stringToAPFloat(llvm::StringRef floatLiteral) {
        // FIXME: Implement
        llvm_unreachable("Implement this function.");
    }

    ast::BinaryOperator::OpKind getBinaryOperator(TokenType type) {
        switch (type) {
            default:
                llvm_unreachable(""); // FIXME: Add message

            case TokenType::Plus:
                return ast::BinaryOperator::OpKind::Add;
            case TokenType::Minus:
                return ast::BinaryOperator::OpKind::Sub;
            case TokenType::Star:
                return ast::BinaryOperator::OpKind::Mul;
            case TokenType::Slash:
                return ast::BinaryOperator::OpKind::Div;
            
            case TokenType::Equal:
                return ast::BinaryOperator::OpKind::Assign;
            case TokenType::PlusEqual:
                return ast::BinaryOperator::OpKind::AddAssign;
            case TokenType::MinusEqual:
                return ast::BinaryOperator::OpKind::SubAssign;
            case TokenType::StarEqual:
                return ast::BinaryOperator::OpKind::MulAssign;
            case TokenType::SlashEqual:
                return ast::BinaryOperator::OpKind::DivAssign;
            
            case TokenType::EqualEqual:
                return ast::BinaryOperator::OpKind::Equal;
            case TokenType::ExclEqual:
                return ast::BinaryOperator::OpKind::NotEqual;
            case TokenType::More:
                return ast::BinaryOperator::OpKind::MoreThan;
            case TokenType::Less:
                return ast::BinaryOperator::OpKind::LessThan;
            case TokenType::MoreEqual:
                return ast::BinaryOperator::OpKind::MTEqual;
            case TokenType::LessEqual:
                return ast::BinaryOperator::OpKind::LTEqual;

            case TokenType::AmpAmp:
                return ast::BinaryOperator::OpKind::And;
            case TokenType::PipePipe:
                return ast::BinaryOperator::OpKind::Or;
        }
    }
       
    ast::UnaryOperator::OpKind getUnaryOperator(TokenType type) {
        switch (type) {
            default:
                llvm_unreachable(""); // FIXME: Add message

            case TokenType::Star:
                return ast::UnaryOperator::OpKind::Deref;
            case TokenType::Ampersand:
                return ast::UnaryOperator::OpKind::AdressOf;
            case TokenType::PlusPlus:
                return ast::UnaryOperator::OpKind::AddOne;
            case TokenType::MinusMinus:
                return ast::UnaryOperator::OpKind::SubOne;
            case TokenType::Plus:
                return ast::UnaryOperator::OpKind::Plus;
            case TokenType::Minus:
                return ast::UnaryOperator::OpKind::Minus;
            case TokenType::Exclamation:
                return ast::UnaryOperator::OpKind::Not;
        }
    }

    ast::Decl* parseNextTopLevel() {
        switch (peekToken().getType()) {
            default:
                llvm::outs() << "Only variable, type, struct, impl and function declarations"
                                " are allowed in the global namespace.\n";
                llvm::outs() << "Currently: " << strTokenType(peekToken().getType()) << "\n"; 
                return nullptr;

            case TokenType::Type:
                return parseTypeAliasDecl();

            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
                return parseVarDecl();

            case TokenType::Struct:
                return parseStructDecl();

            case TokenType::Fn:
                return parseFunctionDecl();
        }
    }

    ast::Stmt* parseNextStmt() {
        switch (peekToken().getType()) {
            default:
                llvm::outs() << "Unexpected token: '" << strTokenType(peekToken().getType()) << "'.\n";
                return nullptr;

            case TokenType::Eof:
                llvm::outs() << "Unclosed Scope.\n";
                return nullptr;

            case TokenType::Type:
                ast::TypeAliasDecl* dc = parseTypeAliasDecl();
                return Alloc.Create<ast::DeclStmt>(dc->getSpan(), dc); 

            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
                ast::VarDecl* dc = parseVarDecl();
                return Alloc.Create<ast::DeclStmt>(dc->getSpan(), dc);

            case TokenType::If:
                return parseIfStmt();
            case TokenType::While:
                return parseWhileStmt();
            case TokenType::Return:
                return parseReturnStmt();
        }
    }

    bool isPrefixUnaryOp(TokenType t) {
        return t == TokenType::Exclamation ||   // Not
               t == TokenType::Plus ||          // Positive number
               t == TokenType::Minus ||         // Negative number
               t == TokenType::Star ||          // Get pointer value
               t == TokenType::Ampersand;       // Get adress
    }

    bool isSuffixUnaryOp(TokenType t) {
        return t == TokenType::PlusPlus || t == TokenType::MinusMinus;
    }

    BindingPower getBindingPower(TokenType op) {
        switch (op) {
            default:
                return BindingPower(0, 0);

            case TokenType::Star:
            case TokenType::Slash:
                return {70, 71};

            case TokenType::Plus:
            case TokenType::Minus:
                return {60, 61};

            case TokenType::Less:
            case TokenType::More:
            case TokenType::LessEqual:
            case TokenType::MoreEqual:
                return {50, 51};

            case TokenType::EqualEqual:
            case TokenType::ExclEqual:
                return {45, 46};

            case TokenType::AmpAmp:
                return {30, 31};
            case TokenType::PipePipe:
                return {20, 21};

            // Assignment (right associative)
            case TokenType::Equal:
            case TokenType::PlusEqual:
            case TokenType::MinusEqual:
            case TokenType::StarEqual:
            case TokenType::SlashEqual:
                return {10, 9};
        }
    }

    ast::Expr* parsePrefix() {
        Token start = peekToken();
        (void)nextToken();

        if (isPrefixUnaryOp(start.getType())) {
            ast::Expr* rhs = parseExpr(100);
            if (!rhs) return nullptr;

            SrcSpan span(start.getStartLoc(), rhs->getEndLoc());
            return Alloc.Create<ast::UnaryOperator>(
                span, getUnaryOperator(start.getType()), rhs
            );
        }

        switch (start.getType()) {
            default:
                llvm::outs() << "Invalid token: " << strTokenType(start.getType()) << ".\n";
                return nullptr;

            case TokenType::Int:
                return Alloc.Create<ast::IntegerLiteral>(
                    peekToken().getSpan(), stringToAPInt(peekToken().getData())
                );
            case TokenType::Float:
                return Alloc.Create<ast::FloatingLiteral>(
                    peekToken().getSpan(), stringToAPFloat(peekToken().getData())
                );
            case TokenType::LParen: {
                ast::Expr* result = parseExpr(0);
                if (peekToken().isNot(TokenType::RParen)) {
                    llvm::outs() << "Unclosed '('.\n";
                    return nullptr;
                }

                (void)nextToken();
                return result;
            }
            case TokenType::Identifier: {
                if (peekToken().isNot(TokenType::LParen)) {
                    return Alloc.Create<ast::VarDeclRef>(
                        start.getSpan(), start.getData()
                    );
                }

                llvm::SmallVector<Expr*> args;
                if (nextToken().isNot(TokenType::RParen))
                    while (true) {
                        if (peekToken().is(TokenType::Eof)) {
                            llvm::outs() << "Unclosed function call. Expected ')'.\n";
                            return nullptr;
                        }

                        Expr* next = parseExpr();
                        if (!next) return nullptr;

                        args.push_back(next);

                        if (peekToken().is(TokenType::RParen)) {
                            break;
                        } else if (peekToken().is(TokenType::Comma)) 
                            if (nextToken().is(TokenType::RParen)) {
                                llvm::outs() << "Unnessesary ','.\n";
                                return nullptr;
                            }
                        
                        (void)nextToken();
                    }
                
                SrcSpan span(start.getStartLoc(), peekToken().getEndLoc());
                CallExpr* result = Alloc.Create<ast::CallExpr>(span, start.getData());
                result->setArguments(std::move(args));

                (void)nextToken();
                return result;
            }
        }
    }

    ast::Expr* parseExpr(int8_t minBP = 0) {
        ast::Expr* lhs = parsePrefix();
        if (!lhs) return nullptr;

        while (true) {
            if (!peekToken().isOperator()) {
                break;
            } else if (isSuffixUnaryOp(peekToken().getType())) {
                SrcSpan span(lhs->getStartLoc(), peekToken().getEndLoc());
                lhs = Alloc.Create<ast::UnaryOperator>(
                    span, getUnaryOperator(peekToken().getType()), lhs
                );

                (void)nextToken();
                continue;
            }

            TokenType op = peekToken().getType();
            BindingPower bp = getBindingPower(op);
            if (bp.lbp < minBP)
                break;

            (void)nextToken();
            ast::Expr* rhs = parseExpr(bp.rbp);
            if (!rhs) return nullptr;

            SrcSpan span(lhs->getStartLoc(), rhs->getStartLoc());
            lhs = Alloc.Create<ast::BinaryOperator>(
                span, getBinaryOperator(op), lhs, rhs
            );
        }

        return lhs;
    }

    ast::TypeInfo* parseTypeRef() {
        if (peekToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }

        return Alloc.Create<ast::TypeInfo>(ast::TypeInfo::CreateNamed(
            peekToken().getSpan(), peekToken().getData()
        ));
    }

    ast::TypeAliasDecl* parseTypeAliasDecl() {
        // The first token will always be TokenType::Type
        SrcLoc start = peekToken().getStartLoc();
        if (nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = peekToken().getData();

        if (nextToken().isNot(TokenType::Equal)) {
            llvm::outs() << "Expected '='.\n";
            return nullptr;
        }

        ast::TypeInfo* type = parseTypeRef();
        if (!type) return nullptr;
        
        (void)nextToken();
        SrcSpan span(start, type->getEndLoc());
        return Alloc.Create<ast::TypeAliasDecl>(span, name, type);
    }

    // FIXME: Init value should only be required for variables
    InitDeclInfo parseInitDecl() {
        if (peekToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return {};
        }
        llvm::StringRef name = peekToken().getData();

        ast::TypeInfo* type = nullptr;
        if (nextToken().is(TokenType::Colon)) {
            (void)nextToken();
            type = parseTypeRef();
            if (!type) return {};

            if (peekToken().isNot(TokenType::Equal)) {
                llvm::outs() << "Expected '='.\n";
                return {};
            }
        } else if (peekToken().isNot(TokenType::Equal)) {
            llvm::outs() << "Expected ':' or '='.\n";
            return {};
        }

        (void)nextToken();
        ast::Expr* init = parseExpr();
        if (!init) return {};
        
        return InitDeclInfo(name, type, init);
    }

    ast::VarDecl* parseVarDecl() {
        SrcLoc start = peekToken().getStartLoc();
        ast::VarDecl::Mutability mut;
        switch (peekToken().getType()) {
            default:
                llvm_unreachable(""); // FIXME: Add message
            
            case TokenType::Const:
                mut = VarDecl::Constant;
                break;
            case TokenType::Let:
                mut = VarDecl::Immutable;
                break;
            case TokenType::Mut:
                mut = VarDecl::Mutable;
                break;
        }

        (void)nextToken();
        InitDeclInfo info = parseInitDecl();
        if (!info) return nullptr;



        SrcSpan span(start, info.init->getEndLoc());
        return Alloc.Create<ast::VarDecl>(span, info.name, mut, info.type, info.init);
    }

    ast::ParamDecl* parseParamDecl() {
        SrcLoc start = peekToken().getStartLoc();
        ast::ParamDecl::Mutability mut = ast::ParamDecl::ImmutableReference;
        if (nextToken().isNot(TokenType::Identifier)) {
            if (peekToken().is(TokenType::Mut)) {
                mut = ast::ParamDecl::MutableReference;
            } else if (peekToken().is(TokenType::Move)) {
                mut = ast::ParamDecl::MovedValue;
            } else
                llvm_unreachable(""); // FIXME: Add message
        }

        InitDeclInfo info = parseInitDecl();
        if (!info) return nullptr;

        SrcSpan span(start, info.init->getEndLoc());
        return Alloc.Create<ast::ParamDecl>(span, info.name, mut, info.type, info.init);
    }

    ast::FieldDecl* parseFieldDecl(size_t idx) {
        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken();
        InitDeclInfo info = parseInitDecl();
        if (!info) return nullptr;

        SrcSpan span(start, peekToken().getEndLoc());
        return Alloc.Create<FieldDecl>(span, info.name, idx, info.type, info.init);
    }

    ast::StructDecl* parseStructDecl() {
        SrcLoc start = peekToken().getStartLoc();
        if (nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = peekToken().getData();

        if (nextToken().isNot(TokenType::Colon)) {
            llvm::outs() << "Expected ':'.\n";
            return nullptr;
        }

        ast::StructDecl decl(name);
        llvm::SmallVector<FieldDecl*> fields;
        while (nextToken().isNot(TokenType::End)) {
            if (peekToken().is(TokenType::Eof)) {
                llvm::outs() << "Unclosed struct declaration. Expected 'end'.\n";
                return nullptr;
            }

            FieldDecl* next = parseFieldDecl(fields.size());
            if (!next) return nullptr;

            fields.push_back(next);
            decl.pushDecl(next);
        }

        decl.setFields(std::move(fields));
        decl.setSpan(SrcSpan(std::move(start), peekToken().getEndLoc()));

        (void)nextToken();
        return Alloc.Create<StructDecl>(std::move(decl));
    }

    ast::FunctionDecl* parseFunctionDecl() {
        // The first token will always be TokenType::Fn 
        SrcLoc start = peekToken().getStartLoc();
        if (nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = peekToken().getData();
        
        if (nextToken().isNot(TokenType::LParen)) {
            llvm::outs() << "Expected '('.\n";
            return nullptr;
        }

        ast::FunctionDecl decl(name);
        llvm::SmallVector<ParamDecl*> params;
        if (nextToken().isNot(TokenType::RParen))
            while (true) {
                if (peekToken().is(TokenType::Eof)) {
                    llvm::outs() << "Unclosed function declaration. Expected ')'.\n";
                    return nullptr;
                }

                ParamDecl* next = parseParamDecl();
                if (!next) return nullptr;

                params.push_back(next);
                decl.pushDecl(next);

                if (peekToken().is(TokenType::RParen)) {
                    break;
                } else if (peekToken().is(TokenType::Comma)) 
                    if (nextToken().is(TokenType::RParen)) {
                        llvm::outs() << "Unnessesary ','.\n";
                        return nullptr;
                    }
                
                (void)nextToken();
            }

        // Ends the function declaration at the closing ")" in case
        // in case the function doesn't have a return type.
        SrcLoc end = peekToken().getEndLoc();

        if (nextToken().is(TokenType::Arrow)) {
            (void)nextToken();
            ast::TypeInfo* type = parseTypeRef();
            if (!type) return nullptr;

            end = peekToken().getEndLoc();
            decl.setTypeInfo(type);
        } else if (peekToken().isNot(TokenType::Colon)) {
            llvm::outs() << "Expected '->' or ':'.\n";
            return nullptr;
        }

        decl.setSpan(SrcSpan(std::move(start), std::move(end)));

        // Check if a scope for the function is created here since
        // abstract functions don't have scopes and calling 
        // parseBlockStmt will therefore cause an error message
        // even though there shouldn't be one.
        if (peekToken().is(TokenType::Colon)) {
            ast::BlockStmt* body = parseBlockStmt();
            if (!body) return nullptr;

            decl.setBody(std::move(body));
        }

        return Alloc.Create<ast::FunctionDecl>(std::move(decl));
    }

    ast::BlockStmt* parseBlockStmt() {
        SrcLoc start = peekToken().getStartLoc();

        if (peekToken().is(TokenType::Colon)) {
            llvm::outs() << "Expected ':'.\n";
            return nullptr;
        }

        (void)nextToken();
        llvm::SmallVector<ast::Stmt*> stmts;
        while (peekToken().isNot(TokenType::End)) {
            if (peekToken().is(TokenType::Eof)) {
                llvm::outs() << "Unclosed ':'\n";
                return nullptr;
            }

            ast::Stmt* next = parseNextStmt();
            if (!next) return nullptr;

            stmts.push_back(next);
        }

        SrcSpan span(start, peekToken().getEndLoc());

        (void)nextToken();
        return Alloc.Create<ast::BlockStmt>(span, std::move(stmts));
    }

    ast::Stmt* parseStmtBody() {
        if (peekToken().is(TokenType::Colon))
            return parseBlockStmt();
        return parseNextStmt();
    }

    ast::IfStmt* parseIfStmt() {
        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip leading If token
        ast::Expr* cond = parseExpr();
        if (!cond) return nullptr;

        SrcSpan span(start, peekToken().getEndLoc());

        ast::Stmt* body = parseStmtBody();
        if (!body) return nullptr;

        // else_stmt will be IfStmt* if there's an elif
        // eles_stmt will be BlockStmt* or Stmt* if there's and else statement
        ast::Stmt* else_stmt = nullptr;
        if (peekToken().is(TokenType::Elif)) {
            else_stmt = parseIfStmt();
            if (!else_stmt) return nullptr;
        } else if (peekToken().is(TokenType::Else)) {
            (void)nextToken(); // Skip over the leading Else token
            else_stmt = parseStmtBody();
            if (!else_stmt) return nullptr;
        }

        return Alloc.Create<ast::IfStmt>(span, cond, body, else_stmt);
    }

    ast::WhileStmt* parseWhileStmt() {
        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip leading "While" token
        ast::Expr* cond = parseExpr();
        if (!cond) return nullptr;

        SrcSpan span(start, peekToken().getEndLoc());

        ast::Stmt* body = parseStmtBody();
        if (!body) return nullptr;

        return Alloc.Create<ast::WhileStmt>(span, cond, body);
    }

    ast::ReturnStmt* parseReturnStmt() {
        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip the leading Else token
        ast::Expr* value = parseExpr();

        SrcSpan span(start, value->getEndLoc());
        return Alloc.Create<ast::ReturnStmt>(span, value);
    }
};

} // namespace ast
} // namespace c
