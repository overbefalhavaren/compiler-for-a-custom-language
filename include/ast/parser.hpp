#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/IR/BasicBlock.h"

#include "include/lexer/lexer.hpp"
#include "include/ast/ast.hpp"

namespace c {

class Parser {
public:
    // using DeclPtr = std::unique_ptr<ast::Decl>; // TODO: Will eventually be reinstated when more advanced functionality is added
    using ExprPtr = std::unique_ptr<ast::Expr>;
private:
    Lexer lexer;
    std::unique_ptr<Token> tok;
public:
    explicit Parser(Lexer&& lexer) : lexer(std::move(lexer)), tok(std::make_unique<Token>(lexer.next())) {}
    ~Parser() = default;

    std::optional<ExprPtr> next() {
        switch (peekToken().type) {
            case TokenType::Const:
            case TokenType::Let:
            case TokenType::Mut:
                return makeOptExprPtr<ast::Variable>(parseVariable());
            case TokenType::Identifier: {
                // FIXME:
                // Right now binary operators will not work if RHS or LHS is a literal,
                // a call or an attribute access. Fix in a future update
                ExprPtr lhs = makeExprPtr<ast::VariableRef>(peekToken().lexeme);
                if (!ast::isBinaryOp(nextToken().type))
                    return lhs;

                TokenType op = peekToken().type;
                if (nextToken().type == TokenType::Identifier) {
                    ExprPtr rhs = makeExprPtr<ast::VariableRef>(peekToken().lexeme);
                    return makeExprPtr<ast::BinaryOp>(ast::BinaryOp(
                        op, std::move(lhs), std::move(rhs)
                    ));
                }

                llvm::outs() << "Expected a binary operator.\n";
                return std::nullopt;
            }
            case TokenType::Int:
                return makeExprPtr<ast::IntLiteral>(parseInt(peekToken().lexeme));
            case TokenType::Float:
                return makeExprPtr<ast::FloatLiteral>(parseFloat(peekToken().lexeme));

            // Works as the default case by catching all error tokens. Does not emit 
            // any errors since they are already emitted by the lexer.
            case TokenType::Unknown:
                return std::nullopt;

            // TODO: Temporary default to catch any tokens that are defined but unsupported
            default:
                llvm::outs() << "Unsupported token type: '" << strTokenType(peekToken().type) << "'\n";
                return std::nullopt;
        }
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

    bool skipUntil(TokenType type, bool consume_match = true) {
        do {
            (void)nextToken();
        } while (!checkToken(type) && !checkToken(TokenType::Eof));
        if (checkToken(TokenType::Eof))
            return false;
        if (consume_match)
            (void)nextToken();
        return true;
    }

    bool skipUntil(llvm::ArrayRef<TokenType> types, bool consume_match) {
        bool is_found = false;
        do {
            (void)nextToken();
            for (size_t i = 0; i < types.size(); i++)
                if (checkToken(types[i])) {
                    is_found = true;
                    break;
                }
        } while (!is_found && !checkToken(TokenType::Eof));
        if (checkToken(TokenType::Eof))
            return false;
        if (consume_match)
            (void)nextToken();
        return true;
    }
    
private:
    // Return an immutable reference of the current token.
    inline const Token& peekToken() { return *tok; }

    // Advance to the next token and return an immutable reference to it.
    const Token& nextToken() {
        *tok = lexer.next();
    }

    // Return bool if the current token is of the specified type.
    inline bool checkToken(TokenType type) {
        return peekToken().type == type;
    }

    // Check if the current token is of a certain type and advance if true.
    inline bool consumeToken(TokenType type) {
        if (checkToken(type)) {
            (void)nextToken(); 
            return true;
        }
        return false;
    }

    template <typename ExprType>
    inline ExprPtr makeExprPtr(ExprType decl) {
        return std::make_unique<ExprType>(std::move(decl.value()));
    }

    template <typename ExprType>
    inline std::optional<ExprPtr> makeOptExprPtr(std::optional<ExprType> decl) {
        if (decl.has_value())
            return std::make_unique<ExprType>(std::move(decl.value()));
        return std::nullopt;
    }

    size_t parseInt(llvm::StringRef lexeme) {

    }

    float parseFloat(llvm::StringRef lexeme) {

    }

    std::optional<ast::Variable> parseVariable() {
        TokenType kind = peekToken().type; // The leading token of a variable
        (void)nextToken();

        llvm::StringRef name = peekToken().lexeme; // TODO: Deprecate "lexeme"
        if (!consumeToken(TokenType::Identifier)) {
            llvm::outs() << "Expected Identifier\n"; 
            return std::nullopt;
        }

        // TODO: Implement type system and matching
        // if (consumeToken(TokenType::Colon)) {
            
        // }

        // TODO: Match type here

        if (consumeToken(TokenType::Equal)) {
            // FIXME: Implement parsing of the value of the variable

            // return ast::Variable();
        }
        return ast::Variable(kind, name);
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
