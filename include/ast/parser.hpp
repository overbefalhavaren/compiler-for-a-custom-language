#pragma once

#include <optional>

#include "llvm/ADT/SmallVector.h"

#include "include/lexer/Lexer.hpp"
#include "include/lexer/Token.hpp"
#include "include/ast/Stmt.hpp"
#include "include/ast/Expr.hpp"
#include "include/ast/Module.hpp"

namespace c {

// FIXME:
// All things that have "arguments", i.e a list of statements like for example
// arguments for a function or parameters in call expressions or types in a 
// templated type have the problem(?) that a comma before the closing syntax is
// allowed. Might want to look into dissallowing this. Example:
// test(t, 1, 2,) // Here you see you can put a "," before the closing syntax ")"
class Parser {
private:
    struct BindingPower {
        uint8_t lbp; // Left binding power
        uint8_t rbp; // Right binding power
    };

    Lexer LexerSource;
    Token CurrentToken;
public:
    Parser(Lexer&& lexer) 
        : LexerSource(std::move(lexer)) {
        // CurrentToken = lexer.next();
        (void)nextToken();
    }
    ~Parser() = default;

    bool parseSourceFile(Module* result) {
        if (peekToken().is(TokenType::Eof))
            return true;
 
        llvm::SmallVector<std::unique_ptr<ast::Stmt>> stmts;
        do {
            llvm::outs() << "loop\n";
            std::unique_ptr<ast::Stmt> next = parseNextTopLevel();
            if (!next) {
                llvm::outs() << "Current token: " << strTokenType(peekToken().getType()) << "\n";
                return true;
            }

            stmts.push_back(std::move(next));
        } while (peekToken().isNot(TokenType::Eof));
        *result = Module(LexerSource.getFileID(), std::move(stmts));
        // new (result) ast::Module(LexerSource.getFileID(), std::move(stmts));
        return false;
    }
private:
    const Token& peekToken() const {
        return CurrentToken;
    }

    const Token& nextToken() {
        CurrentToken = LexerSource.next();
        llvm::outs() << "nextToken() = \"" << strTokenType(peekToken().getType()) << "\"\n";
        return CurrentToken;
    }

    template <typename T, typename... Args>
    inline std::unique_ptr<T> makeStmtPtr(Args&&... args) const {
        // static_assert(std::is_base_of_v<ast::Stmt, T>, "T must derive from ast::Stmt.");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    inline std::unique_ptr<T> makeStmtPtr(T&& s) const {
        // static_assert(std::is_base_of_v<ast::Stmt, T>, "T must derive from ast::Stmt.");
        return std::make_unique<T>(std::move(s));
    }

    size_t stringToSizeT(llvm::StringRef NumberLiteral) {
        return 0;
    }

    double stringToDouble(llvm::StringRef floatLiteral) {
        return 0.0;
    }

    std::unique_ptr<ast::Stmt> parseNextTopLevel() {
        switch (peekToken().getType()) {
            default:
                llvm::outs() << "Only variable, type, struct, impl and function declarations"
                                " are allowed in the global namespace.\n";
                llvm::outs() << "Currently: " << strTokenType(peekToken().getType()) << "\n"; 
                return nullptr;

            case TokenType::Unknown:
                llvm::outs() << "Unknown token.\n";
                return nullptr;

            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
                llvm::outs() << "VarDecl\n";
                return parseVarDecl(false);

            case TokenType::Type:
                llvm::outs() << "TypeDecl\n";
                return parseTypeDecl();

            case TokenType::Enum:
                llvm::outs() << "Enums are not yet supported\n";
                return nullptr;

            case TokenType::Struct:
                llvm::outs() << "StructDecl\n";
                return parseStructDecl();

            case TokenType::Impl:
                llvm::outs() << "ImplDecl\n";
                return parseImplDecl(std::nullopt);

            case TokenType::Fn:
                llvm::outs() << "FunctionDecl\n";
                return parseFunctionDecl(false);
        }
    }

    std::unique_ptr<ast::Stmt> parseNextStmt() {
        if (isPrefixOp(peekToken().getType()))
            return parseExpr();

        switch (peekToken().getType()) {
            default:
                llvm::outs() << "Unexpected token: '" << strTokenType(peekToken().getType()) << "'.\n";
                return nullptr;

            case TokenType::Eof:
                llvm::outs() << "Unclosed '{'.\n";
                return nullptr;

            case TokenType::Move:
            case TokenType::Identifier:
                return parseExpr();

            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
                return parseVarDecl(false);

            case TokenType::Type:
                return parseTypeDecl();

            case TokenType::If:
                return parseIfStmt();
            case TokenType::Match:
                llvm::outs() << "'match' (switch) statements are not yet supported.\n";
                return nullptr;

            case TokenType::While:
                return parseWhileStmt();
            case TokenType::For:
                llvm::outs() << "'for' loops are not yet supported.\n";
                return nullptr;

            case TokenType::Return:
                return parseReturnStmt();
        }
    }

