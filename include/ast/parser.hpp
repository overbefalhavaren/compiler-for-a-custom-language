#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/IR/BasicBlock.h"

#include "include/lexer/lexer.hpp"
#include "include/ast/ast.hpp"

namespace c {

// TODO: Remake and restructure the Parser class
class Parser {
public:
    // using DeclPtr = std::unique_ptr<ast::Decl>; // TODO: Will eventually be reinstated when more advanced functionality is added
    using ExprPtr = std::unique_ptr<ast::Expr>;
private:
    Lexer lexer;
    std::unique_ptr<Token> tok;
public:
    explicit Parser(Lexer&& lexer) : lexer(std::move(lexer)), tok(std::make_unique<Token>()) {
        (void)nextToken();
    }
    ~Parser() = default;

    inline bool eof() const { return tok->is(TokenType::Eof); }

    std::optional<ExprPtr> next() {
        // llvm::outs() << "CURRENT: " << llvm::StringRef(peekToken().str()) << "\n";

        std::unique_ptr<ast::Expr> lhs; // 
        switch (peekToken().type) {
            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut: 
                return parseVariable();

            // All folowing cases allow for use of a binary operator.
            case TokenType::Identifier:
                lhs = makeExprPtr<ast::VariableRef>(peekToken().lexeme);
                break;
            case TokenType::Int: // { // NOTE: Temporarily make Int and Float both be Float
            //     int64_t value;
            //     // Should not give error since the lexer has already validated it
            //     (void)peekToken().lexeme.getAsInteger(10, value);
            //     lhs = makeExprPtr<ast::IntLiteral>(std::move(value));
            //     break;
            // }
            case TokenType::Float: {
                double value;
                // Should not give error since the lexer has already validated it

                // NOTE:
                // When adding support for expressions like "1e-5" this will
                // probably have to be updated
                (void)peekToken().lexeme.getAsDouble(value);
                lhs = makeExprPtr<ast::FloatLiteral>(std::move(value));
                break;
            }

            case TokenType::Eof:
                return std::nullopt;
            
            // Works as the default case by catching all error tokens. Does not emit 
            // any errors since they are already emitted by the lexer.
            case TokenType::Unknown:
                return std::nullopt;

            // NOTE: Temporary default to catch any tokens that are defined but unsupported
            default:
                llvm::outs() << "Unsupported token type: '" << peekToken().str() << "'\n";
                assert(false && "Unsupported token type.");
                // llvm::outs() << "Unsupported token type: '" << strTokenType(peekToken().type) << "'\n";
                // return std::nullopt;
        }

        if (!nextToken().isOperator()) 
            return lhs;

        TokenType op = peekToken().type;

        (void)nextToken();
        std::optional<std::unique_ptr<ast::Expr>> rhs = next();
        if (!rhs.has_value())
            return std::nullopt;
        
        auto expr = ast::BinaryOp(op, std::move(lhs), std::move(rhs.value()));
        return makeExprPtr<ast::BinaryOp>(std::move(expr));
    }

    // TODO: Top level declaration not yet supported
    // std::optional<DeclPtr> parseTopLevelDecl() {
    //     assert(false && "Top level declaration not yet supported");
    //     switch (peekToken().type) {
    //         case TokenType::Const:
    //         case TokenType::Let:
    //         case TokenType::Mut: 
    //             return makeOptExprPtr<ast::Variable>(parseVariable());
    //         case TokenType::Fn: 
    //             return makeOptDeclPtr<ast::Function>(parseFunction());
    //         default:
    //             llvm::outs() << "Expected a declaration, got: " << strTokenType(peekToken().type) << "\n";
    //             return std::nullopt;
    //     }
    // }

    // bool skipUntil(TokenType type, bool consume_match = true) {
    //     do {
    //         (void)nextToken();
    //     } while (!checkToken(type) && !checkToken(TokenType::Eof));
    //     if (checkToken(TokenType::Eof))
    //         return false;
    //     if (consume_match)
    //         (void)nextToken();
    //     return true;
    // }

    // bool skipUntil(llvm::ArrayRef<TokenType> types, bool consume_match) {
    //     bool is_found = false;
    //     do {
    //         (void)nextToken();
    //         for (size_t i = 0; i < types.size(); i++)
    //             if (checkToken(types[i])) {
    //                 is_found = true;
    //                 break;
    //             }
    //     } while (!is_found && !checkToken(TokenType::Eof));
    //     if (checkToken(TokenType::Eof))
    //         return false;
    //     if (consume_match)
    //         (void)nextToken();
    //     return true;
    // }
    
private:
    // Return an immutable reference of the current token.
    inline const Token& peekToken() { return *tok; }

