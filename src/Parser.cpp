#include "include/AST/Parser.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/Support/Error.h"
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

ast::BinaryOperator::OpKind getBinaryOperator(TokenKind type) {
    switch (type) {
        default:
            llvm_unreachable(""); // FIXME: Add message

        case TokenKind::Plus:
            return ast::BinaryOperator::OpKind::Add;
        case TokenKind::Minus:
            return ast::BinaryOperator::OpKind::Sub;
        case TokenKind::Star:
            return ast::BinaryOperator::OpKind::Mul;
        case TokenKind::Slash:
            return ast::BinaryOperator::OpKind::Div;

        case TokenKind::Equal:
            return ast::BinaryOperator::OpKind::Assign;
        case TokenKind::PlusEqual:
            return ast::BinaryOperator::OpKind::AddAssign;
        case TokenKind::MinusEqual:
            return ast::BinaryOperator::OpKind::SubAssign;
        case TokenKind::StarEqual:
            return ast::BinaryOperator::OpKind::MulAssign;
        case TokenKind::SlashEqual:
            return ast::BinaryOperator::OpKind::DivAssign;

        case TokenKind::EqualEqual:
            return ast::BinaryOperator::OpKind::Equal;
        case TokenKind::ExclEqual:
            return ast::BinaryOperator::OpKind::NotEqual;
        case TokenKind::More:
            return ast::BinaryOperator::OpKind::MoreThan;
        case TokenKind::Less:
            return ast::BinaryOperator::OpKind::LessThan;
        case TokenKind::MoreEqual:
            return ast::BinaryOperator::OpKind::MTEqual;
        case TokenKind::LessEqual:
            return ast::BinaryOperator::OpKind::LTEqual;

        case TokenKind::AmpAmp:
            return ast::BinaryOperator::OpKind::And;
        case TokenKind::PipePipe:
            return ast::BinaryOperator::OpKind::Or;
    }
}