    bool isPrefixOp(TokenType t) {
        return t == TokenType::Exclamation ||   // Not
               t == TokenType::Minus ||         // Negative numer
               t == TokenType::Star ||          // Get pointer value
               t == TokenType::Ampersand;       // Get pointer
    }

    bool isSuffixOp(TokenType t) {
        return t == TokenType::PlusPlus || t == TokenType::MinusMinus;
    }

    BindingPower getBindingPower(TokenType op) {
        switch (op) {
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

            case TokenType::LogAND:
                return {30, 31};
            case TokenType::LogOR:
                return {20, 21};

            // Assignment (right associative)
            case TokenType::Equal:
            case TokenType::PlusEqual:
            case TokenType::MinusEqual:
            case TokenType::StarEqual:
            case TokenType::SlashEqual:
                return {10, 9};

            default:
                return BindingPower(0, 0);
        }
    }

    std::unique_ptr<ast::Expr> parsePrefix() {
        llvm::outs() << "\nParsing Prefix\n";

        Token start = peekToken();
        (void)nextToken();

        if (isPrefixOp(start.getType())) {
            llvm::outs() << "PrefixOperator\n";
            std::unique_ptr<ast::Expr> rhs = parseExpr(100);
            if (!rhs) return nullptr;

            SrcSpan span(start.getStartLoc(), rhs->getEndLoc());
            return makeStmtPtr<ast::UnaryOperator>(span, start.getType(), std::move(rhs));
        }

        switch (start.getType()) {
            default:
                llvm::outs() << "Invalid token: " << strTokenType(start.getType()) << ".\n";
                return nullptr;
            
            case TokenType::String:
                llvm::outs() << "StringLiteral\n";
                return makeStmtPtr<ast::StringLiteral>(
                    start.getSpan(), start.getData()
                );
            case TokenType::Int:
                llvm::outs() << "NumberLiteral\n";
                return makeStmtPtr<ast::NumberLiteral>(
                    start.getSpan(), stringToSizeT(start.getData())
                );
            case TokenType::Float:
                llvm::outs() << "FloatLiteral\n";
                return makeStmtPtr<ast::FloatLiteral>(
                    start.getSpan(), stringToDouble(start.getData())
                );
            case TokenType::Move: {
                llvm::outs() << "Move\n";
                std::unique_ptr<ast::Expr> v = parseExpr(100);
                SrcSpan span(start.getStartLoc(), v->getEndLoc());
                return makeStmtPtr<ast::MoveExpr>(std::move(span), std::move(v));
            }
            case TokenType::LParen: {
                llvm::outs() << "Parenthesis\n";
                std::unique_ptr<ast::Expr> v = parseExpr(0);
                if (peekToken().isNot(TokenType::RParen)) {
                    llvm::outs() << "Unclosed '('.\n";
                    return nullptr;
                }

                (void)nextToken();
                return v;
            }
            case TokenType::Identifier: {
                llvm::outs() << "Identifier\n";
                if (peekToken().isNot(TokenType::LParen)) {
                    return makeStmtPtr<ast::NameExpr>(
                        start.getSpan(), start.getData()
                    );
                }
                
                if (nextToken().is(TokenType::RParen)) {
                    SrcSpan span(start.getStartLoc(), peekToken().getEndLoc());

                    (void)nextToken();
                    return makeStmtPtr<ast::CallExpr>(std::move(span), start.getData());
                }

                llvm::SmallVector<std::unique_ptr<ast::Expr>> args;
                llvm::StringMap<std::unique_ptr<ast::Expr>> kwargs;
                do {
                    std::unique_ptr<ast::Expr> arg = parseExpr();
                    if (!arg) return nullptr;

                    args.push_back(std::move(arg));

                    if (peekToken().isNot(TokenType::Comma)) {
                        if (peekToken().is(TokenType::RParen))
                            break;

                        llvm::outs() << "Function arguments need to be separated by a ','.\n";
                        return nullptr;
                    }
                } while (nextToken().isNot(TokenType::RParen));
                
                SrcSpan span(start.getStartLoc(), peekToken().getEndLoc());

                (void)nextToken();
                return makeStmtPtr<ast::CallExpr>(
                    std::move(span), start.getData(), std::move(args), std::move(kwargs)
                );
            }
        }
    }