    // Advance to the next token and return an immutable reference to it.
    const Token& nextToken() {
        *tok = lexer.next();
        return *tok;
    }

    // Check if the current token is of a certain type and advance if true.
    inline bool consumeToken(TokenType type) {
        if (peekToken().is(type)) {
            (void)nextToken(); 
            return true;
        }
        return false;
    }

    template <typename ExprType>
    inline ExprPtr makeExprPtr(ExprType decl) {
        static_assert(std::is_base_of_v<ast::Expr, ExprType>, "ExprType must derive from ast::Expr.");
        return std::make_unique<ExprType>(std::move(decl));
    }

    template <typename ExprType>
    inline std::optional<ExprPtr> makeOptExprPtr(std::optional<ExprType> decl) {
        if (decl.has_value())
            return makeExprPtr<ExprType>(std::move(decl.value()));
        return std::nullopt;
    }

    std::optional<std::unique_ptr<ast::Expr>> parseVariable() {
        TokenType mutability = peekToken().type;
        
        llvm::StringRef name = nextToken().lexeme;
        if (!peekToken().is(TokenType::Identifier)) {
            llvm::outs() << "Expected Identifier: " << peekToken().str() << "\n";
            return std::nullopt;
        }

        // TODO: Implement type system and matching
        // TODO: Match type here

        std::unique_ptr<ast::Expr> value = nullptr;
        if (nextToken().is(TokenType::Equal)) {
            (void)nextToken();
            std::optional<std::unique_ptr<ast::Expr>> v = next();
            if (!v.has_value())
                return std::nullopt;
            value = std::move(v.value());
        }

        ast::Variable result(mutability, name, std::move(value));
        return makeExprPtr<ast::Variable>(std::move(result));
    }

    // TODO: Functions are not yet supported
    /*std::optional<ast::Function> parseFunction() {
        (void)nextToken(); // Skip the leading "Fn" token (the "fn" keyword)
        llvm::StringRef name = peekToken().lexeme; // TODO: Deprecate "lexeme"
        if (!consumeToken(TokenType::Identifier)) {
            llvm::outs() << "Expected Identifier\n"; 
            return std::nullopt;
        }

        if (!consumeToken(TokenType::LParen)) {
            llvm::outs() << "Expected '('\n"; 
            return std::nullopt;
        }

        llvm::SmallVector<ast::Variable, 0> args;
        if (!checkToken(TokenType::RParen)) {
            args.reserve(3);
            do {
                std::optional<ast::Variable> arg = parseVariable();
                if (arg.has_value()) {
                    args.push_back(std::move(arg.value()));
                } else return std::nullopt;

                if (args.size() == args.capacity()) 
                    args.reserve(3);

                (void)nextToken();
                if (checkToken(TokenType::Colon)) {
                    (void)nextToken();
                } else if (!checkToken(TokenType::RParen)){
                    llvm::outs() << "Expected ',' or ')'\n";
                    return std::nullopt;
                }
            } while (!checkToken(TokenType::RParen));

            if (args.size() < args.capacity())
                args.resize(args.size());
        }

        (void)nextToken(); // Skip the closing ')'

        // TODO: Implement type system and matching

        // TODO: Match function return type here.
        // Syntax: -> TYPE

        // FIXME: Implement a way to skip over the function 
        // body and store it as a StringRef

        llvm::StringRef body;
        if (consumeToken(TokenType::LBrace)) {
            // FIXME:
            // For now we'll use braces ('{' and '}') for scopes but eventually 
            // we'll use ':' for the start of the scope and "end" for the end

            // TODO: Find better way of doing this, for example using spans and a stream manager
            const char* start = peekToken().lexeme.data();
            llvm::StringRef end_lexeme = peekToken().lexeme;

            // Skip function body
            uint8_t nested = 1;
            do {
                switch (peekToken().type) {
                    case TokenType::LBrace: { nested++; }
                    case TokenType::RBrace: { nested--; }
                    case TokenType::Eof:
                        llvm::outs() << "Unclosed '{'\n";
                        return std::nullopt;

                    default:
                        end_lexeme = peekToken().lexeme;
                }
                (void)nextToken();
            } while (nested > 0);
            
            body = llvm::StringRef(start, end_lexeme.end() - start);
        }

        return ast::Function(name, std::move(args), body);
    }*/
};

} // namespace c