ast::UnaryOperator::OpKind getUnaryOperator(TokenKind type) {
    switch (type) {
        default:
            llvm_unreachable(""); // FIXME: Add message

        case TokenKind::Star:
            return ast::UnaryOperator::OpKind::Deref;
        case TokenKind::Ampersand:
            return ast::UnaryOperator::OpKind::Adress;
        case TokenKind::PlusPlus:
            return ast::UnaryOperator::OpKind::AddOne;
        case TokenKind::MinusMinus:
            return ast::UnaryOperator::OpKind::SubOne;
        case TokenKind::Plus:
            return ast::UnaryOperator::OpKind::Plus;
        case TokenKind::Minus:
            return ast::UnaryOperator::OpKind::Minus;
        case TokenKind::Exclamation:
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

    BindingPower getBindingPower(TokenKind op) {
        switch (op) {
            default:
                llvm_unreachable("Invalid TokenKind passed to PrattParser::getBindingPower");

            case TokenKind::Star:
            case TokenKind::Slash:
                return {70, 71};

            case TokenKind::Plus:
            case TokenKind::Minus:
                return {60, 61};

            case TokenKind::Less:
            case TokenKind::More:
            case TokenKind::LessEqual:
            case TokenKind::MoreEqual:
                return {50, 51};

            case TokenKind::EqualEqual:
            case TokenKind::ExclEqual:
                return {45, 46};

            case TokenKind::AmpAmp:
                return {30, 31};
            case TokenKind::PipePipe:
                return {20, 21};

            // Assignment (right associative)
            case TokenKind::Equal:
            case TokenKind::PlusEqual:
            case TokenKind::MinusEqual:
            case TokenKind::StarEqual:
            case TokenKind::SlashEqual:
                return {10, 9};
        }
    }

    Expr* parsePrefix() {
        DEBUG("Called: parsePrefix");
        switch (P.peekToken().getType()) {
            default:
                llvm::outs() << "Invalid token: " << lex::strTokenKind(P.peekToken().getType()) << ".\n";
                return nullptr;

            case TokenKind::Plus:
            case TokenKind::Minus:
            case TokenKind::Exclamation:
            case TokenKind::Star:
            case TokenKind::Ampersand:
                return parseUnaryPrefixOperator();
            case TokenKind::LSquare:
                return parseArrayLiteral();
            case TokenKind::Integer:
                return parseIntegerLiteral();
            case TokenKind::FloatPoint:
                return parseFloatingLiteral();
            case TokenKind::LParen:
                return parseParenthesis();
            case TokenKind::Identifier:
                return parseIdentifier();
        }
    }

    bool isUnarySuffixOperator(TokenKind T) {
        return T == TokenKind::PlusPlus ||
               T == TokenKind::MinusMinus;
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

            TokenKind op = P.peekToken().getType();
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
        assert(P.peekToken().is(TokenKind::LSquare));
        SrcLoc start = P.peekToken().getStartLoc();
        llvm::SmallVector<Expr*> items;
        if (P.nextToken().isNot(TokenKind::RSquare))
            while (true) {
                if (P.peekToken().is(TokenKind::Eof)) {
                    llvm::outs() << "Unclosed array literal. Expected ']'.\n";
                    return nullptr;
                }

                Expr* next = parseExpr();
                if (!next) return nullptr;

                items.push_back(next);

                if (P.peekToken().is(TokenKind::RSquare)) {
                    break;
                } else if (P.peekToken().is(TokenKind::Comma)) {
                    if (P.nextToken().is(TokenKind::RSquare)) {
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
        TokenKind op = P.peekToken().getType();
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
        if (P.peekToken().isNot(TokenKind::RParen)) {
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
        if (P.nextToken().isNot(TokenKind::RParen))
            while (true) {
                if (P.peekToken().is(TokenKind::Eof)) {
                    llvm::outs() << "Unclosed function call. Expected ')'.\n";
                    return nullptr;
                }

                Expr* next = parseExpr();
                if (!next) return nullptr;

                args.push_back(next);

                if (P.peekToken().is(TokenKind::RParen)) {
                    break;
                } else if (P.peekToken().is(TokenKind::Comma)) {
                    if (P.nextToken().is(TokenKind::RParen)) {
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
        assert(P.peekToken().is(TokenKind::Identifier));
        llvm::StringRef name = P.peekToken().getData();
        SrcSpan start_span = P.peekToken().getSpan();

        Expr* base;
        if (P.nextToken().isNot(TokenKind::LParen)) {
            base = Alloc.Create<DeclRefExpr>(start_span, name);
        } else
            base = parseCallExpr(name, start_span.getStart());

        if (P.peekToken().is(TokenKind::Dot)) {
            if (P.nextToken().isNot(TokenKind::Identifier)) {
                llvm::outs() << "Expected identifier.\n";
                return nullptr;
            }

            llvm::StringRef field_name = P.peekToken().getData();
            SrcSpan span(start_span.getStart(), P.peekToken().getEndLoc());

            (void)P.nextToken();
            return Alloc.Create<AccessExpr>(span, base, field_name);
        } else if (P.peekToken().is(TokenKind::LSquare)) {
            (void)P.nextToken();
            Expr* start = parseIntegerLiteral();
            if (P.peekToken().isNot(TokenKind::RSquare)) {
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
                llvm::outs() << "Currently: " << lex::strTokenKind(P.peekToken().getType()) << "\n";
                return nullptr;

            case TokenKind::Type:
                return parseTypeAiasDecl();

            case TokenKind::Const:
            case TokenKind::Let:
            case TokenKind::Mut:
                return parseVarDecl();

            case TokenKind::Struct:
                return parseStructDecl();

            case TokenKind::Fn:
                return parseFunctionDecl();
        }
    }

    Stmt* parseStmt() {
        DEBUG("Called: parseStmt");
        switch (P.peekToken().getType()) {
            case TokenKind::Type: {
                ast::TypeAliasDecl* DC = parseTypeAiasDecl();
                return Alloc.Create<DeclStmt>(DC);
            }

            case TokenKind::Const:
            case TokenKind::Let:
            case TokenKind::Mut: {
                ast::VarDecl* DC = parseVarDecl();
                return Alloc.Create<DeclStmt>(DC);
            }

            case TokenKind::If:
                return parseIfStmt();
            case TokenKind::While:
                return parseWhileStmt();
            case TokenKind::Ret:
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
        if (P.peekToken().is(TokenKind::LSquare)) {
            SrcLoc start = P.peekToken().getStartLoc();

            (void)P.nextToken();
            TypeInfo* pointee = parseTypeRef();

            size_t size = 0;
            if (P.peekToken().is(TokenKind::Colon)) {
                if (P.nextToken().isNot(TokenKind::Integer)) {
                    llvm::outs() << "Expected an integer literal for the size of the array.\n";
                    return nullptr;
                }

                if (P.peekToken().getData().getAsInteger(10, size))
                    // Indicates an error probably in the lexer with how integer literals are
                    // matched or with how the tokens are constructed.
                    llvm_unreachable("'Data' for integer literal token was not an integer literal.");

                if (P.nextToken().isNot(TokenKind::RSquare)) {
                    llvm::outs() << "Expected ']' to close the array type.\n";
                    return nullptr;
                }
            } else if (!P.peekToken().is(TokenKind::RSquare)) {
                llvm::outs() << "Invalid array type, expected ':' or ']'.\n";
                return nullptr;
            }

            SrcSpan span(start, P.peekToken().getEndLoc());

            (void)P.nextToken();
            return Alloc.Create<TypeInfo>(TypeInfo::CreateArray(span, pointee, size));
        } else if (P.peekToken().isNot(TokenKind::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }

        auto result = Alloc.Create<TypeInfo>(TypeInfo::CreateNamed(
            P.peekToken().getSpan(), P.peekToken().getData()
        ));

        (void)P.nextToken();
        if (P.peekToken().is(TokenKind::Star) ||
            P.peekToken().is(TokenKind::Ampersand)) {
            SrcLoc start = result->getStartLoc();
            do {
                result = Alloc.Create<TypeInfo>(TypeInfo::CreatePointer(
                    SrcSpan(start, P.peekToken().getEndLoc()),
                    P.peekToken().is(TokenKind::Star) ? true : false,
                    result
                ));
                (void)P.nextToken();
            } while (P.peekToken().is(TokenKind::Star) ||
                     P.peekToken().is(TokenKind::Ampersand));
        }

        return result;
    }

    TypeAliasDecl* parseTypeAiasDecl() {
        DEBUG("Called: parseTypeAliasDecl");
        SrcLoc start = P.peekToken().getStartLoc();
        if (P.nextToken().isNot(TokenKind::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();

        if (P.nextToken().isNot(TokenKind::Equal)) {
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
        if (P.peekToken().isNot(TokenKind::Identifier)) {
            llvm::outs() << "Expected identifier, got '" << lex::strTokenKind(P.peekToken().getType()) <<"'.\n";
            return true;
        }
        name = P.peekToken().getData();

        // Only used for the else case to create an SrcLoc for the implicit type.
        size_t colon_offset = P.peekToken().getEndLoc().getOffset() + 1;

        if (P.nextToken().is(TokenKind::Colon)) {
            (void)P.nextToken();
            info = parseTypeRef();
            if (!info) return true;
        } else if (!P.peekToken().is(TokenKind::Equal)) {
            llvm::outs() << "Expected ':' or '='.\n";
            return true;
        } else
            info = Alloc.Create<TypeInfo>(TypeInfo::CreateImplicit(
                SrcLoc(P.peekToken().getSpan().getID(), colon_offset)
            ));

        if (P.peekToken().is(TokenKind::Equal)) {
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
        if (P.peekToken().is(TokenKind::Const)) {
            mutability = VarDecl::Constant;
        } else if (P.peekToken().is(TokenKind::Let)) {
            mutability = VarDecl::Immutable;
        } else if (P.peekToken().is(TokenKind::Mut)) {
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
        if (P.nextToken().isNot(TokenKind::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();

        if (P.nextToken().isNot(TokenKind::Colon)) {
            llvm::outs() << "Expected ':'.\n";
            return nullptr;
        }

        (void)P.nextToken();
        StructDecl* result = Alloc.Create<StructDecl>(name);
        llvm::SmallVector<FieldDecl*> fields;
        // while (P.nextToken().isNot(TokenType::End)) {
        while (P.peekToken().isNot(TokenKind::End)) {
            if (P.peekToken().is(TokenKind::Eof)) {
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
        if (P.nextToken().isNot(TokenKind::Identifier)) {
            llvm::outs() << "Expected identifier.\n";
            return nullptr;
        }
        llvm::StringRef name = P.peekToken().getData();

        if (P.nextToken().isNot(TokenKind::LParen)) {
            llvm::outs() << "Expected '('.\n";
            return nullptr;
        }

        FunctionDecl* result = Alloc.Create<FunctionDecl>(name);
        llvm::SmallVector<ParamDecl*> parameters;
        if (P.nextToken().isNot(TokenKind::RParen))
            while (true) {
                if (P.peekToken().is(TokenKind::Eof)) {
                    llvm::outs() << "Unclosed function declaration. Expected ')'.\n";
                    return nullptr;
                }

                ParamDecl* next = parseParamDecl();
                if (!next) return nullptr;

                parameters.push_back(next);
                result->pushDecl(next);

                if (P.peekToken().is(TokenKind::RParen)) {
                    break;
                } else if (P.peekToken().is(TokenKind::Comma))
                    if (P.nextToken().is(TokenKind::RParen)) {
                        llvm::outs() << "Unnessesary ',' after the last parameter in a function declaration.\n";
                        return nullptr;
                    }
            }

        // Ends the function declaration at the closing ")" in case
        // in case the function doesn't have a return type.
        SrcLoc end = P.peekToken().getEndLoc();

        result->setParams(parameters);
        if (P.nextToken().is(TokenKind::Arrow)) {
            (void)P.nextToken();
            TypeInfo* type = parseTypeRef();
            if (!type) return nullptr;

            end = P.peekToken().getEndLoc();
            result->setTypeInfo(type);
        }

        result->setSpan(SrcSpan(start, end));

        if (P.peekToken().is(TokenKind::Colon)) {
            BlockStmt* body = parseBlockStmt();
            if (!body) return nullptr;

            result->setBody(body);
        }

        return result;
    }

    BlockStmt* parseBlockStmt() {
        DEBUG("Called: parseBlockStmt");
        assert(P.peekToken().is(TokenKind::Colon));

        SrcLoc start = P.peekToken().getStartLoc();

        (void)P.nextToken();
        llvm::SmallVector<Stmt*> stmts;
        while (P.peekToken().isNot(TokenKind::End)) {
            // FIXME: I don't like this setup, but it works for now...
            if (P.peekToken().is(TokenKind::Elif) ||
                P.peekToken().is(TokenKind::Else)) {
                SrcSpan span(start, P.peekToken().getEndLoc());
                return Alloc.Create<BlockStmt>(span, stmts);
            }

            if (P.peekToken().is(TokenKind::Eof)) {
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
        if (P.peekToken().is(TokenKind::Colon))
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
        if (P.peekToken().is(TokenKind::Elif)) {
            elseif = parseIfStmt();
            if (!elseif) return nullptr;
        } else if (P.peekToken().is(TokenKind::Else)) {
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
        if (!P.peekToken().isKeyword() && !P.peekToken().is(TokenKind::Self)) {
            value = parseExpr();
            end = value->getEndLoc();
        }

        return Alloc.Create<ReturnStmt>(SrcSpan(start, end), value);
    }
};

namespace c {

const Token& Parser::nextToken() {
    CurrentToken = LexerSource.lex();
    DEBUG(std::string("Called: nextToken:")
          + " Kind: " + lex::strTokenKind(CurrentToken.getType()).str()
          + " Data: " + CurrentToken.getData().str());
    return CurrentToken;
}

bool Parser::parse(ModuleDecl& result) {
    DEBUG("Called: parse");
    if (peekToken().is(TokenKind::Eof))
        return true;

    PrattParser pratt(*this, Alloc);
    RecurseiveDescentParser recurse(*this, pratt, Alloc);

    do {
        ast::Decl* next = recurse.parseDecl();
        if (!next) return true;

        result.pushDecl(next);
    } while (!peekToken().is(TokenKind::Eof));
    return false;
}

} // namespace c