    std::unique_ptr<ast::Expr> parseExpr(int8_t minBP /*"BP" is binding power*/ = 0) {
        llvm::outs() << "\nParsing Expr\n";
        
        std::unique_ptr<ast::Expr> lhs = parsePrefix();
        if (lhs == nullptr) return nullptr;

        while (true) {
            if (!peekToken().isOperator()) {
                break;
            } else if (isSuffixOp(peekToken().getType())) {
                SrcSpan span(lhs->getStartLoc(), peekToken().getEndLoc());
                lhs = makeStmtPtr<ast::UnaryOperator>(
                    std::move(span), peekToken().getType(), std::move(lhs)
                );

                (void)nextToken();
                continue;
            }

            TokenType op = peekToken().getType();
            BindingPower bp = getBindingPower(op);
            if (bp.lbp < minBP) 
                break;

            (void)nextToken();
            std::unique_ptr<ast::Expr> rhs = parseExpr(bp.rbp);
            if (!rhs) return nullptr;

            SrcSpan span(lhs->getStartLoc(), rhs->getStartLoc());
            lhs = makeStmtPtr<ast::BinaryOperator>(std::move(span), op, std::move(lhs), std::move(rhs));
        }

        return lhs;
    }

    // TODO: Debug this
    std::unique_ptr<ast::TypeExpr> parseTypeExpr() {
        llvm::outs() << "\nParsing TypeRef\n";

        if (peekToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }

        Token start = peekToken();
        std::unique_ptr<ast::TypeExpr> result;
        if (nextToken().is(TokenType::Less)) {
            llvm::SmallVector<std::unique_ptr<ast::TypeExpr>> types;
            while (nextToken().isNot(TokenType::More)) {
                std::unique_ptr<ast::TypeExpr> t = parseTypeExpr();
                if (!t) return nullptr;

                types.push_back(std::move(t));
                if (nextToken().isNot(TokenType::Comma)) {
                    if (peekToken().is(TokenType::More))
                        break;
                    
                    llvm::outs() << "Template types need to be separated by a ','.\n";
                    return nullptr;
                }
            }

            SrcSpan span(start.getStartLoc(), peekToken().getEndLoc());

            (void)nextToken();
            result = makeStmtPtr<ast::TemplateTypeExpr>(
                std::move(span), start.getData(), std::move(types)
            );
        } else 
            std::unique_ptr<ast::TypeExpr> result = makeStmtPtr<ast::NamedTypeExpr>(
                start.getSpan(), start.getData()
            );

        if (peekToken().is(TokenType::Star)) {
            do {
                SrcSpan span(start.getStartLoc(), peekToken().getEndLoc());
                result = makeStmtPtr<ast::PointerTypeExpr>(
                    std::move(span), std::move(result)
                );
            } while (nextToken().is(TokenType::Star));
        }
        
        return result;
    }

    std::unique_ptr<ast::TypeDecl> parseTypeDecl() {
        llvm::outs() << "\nParsing TypeDecl\n";

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

        std::unique_ptr<ast::TypeExpr> type = parseTypeExpr();

        (void)nextToken();
        SrcSpan span(start, type->getEndLoc());
        return makeStmtPtr<ast::TypeDecl>(std::move(span), name, std::move(type));
    }
    
    std::unique_ptr<ast::FunctionDecl> parseFunctionDecl(bool isMethodAndIsPublic) {
        llvm::outs() << "\nParsing FunctionDecl\n";

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

        llvm::SmallVector<std::unique_ptr<ast::VarDecl>> args;
        while (nextToken().isNot(TokenType::RParen)) {
            switch (peekToken().getType()) {
                default:
                    llvm::outs() << "Invalid function arg declarator.\n";
                    return nullptr;

                case TokenType::Eof:
                    llvm::outs() << "Incomplete function declaration.\n";
                    return nullptr;

                case TokenType::Const:
                    llvm::outs() << "Function argument cannot be const.\n";
                    return nullptr;

                case TokenType::Let:
                case TokenType::Mut:
                case TokenType::Move:
                    std::unique_ptr<ast::VarDecl> a = parseVarDecl(false);
                    if (!a) return nullptr;

                    args.push_back(std::move(a));
                    break;
            }

            if (peekToken().isNot(TokenType::Comma)) {
                if (peekToken().is(TokenType::RParen))
                    break;

                llvm::outs() << "Function arguments need to be separated by a ','.\n";
                return nullptr;
            }
        }

        if (nextToken().isNot(TokenType::Arrow)) {
            llvm::outs() << "Expected '->'.\n";
            return nullptr;
        }

        (void)nextToken();
        std::unique_ptr<ast::TypeExpr> type = parseTypeExpr();
        if (!type) return nullptr;

        SrcLoc end = peekToken().getEndLoc();

        if (peekToken().isNot(TokenType::LBrace)) {
            llvm::outs() << "Unclosed '{'.\n";
            return nullptr;
        }
        std::unique_ptr<ast::BlockStmt> body = parseBlockStmt();
        if (!body) return nullptr;

        SrcSpan span(start, end);
        return makeStmtPtr<ast::FunctionDecl>(
            std::move(span), name, std::move(type), std::move(body), std::move(args)
        );
    }

