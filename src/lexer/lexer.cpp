#include "include/lexer/lexer.hpp"

#include "include/lexer/utils.hpp"

using namespace c;

// class Lexer
ExpectSpan<Token> Lexer::next() {
    if (eof()) 
        return Token{TokenType::Eof, m_loc};
    else skip_trivia();

    if (lexer::is_ident_start(peek())) {
        return match_identifier();
    } else if (lexer::is_digit(peek())) {
        ExpectSpan<Token> result = match_number();
        if (!result) return result.takeError();
        return result.get();
    } else if (false) {
        ExpectSpan<Token> result = match_operator();
        if (!result) return result.takeError();
        return result.get();
    } else if (peek() == '"' || peek() == '\'') {
        ExpectSpan<Token> result = match_literal();
        if (!result) return result.takeError();
        return result.get();
    }

    TokenType type = match_parentheses();
    if (type != TokenType::Unknown) {
        tb::Location location = loc();
        if (!eof()) advance(1);
        return Token{type, location};
    }

    // TODO: Implement error logic
}

Token Lexer::match_identifier() {
    size_t start_offset = offset();
    size_t start_column = column();

    do {
        advance(1);
    } while (!eof() && lexer::is_ident(peek()));

    llvm::StringRef lexeme = m_src.slice(start_offset, offset());

    TokenType type = TokenType::Identifier;
    auto keywords = lexer::get_keywords();
    if (keywords.contains(lexeme))
        type = keywords.find(lexeme)->second;

    tb::Location start{start_offset, line(), start_column};
    return Token{type, std::move(start), loc()};
}

ExpectSpan<Token> Lexer::match_number() {
    size_t start_offset = offset();
    size_t start_column = column();

    do {
        advance(1);
    } while (!eof() && lexer::is_digit(peek()));

    TokenType type = TokenType::Int;
    if (peek() == '.') {
        if (eof()) {
            // TODO: Implement error return logic
        }

        do {
            advance(1);
        } while (!eof() && lexer::is_digit(peek()));
        type = TokenType::Float;
    }

    tb::Location start{start_offset, line(), start_column};
    return Token{type, std::move(start), loc()};
}

ExpectSpan<Token> Lexer::match_literal() {
    size_t start_offset = offset();
    size_t start_column = column();
    
    TokenType type;
    if (peek() == '\'') {
        if (in_bounds(offset() + 2)) { // Size of a character literal is 3 ('c'), so current pos (1) plus 2
            // TODO: Implement error return logic
        }

        type = TokenType::Character;
        advance(2);
    } else { // peek() == '"'
        do {
            advance(1);
            if (eof() || peek() == '\n') { // " was never closed
                // TODO: Implement error return logic
            }
        } while (peek() != '"');
        type = TokenType::String;
    }

    tb::Location start{start_offset, line(), start_column};
    return Token{type, std::move(start), loc()};
}

ExpectSpan<Token> Lexer::match_operator() {
    size_t start_offset = offset();
    size_t start_column = column();

    TokenType type = TokenType::Unknown;
    for (const auto& kv_pair : lexer::get_operators()) {
        auto key = kv_pair.getKey();
        if (in_bounds(column() + key.size()) && m_src.substr(column(), key.size()) == key) {
            
        }
    }
}

TokenType Lexer::match_parentheses() const {
    switch (peek()) {
        case '(': return TokenType::LParen;
        case ')': return TokenType::RParen;
        case '[': return TokenType::LBrack;
        case ']': return TokenType::RBrack;
        case '{': return TokenType::LBrace;
        case '}': return TokenType::RBrace;
        default: return TokenType::Unknown;
    }
}

void Lexer::skip_trivia() {
    for (; !eof(); advance(1)) {
        if (peek() == '\n') {
            advance_line();
        } else if (lexer::is_space(peek()))
            advance(1);
    }
}
// Lexer
