#include "include/AST/Parser.hpp"

#include <type_traits>

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/TypeInfo.hpp"
#include "include/IO/SrcSpan.hpp"

#include "src/Debug.hpp"

using namespace c;
using namespace c::ast;

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
            return ast::UnaryOperator::OpKind::Adress;
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

class PrattParser {
private:
    struct BindingPower {
        uint8_t lbp; // Left binding power
        uint8_t rbp; // Right binding power
    };

    static constexpr uint8_t MaxBindingPower = 100;

    Parser& P;
    ASTAllocator& Alloc;
public:
    PrattParser(Parser& parser, ASTAllocator& alloc)
        : P(parser), Alloc(alloc) {}
    
    BindingPower getBindingPower(TokenType op) {
        switch (op) {
            default:
                llvm_unreachable("Invalid TokenType passed to PrattParser::getBindingPower");

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

    Expr* parsePrefix() {
        DEBUG("Called: parsePrefix");
        switch (P.peekToken().getType()) {
            default:
                llvm::outs() << "Invalid token: " << strTokenType(P.peekToken().getType()) << ".\n";
                return nullptr;

            case TokenType::Plus:
            case TokenType::Minus:
            case TokenType::Exclamation:
            case TokenType::Star:
            case TokenType::Ampersand:
                return parseUnaryPrefixOperator();
            case TokenType::LBrack:
                return parseArrayLiteral();
            case TokenType::Int:
                return parseIntegerLiteral();
            case TokenType::Float:
                return parseFloatingLiteral();
            case TokenType::LParen:
                return parseParenthesis();
            case TokenType::Identifier:
                return parseIdentifier();
        }
    }

    bool isUnarySuffixOperator(TokenType T) {
        return T == TokenType::PlusPlus ||
               T == TokenType::MinusMinus;
    }

    Expr* parseExpr(uint8_t minBP = 0) {
        DEBUG("Called: parseExpr");
        Expr* lhs = parsePrefix();
        if (!lhs) return nullptr;

        while (true) {
            if (!P.peekToken().isOperator()) {
                break;
            } else if (isUnarySuffixOperator(P.peekToken().getType())) {
                SrcSpan span(lhs->getStartLoc(), P.peekToken().getEndLoc());
                lhs = Alloc.Create<UnaryOperator>(
                    span, getUnaryOperator(P.peekToken().getType()), lhs
                );

                (void)P.nextToken();
                continue;
            }

            TokenType op = P.peekToken().getType();
            BindingPower bp = getBindingPower(op);
            if (bp.lbp < minBP)
                break;

            (void)P.nextToken();
            Expr* rhs = parseExpr(bp.rbp);
            if (!rhs) return nullptr;

            SrcSpan span(lhs->getStartLoc(), rhs->getStartLoc());
            lhs = Alloc.Create<BinaryOperator>(
                span, getBinaryOperator(op), lhs, rhs
            );
        }

        return lhs;
    }

    ArrayLiteral* parseArrayLiteral() {
        DEBUG("Called: parseArrayLiteral");
        assert(P.peekToken().is(TokenType::LBrack));
        SrcLoc start = P.peekToken().getStartLoc();
        llvm::SmallVector<Expr*> items;
        if (P.nextToken().isNot(TokenType::RBrack))
            while (true) {
                if (P.peekToken().is(TokenType::Eof)) {
                    llvm::outs() << "Unclosed array literal. Expected ']'.\n";
                    return nullptr;
                }
                
                Expr* next = parseExpr();
                if (!next) return nullptr;

                items.push_back(next);

                if (P.peekToken().is(TokenType::RBrack)) {
                    break;
                } else if (P.peekToken().is(TokenType::Comma)) {
                    if (P.nextToken().is(TokenType::RBrack)) {
                        llvm::outs() << "Unnessesary ','.\n";
                        return nullptr;
                    }
                } else
                    (void)P.nextToken();
            }

        SrcSpan span(start, P.peekToken().getEndLoc());

        (void)P.nextToken();
        return Alloc.Create<ArrayLiteral>(span, items);
    }

    IntegerLiteral* parseIntegerLiteral() {
        DEBUG("Called: parseIntegerLiteral");
        SrcSpan span = P.peekToken().getSpan();

        llvm::APInt value;
        llvm::StringRef data = P.peekToken().getData();
        if (data.getAsInteger(10, value)) {
            llvm::outs() << "Invalid integer literal\n";
            return nullptr;
        }

        (void)P.nextToken();
        return Alloc.Create<IntegerLiteral>(
            span, std::move(value)
        );
    }

    FloatingLiteral* parseFloatingLiteral() {
        DEBUG("Called: parseFloatingLiteral");
        SrcSpan span = P.peekToken().getSpan();

        llvm::APFloat value(llvm::APFloat::IEEEdouble());
        llvm::StringRef data = P.peekToken().getData();
        auto mode = llvm::APFloat::rmNearestTiesToEven;
        if (!value.convertFromString(data, mode)) {
            llvm::outs() << "Invalid float literal.\n";
            return nullptr;
        }

        (void)P.nextToken();
        return Alloc.Create<FloatingLiteral>(
            span, std::move(value)
        );
    }

    Expr* parseUnaryPrefixOperator() {
        DEBUG("Called: parseUnaryPrefixOperator");
        TokenType op = P.peekToken().getType();
        SrcLoc start = P.peekToken().getStartLoc();

        Expr* rhs = parseExpr(MaxBindingPower);
        if (!rhs) return nullptr;
        
        SrcSpan span(start, rhs->getEndLoc());
        return Alloc.Create<UnaryOperator>(
            span, getUnaryOperator(op), rhs
        );
    }

    Expr* parseParenthesis() {
        DEBUG("Called: parseParethesis");
        (void)P.nextToken();
        ast::Expr* result = parseExpr(0);
        if (P.peekToken().isNot(TokenType::RParen)) {
            llvm::outs() << "Unclosed '('.\n";
            return nullptr;
        }

        (void)P.nextToken();
        return result;
    }

    Expr* parseCallExpr(llvm::StringRef name, SrcLoc start) {
        DEBUG("Called: CallExpr");
        auto result = Alloc.Create<CallExpr>(SrcSpan(), name);

        llvm::SmallVector<Expr*> args;
        if (P.nextToken().isNot(TokenType::RParen))
            while (true) {
                if (P.peekToken().is(TokenType::Eof)) {
                    llvm::outs() << "Unclosed function call. Expected ')'.\n";
                    return nullptr;
                }
                
                Expr* next = parseExpr();
                if (!next) return nullptr;

                args.push_back(next);

                if (P.peekToken().is(TokenType::RParen)) {
                    break;
                } else if (P.peekToken().is(TokenType::Comma)) {
                    if (P.nextToken().is(TokenType::RParen)) {
                        llvm::outs() << "Unnessesary ','.\n";
                        return nullptr;
                    }
                } else
                    (void)P.nextToken();
            }

        result->setArguments(args);
        result->setSpan(SrcSpan(start, P.peekToken().getEndLoc()));

        (void)P.nextToken();
        return result;
    }
    
    Expr* parseIdentifier() {
        DEBUG("Called: parseIdentifier");
        assert(P.peekToken().is(TokenType::Identifier));
        llvm::StringRef name = P.peekToken().getData();
        SrcSpan start_span = P.peekToken().getSpan();

        Expr* base;
        if (P.nextToken().isNot(TokenType::LParen)) {
            base = Alloc.Create<DeclRefExpr>(start_span, name);
        } else 
            base = parseCallExpr(name, start_span.getStart()); 

        if (P.peekToken().is(TokenType::Dot)) {
            if (P.nextToken().isNot(TokenType::Identifier)) {
                llvm::outs() << "Expected identifier.\n";
                return nullptr;
            }

            llvm::StringRef field_name = P.peekToken().getData();
            SrcSpan span(start_span.getStart(), P.peekToken().getEndLoc());

            (void)P.nextToken();
            return Alloc.Create<AccessExpr>(span, base, field_name);
        } else if (P.peekToken().is(TokenType::LBrack)) {
            (void)P.nextToken();
            Expr* start = parseIntegerLiteral();
            if (P.peekToken().isNot(TokenType::RBrack)) {
                llvm::outs() << "Expected closing ']'\n";
                return nullptr;
            }

            SrcSpan span(start_span.getStart(), P.peekToken().getEndLoc());

            (void)P.nextToken();
            return Alloc.Create<SliceExpr>(span, base, start);
        }

        return base;
    }
};

class RecurseiveDescentParser {
private:
    Parser& P;
    PrattParser& Pratt;
    ASTAllocator& Alloc;
public:
    RecurseiveDescentParser(Parser& parser, PrattParser& pratt, ASTAllocator& alloc)
        : P(parser), Pratt(pratt), Alloc(alloc) {}

    Decl* parseDecl() {
        DEBUG("Called: parseDecl");
        switch (P.peekToken().getType()) {
            default:
                llvm::outs() << "Only variable, type, struct, impl and function declarations"
                                " are allowed in the global namespace.\n";
                llvm::outs() << "Currently: " << strTokenType(P.peekToken().getType()) << "\n"; 
                return nullptr;

            case TokenType::Type:
                return parseTypeAiasDecl();

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

    Stmt* parseStmt() {
        DEBUG("Called: parseStmt");
        switch (P.peekToken().getType()) {
            case TokenType::Type: {
                ast::TypeAliasDecl* DC = parseTypeAiasDecl();
                return Alloc.Create<DeclStmt>(DC);
            }

            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut: {
                ast::VarDecl* DC = parseVarDecl();
                return Alloc.Create<DeclStmt>(DC);
            }

            case TokenType::If:
                return parseIfStmt();
            case TokenType::While:
                return parseWhileStmt();
            case TokenType::Return:
                return parseReturnStmt();

            default:
                return parseExpr();
        }
    }

    Expr* parseExpr() {
        DEBUG("Called: parseExpr (RecusiveDecentParser)");
        return Pratt.parseExpr();
    }

    TypeInfo* parseTypeRef() {
        DEBUG("Called: parseTypeRef");
        if (P.peekToken().is(TokenType::LBrack)) {
            SrcLoc start = P.peekToken().getStartLoc();

            (void)P.nextToken();
            TypeInfo* pointee = parseTypeRef();

            size_t size = 0;
            if (P.peekToken().is(TokenType::Colon)) {
                if (P.nextToken().isNot(TokenType::Int)) {
                    llvm::outs() << "Expected an integer literal for the size of the array.\n";
                    return nullptr;
                }

                if (P.peekToken().getData().getAsInteger(10, size))
                    // Indicates an error probably in the lexer with how integer literals are
                    // matched or with how the tokens are constructed.
                    llvm_unreachable("'Data' for integer literal token was not an integer literal.");
                
                if (P.nextToken().isNot(TokenType::RBrack)) {
                    llvm::outs() << "Expected ']' to close the array type.\n";
                    return nullptr;
                }
            } else if (!P.peekToken().is(TokenType::RBrack)) {
                llvm::outs() << "Invalid array type, expected ':' or ']'.\n";
                return nullptr;
            }
            
            SrcSpan span(start, P.peekToken().getEndLoc());

            (void)P.nextToken();
            return Alloc.Create<TypeInfo>(TypeInfo::CreateArray(span, pointee, size));
        } else if (P.peekToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }

        auto result = Alloc.Create<TypeInfo>(TypeInfo::CreateNamed(
            P.peekToken().getSpan(), P.peekToken().getData()
        ));

        (void)P.nextToken();
        if (P.peekToken().is(TokenType::Star) ||
            P.peekToken().is(TokenType::Ampersand)) {
            SrcLoc start = result->getStartLoc();
            do {
                result = Alloc.Create<TypeInfo>(TypeInfo::CreatePointer(
                    SrcSpan(start, P.peekToken().getEndLoc()),
                    P.peekToken().is(TokenType::Star) ? true : false,
                    result
                ));
                (void)P.nextToken();
            } while (P.peekToken().is(TokenType::Star) ||
                     P.peekToken().is(TokenType::Ampersand));
        }

        return result;
    }

    TypeAliasDecl* parseTypeAiasDecl() {
        DEBUG("Called: parseTypeAliasDecl");
        SrcLoc start = P.peekToken().getStartLoc();
        if (P.nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();

        if (P.nextToken().isNot(TokenType::Equal)) {
            llvm::outs() << "Expected '='.\n";
            return nullptr;
        }

        (void)P.nextToken();
        TypeInfo* type = parseTypeRef();
        if (!type) return nullptr;
        
        (void)P.nextToken();
        SrcSpan span(start, type->getEndLoc());
        return Alloc.Create<TypeAliasDecl>(span, name, type);
    }

    bool parseInitDecl(SrcLoc& end, llvm::StringRef& name, TypeInfo*& info, Expr*& init) {
        DEBUG("Called: parseInitDecl");
        if (P.peekToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier, got '" << strTokenType(P.peekToken().getType()) <<"'.\n";
            return true;
        }
        name = P.peekToken().getData();

        // Only used for the else case to create an SrcLoc for the implicit type.
        size_t colon_offset = P.peekToken().getEndLoc().getOffset() + 1;

        if (P.nextToken().is(TokenType::Colon)) {
            (void)P.nextToken();
            info = parseTypeRef();
            if (!info) return true;
        } else if (!P.peekToken().is(TokenType::Equal)) {
            llvm::outs() << "Expected ':' or '='.\n";
            return true;
        } else 
            info = Alloc.Create<TypeInfo>(TypeInfo::CreateImplicit(
                SrcLoc(P.peekToken().getSpan().getID(), colon_offset)
            ));

        if (P.peekToken().is(TokenType::Equal)) {
            (void)P.nextToken();
            init = parseExpr();
            end = init->getEndLoc();
            if (!init) return true;
        } else
            end = info->getEndLoc();

        return false;
    }

    VarDecl* parseVarDecl() {
        DEBUG("Called: parseVarDecl");
        VarDecl::Mutability mutability;
        if (P.peekToken().is(TokenType::Const)) {
            mutability = VarDecl::Constant;
        } else if (P.peekToken().is(TokenType::Let)) {
            mutability = VarDecl::Immutable;
        } else if (P.peekToken().is(TokenType::Mut)) {
            mutability = VarDecl::Mutable;
        } else 
            llvm_unreachable("Invalid Mutability for VarDecl");

        SrcLoc start = P.peekToken().getStartLoc();

        (void)P.nextToken();
        SrcLoc end;
        llvm::StringRef name;
        TypeInfo* info = nullptr;
        Expr* init = nullptr;
        if (parseInitDecl(end, name, info, init))
            return nullptr;

        return Alloc.Create<VarDecl>(
            SrcSpan(start, end), name, mutability, info, init
        );
    }

    ParamDecl* parseParamDecl() {
        DEBUG("Called: parseParamDecl");
        SrcLoc start = P.peekToken().getStartLoc();

        SrcLoc end;
        llvm::StringRef name;
        TypeInfo* info = nullptr;
        Expr* init = nullptr;
        if (parseInitDecl(end, name, info, init))
            return nullptr;

        return Alloc.Create<ParamDecl>(
            SrcSpan(start, end), name, info, init
        );
    }

    FieldDecl* parseFieldDecl(size_t fieldIndex) {
        DEBUG("Called: parseFieldDecl");
        SrcLoc start = P.peekToken().getStartLoc();

        SrcLoc end;
        llvm::StringRef name;
        TypeInfo* info = nullptr;
        Expr* init = nullptr;
        if (parseInitDecl(end, name, info, init))
            return nullptr;
        
        return Alloc.Create<FieldDecl>(
            SrcSpan(start, end), name, fieldIndex, info, init
        );
    }

    StructDecl* parseStructDecl() {
        DEBUG("Called: parseStructDecl");
        SrcLoc start = P.peekToken().getStartLoc();
        if (P.nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();

        if (P.nextToken().isNot(TokenType::Colon)) {
            llvm::outs() << "Expected ':'.\n";
            return nullptr;
        }

        (void)P.nextToken();
        StructDecl* result = Alloc.Create<StructDecl>(name);
        llvm::SmallVector<FieldDecl*> fields;
        // while (P.nextToken().isNot(TokenType::End)) {
        while (P.peekToken().isNot(TokenType::End)) {
            if (P.peekToken().is(TokenType::Eof)) {
                llvm::outs() << "Unclosed struct declaration. Expected 'end'.\n";
                return nullptr;
            }

            FieldDecl* next = parseFieldDecl(fields.size());
            if (!next) return nullptr;

            fields.push_back(next);
            result->pushDecl(next);
        }

        result->setFields(fields);
        result->setSpan(SrcSpan(start, P.peekToken().getEndLoc()));

        (void)P.nextToken();
        return result;
    }

    FunctionDecl* parseFunctionDecl() {
        DEBUG("Called: parseFunctionDecl");
        SrcLoc start = P.peekToken().getStartLoc();
        if (P.nextToken().isNot(TokenType::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();
        
        if (P.nextToken().isNot(TokenType::LParen)) {
            llvm::outs() << "Expected '('.\n";
            return nullptr;
        }

        FunctionDecl* result = Alloc.Create<FunctionDecl>(name);
        llvm::SmallVector<ParamDecl*> parameters;
        if (P.nextToken().isNot(TokenType::RParen))
            while (true) {
                if (P.peekToken().is(TokenType::Eof)) {
                    llvm::outs() << "Unclosed function declaration. Expected ')'.\n";
                    return nullptr;
                }

                ParamDecl* next = parseParamDecl();
                if (!next) return nullptr;

                parameters.push_back(next);
                result->pushDecl(next);

                if (P.peekToken().is(TokenType::RParen)) {
                    break;
                } else if (P.peekToken().is(TokenType::Comma)) 
                    if (P.nextToken().is(TokenType::RParen)) {
                        llvm::outs() << "Unnessesary ',' after the last parameter in a function declaration.\n";
                        return nullptr;
                    }                
            }

        // Ends the function declaration at the closing ")" in case
        // in case the function doesn't have a return type.
        SrcLoc end = P.peekToken().getEndLoc();

        result->setParams(parameters);
        if (P.nextToken().is(TokenType::Arrow)) {
            (void)P.nextToken();
            TypeInfo* type = parseTypeRef();
            if (!type) return nullptr;

            end = P.peekToken().getEndLoc();
            result->setTypeInfo(type);
        }

        result->setSpan(SrcSpan(start, end));

        if (P.peekToken().is(TokenType::Colon)) {
            BlockStmt* body = parseBlockStmt();
            if (!body) return nullptr;

            result->setBody(body);
        }

        return result;
    }

    BlockStmt* parseBlockStmt() {
        DEBUG("Called: parseBlockStmt");
        assert(P.peekToken().is(TokenType::Colon));

        SrcLoc start = P.peekToken().getStartLoc();

        (void)P.nextToken();
        llvm::SmallVector<Stmt*> stmts;
        while (P.peekToken().isNot(TokenType::End)) {
            // FIXME: I don't like this setup, but it works for now...
            if (P.peekToken().is(TokenType::Elif) || 
                P.peekToken().is(TokenType::Else)) {
                SrcSpan span(start, P.peekToken().getEndLoc());
                return Alloc.Create<BlockStmt>(span, stmts);
            }

            if (P.peekToken().is(TokenType::Eof)) {
                llvm::outs() << "Unclosed ':'.\n";
                return nullptr;
            }

            Stmt* next = parseStmt();
            if (!next) return nullptr;

            stmts.push_back(next);
        }

        SrcSpan span(start, P.peekToken().getEndLoc());

        (void)P.nextToken();
        return Alloc.Create<BlockStmt>(span, stmts);
    }

    Stmt* parseStmtBody() {
        DEBUG("Called: parseStmtBody");
        if (P.peekToken().is(TokenType::Colon))
            return parseBlockStmt();
        // FIXME: Should also look for an Expr as well
        return parseStmt();
    }

    IfStmt* parseIfStmt() {
        DEBUG("Called: parseIfStmt");
        SrcLoc start = P.peekToken().getStartLoc();

        (void)P.nextToken(); // Skip leading If token
        ast::Expr* condition = parseExpr();
        if (!condition) return nullptr;

        SrcSpan span(start, P.peekToken().getEndLoc());

        Stmt* body = parseStmtBody();
        if (!body) return nullptr;

        Stmt* elseif = nullptr;
        if (P.peekToken().is(TokenType::Elif)) {
            elseif = parseIfStmt();
            if (!elseif) return nullptr;
        } else if (P.peekToken().is(TokenType::Else)) {
            (void)P.nextToken(); // Skip over the leading Else token
            elseif = parseStmtBody();
            if (!elseif) return nullptr;
        }

        return Alloc.Create<IfStmt>(span, condition, body, elseif);
    }

    WhileStmt* parseWhileStmt() {
        DEBUG("Called: parseWhileStmt");
        SrcLoc start = P.peekToken().getStartLoc();

        // FIXME: Currently doesn't allow a declaration within the condition.
        (void)P.nextToken(); // Skip leading "While" token
        Expr* condition = parseExpr();
        if (!condition) return nullptr;

        SrcSpan span(start, P.peekToken().getEndLoc());

        Stmt* body = parseStmtBody();
        if (!body) return nullptr;

        return Alloc.Create<WhileStmt>(span, condition, body);
    }

    ReturnStmt* parseReturnStmt() {
        DEBUG("Called: parseReturnStmt");
        SrcLoc start = P.peekToken().getStartLoc();
        SrcLoc end = P.peekToken().getEndLoc();

        (void)P.nextToken(); // Skip the leading Return token
        // FIXME: Probably find a better way of doing this.
        Expr* value = nullptr;
        if (!P.peekToken().isKeyword() && !P.peekToken().is(TokenType::Self)) {
            value = parseExpr();
            end = value->getEndLoc();
        }

        return Alloc.Create<ReturnStmt>(SrcSpan(start, end), value);
    }
};

namespace c {

const Token& Parser::nextToken() {
    CurrentToken = LexerSource.next();
    DEBUG(std::string("Called: nextToken:")
          + " Kind: " + strTokenType(CurrentToken.getType()).str() 
          + " Data: " + CurrentToken.getData().str());
    return CurrentToken;
}

bool Parser::parse(ModuleDecl& result) {
    DEBUG("Called: parse");
    if (peekToken().is(TokenType::Eof))
        return true;

    PrattParser pratt(*this, Alloc);
    RecurseiveDescentParser recurse(*this, pratt, Alloc);

    do {
        ast::Decl* next = recurse.parseDecl();
        if (!next) return true;

        result.pushDecl(next);
    } while (!peekToken().is(TokenType::Eof));
    return false;
}

} // namespace c