    // TODO: Debug this
    std::unique_ptr<ast::ImplDecl> parseImplDecl(std::optional<llvm::StringRef> structName) {
        llvm::outs() << "\nParsing ImplDecl\n";

        SrcLoc start = peekToken().getStartLoc();

        llvm::StringRef name;
        if (!structName.has_value()) {
            if (nextToken().isNot(TokenType::Identifier)) {
                llvm::outs() << "Expected identifier.\n";
                return nullptr;
            }
            name = peekToken().getData();
            
            if (nextToken().isNot(TokenType::LBrace)) {
                llvm::outs() << "Expected '{'.\n";
                return nullptr;
            }
        } else name = structName.value();

        SrcLoc end = peekToken().getEndLoc();

        llvm::SmallVector<std::unique_ptr<ast::FunctionDecl>> methods;
        while (nextToken().isNot(TokenType::RBrace) && peekToken().isNot(TokenType::Eof)) {
            bool is_public = false;
            if (peekToken().is(TokenType::Pub)) {
                (void)nextToken();
                is_public = true;
            }

            if (peekToken().isNot(TokenType::Fn)) {
                if (peekToken().is(TokenType::RBrace) || peekToken().is(TokenType::Eof))
                    break;
                
                llvm::outs() << "Expected function declarator.\n";
                return nullptr;
            }

            std::unique_ptr<ast::FunctionDecl> method = parseFunctionDecl(is_public);
            if (!method) return nullptr;

            methods.push_back(std::move(method));
        }

        SrcSpan span(start, end);
        return makeStmtPtr<ast::ImplDecl>(span, name, std::move(methods));
    }

    // TODO: Debug this
    std::unique_ptr<ast::StructDecl> parseStructDecl() {
        llvm::outs() << "\nParsing StructDecl\n";

        SrcLoc start = peekToken().getStartLoc();
        if (nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = peekToken().getData();

        if (nextToken().isNot(TokenType::LBrace)) {
            llvm::outs() << "Expected '{'.\n";
            return nullptr;
        }

        llvm::SmallVector<std::unique_ptr<ast::VarDecl>> attrs;
        while (nextToken().isNot(TokenType::RParen)) {
            bool is_public = false;
            if (peekToken().is(TokenType::Pub)) {
                (void)nextToken();
                is_public = true;
            }

            switch (peekToken().getType()) {
                default:
                    llvm::outs() << "Invalid struct attr declarator.\n";
                    return nullptr;

                case TokenType::Eof:
                    llvm::outs() << "Incomplete attribute declaration.\n";
                    return nullptr;

                case TokenType::Const:
                    llvm::outs() << "Struct attributes cannot be const.\n";
                    return nullptr;
                
                case TokenType::Move:
                    llvm::outs() << "Struct attributes cannot be move.\n";
                    return nullptr;

                case TokenType::Let:
                case TokenType::Mut:
                    std::unique_ptr<ast::VarDecl> a = parseVarDecl(is_public);
                    if (!a) return nullptr;

                    attrs.push_back(std::move(a));
                    break;
            }
        }

        if (!peekToken().is(TokenType::RBrace)) {
            llvm::outs() << "Expected '}'.\n";
            return nullptr;
        }
        
        SrcLoc end = peekToken().getEndLoc();

        std::unique_ptr<ast::ImplDecl> impl;
        llvm::SmallVector<std::unique_ptr<ast::ImplRef>> inherited;
        if (nextToken().is(TokenType::Impl)) {
            if (nextToken().isNot(TokenType::Identifier)) {
                llvm::outs() << "Expected identifier.\n";
                return nullptr;
            }
            llvm::StringRef impl_name = peekToken().getData();

            if (nextToken().is(TokenType::From)) {
                (void)nextToken();
                do {
                    if (peekToken().is(TokenType::Identifier)) {
                        inherited.push_back(makeStmtPtr<ast::ImplRef>(
                            peekToken().getSpan(), peekToken().getData()
                        ));
                    } else {
                        llvm::outs() << "Expected identifier.\n";
                        return nullptr;
                    }
                } while (nextToken().isNot(TokenType::LBrace));
            } else if (peekToken().isNot(TokenType::LBrace)) {
                llvm::outs() << "Expected '{' or 'from'.\n";
                return nullptr;
            }

            impl = parseImplDecl(impl_name);
            if (!impl) return nullptr;
        }

        SrcSpan span(start, end);
        return makeStmtPtr<ast::StructDecl>(span, name, std::move(attrs), std::move(impl), std::move(inherited));
    }

    std::unique_ptr<ast::VarDecl> parseVarDecl(bool isAttrAndIsPublic) {
        llvm::outs() << "\nParsing VarDecl\n";

        SrcLoc start = peekToken().getStartLoc();
        TokenType kind = peekToken().getType();

        if (nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = peekToken().getData();

        std::unique_ptr<ast::TypeExpr> type;
        if (nextToken().is(TokenType::Colon)) {
            (void)nextToken();
            type = parseTypeExpr();

            if (peekToken().isNot(TokenType::Equal)) {
                llvm::outs() << "Expected '='.\n";
                return nullptr;
            }
        } else if (peekToken().isNot(TokenType::Equal)) {
            llvm::outs() << "Expected ':' or '='.\n";
            return nullptr;
        }

        (void)nextToken();
        std::unique_ptr<ast::Expr> value = parseExpr();

        SrcSpan span(start, value->getEndLoc());
        return makeStmtPtr<ast::VarDecl>(
            std::move(span), name, kind, std::move(type), std::move(value)
        );
    }

    std::unique_ptr<ast::IfStmt> parseIfStmt() {
        llvm::outs() << "\nParsing IfStmt\n";

        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip leading "If" token
        std::unique_ptr<ast::Expr> condition = parseExpr();
        SrcSpan span(start, peekToken().getEndLoc());

        std::unique_ptr<ast::Stmt> body = parseStmtBody();

        std::unique_ptr<ast::IfStmt> else_stmt;
        if (peekToken().is(TokenType::Elif)) {
            else_stmt = parseIfStmt();
        } else if (peekToken().is(TokenType::Else)) {
            SrcLoc else_start = peekToken().getStartLoc();

            (void)nextToken();
            std::unique_ptr<ast::Stmt> else_body = parseStmtBody();
            SrcSpan else_span(else_start, peekToken().getEndLoc());
            else_stmt = makeStmtPtr<ast::IfStmt>(
                std::move(else_span), nullptr, std::move(else_body), nullptr
            );
        }

        return makeStmtPtr<ast::IfStmt>(
            std::move(span), std::move(condition), std::move(body), std::move(else_stmt)
        );
    }

    std::unique_ptr<ast::WhileStmt> parseWhileStmt() {
        llvm::outs() << "\nParsing WhileStmt\n";

        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip leading "While" token
        std::unique_ptr<ast::Expr> condition = parseExpr();
        if (!condition) return nullptr;

        SrcSpan span(start, peekToken().getEndLoc());

        std::unique_ptr<ast::Stmt> body = parseStmtBody();
        if (!body) return nullptr;

        return makeStmtPtr<ast::WhileStmt>(
            std::move(span), std::move(condition), std::move(body)
        );
    }

    std::unique_ptr<ast::ReturnStmt> parseReturnStmt() {
        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken();
        std::unique_ptr<ast::Expr> value = parseExpr();

        SrcSpan span(start, value->getEndLoc());
        return makeStmtPtr<ast::ReturnStmt>(std::move(span), std::move(value));
    }

    std::unique_ptr<ast::Stmt> parseStmtBody() {
        llvm::outs() << "\nParsing StmtBody\n";

        if (peekToken().is(TokenType::LBrace))
            return parseBlockStmt();
        return parseNextStmt();
    }

    std::unique_ptr<ast::BlockStmt> parseBlockStmt() {
        llvm::outs() << "\nParsing BlockStmt\n";

        SrcLoc start = peekToken().getStartLoc();

        (void)nextToken(); // Skip the leading "{" token
        llvm::SmallVector<std::unique_ptr<ast::Stmt>> stmts;
        while (peekToken().isNot(TokenType::RBrace)) {
            std::unique_ptr<ast::Stmt> next = parseNextStmt();
            if (!next) return nullptr;

            stmts.push_back(std::move(next));
        }

        SrcSpan span(start, peekToken().getEndLoc());

        (void)nextToken();
        return makeStmtPtr<ast::BlockStmt>(std::move(span), std::move(stmts));
    }
};

} // namespace c
