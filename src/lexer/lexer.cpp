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

    return {tb::DiagID::ERROR_invalid_character, loc()};
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
    tb::Location start_loc = loc();

    do {
        advance(1);
    } while (!eof() && lexer::is_digit(peek()));

    TokenType type = TokenType::Int;
    if (peek() == '.') {
        if (eof()) return {tb::DiagID::ERROR_incomplete_float,
                           {std::move(start_loc), loc()}};
        
        do {
            advance(1);
        } while (!eof() && lexer::is_digit(peek()));
        type = TokenType::Float;
    }

    return Token{type, std::move(start_loc), loc()};
}

ExpectSpan<Token> Lexer::match_literal() {
    tb::Location start_loc = loc();
    
    TokenType type;
    if (peek() == '\'') {
        if (in_bounds(offset() + 2)) // Size of a character literal is 3 ('c'), so current pos (1) plus 2
            return {tb::DiagID::ERROR_unclosed_character_literal,
                    {std::move(start_loc), loc()}};
        

        type = TokenType::Character;
        advance(2);
    } else { // peek() == '"'
        do {
            advance(1);
            if (eof() || peek() == '\n')  // " was never closed
                return {tb::DiagID::ERROR_unclosed_string_literal,
                        {std::move(start_loc), loc()}};
        } while (peek() != '"');
        type = TokenType::String;
    }

    return Token{type, std::move(start_loc), loc()};
}

// FIXME: Improve logic
ExpectSpan<Token> Lexer::match_operator() {
    tb::Location start_loc = loc();
    TokenType type = TokenType::Unknown;
    switch (peek()) {
        case '-':
            if (!eof()) {
                if (m_src[offset() + 1] == '-') {
                    type = TokenType::MinusMinus;
                } else if (m_src[offset() + 1] == '=') {
                    type = TokenType::MinusEqual;
                } else if (m_src[offset() + 1] == '>') {
                    type = TokenType::RArrow;
                } else type = TokenType::Minus;
            } else type = TokenType::Minus;
        case '+':
            if (!eof()) {
                if (m_src[offset() + 1] == '+') {
                    type = TokenType::PlusPlus;
                } else if (m_src[offset() + 1] == '=') {
                    type = TokenType::PlusEqual;
                } else type = TokenType::Plus;
            } else type = TokenType::Plus;
        case '<':
            if (!eof()) {
                if (m_src[offset() + 1] == '=') {
                    type = TokenType::LTEqual;
                } else if (m_src[offset() + 1] == '<') {
                    type = TokenType::LShift;
                } else type = TokenType::LAngle;
            } else type = TokenType::LAngle;
        case '>':
            if (!eof()) {
                if (m_src[offset() + 1] == '=') {
                    type = TokenType::MTEqual;
                } else if (m_src[offset() + 1] == '>') {
                    type = TokenType::RShift;
                } else type = TokenType::RAngle;
            } else type = TokenType::RAngle;
        case '!':
            if (!eof()) {
                if (m_src[offset() + 1] == '=') {
                    type = TokenType::NotEqual;
                } else if (m_src[offset() + 1] == '|') {
                    type = TokenType::BitXOR;
                } else type = TokenType::Exclamation;
            } else type = TokenType::Exclamation;
        case '*':
            if (!eof() && m_src[offset() + 1] == '=') {
                type = TokenType::TimesEqual;
            } else type = TokenType::Star;
        case '/':
            if (!eof() && m_src[offset() + 1] == '=') {
                type = TokenType::DivideEqual;
            } else type = TokenType::Slash;
        case '=':
            if (!eof() && m_src[offset() + 1] == '=') {
                type = TokenType::DoubleEqual;
            } else type = TokenType::Equal;
        case '&':
            if (!eof() && m_src[offset() + 1] == '&') {
                type = TokenType::LogAND;
            } else type = TokenType::Ampersand;
        case '|':
            if (!eof() && m_src[offset() + 1] == '|') {
                type = TokenType::LogOR;
            } else type = TokenType::Pipe;
        case ':':
            if (!eof() && m_src[offset() + 1] == ':') {
                type = TokenType::DoubleColon;
            } else type = TokenType::Colon;
        case ',': type = TokenType::Comma; break;
        case '.': type = TokenType::Dot; break;
    }

    if (type == TokenType::Unknown) 
        return {tb::DiagID::ERROR_invalid_operator,
                {std::move(start_loc), loc()}};
    return Token{type, std::move(start_loc), loc()};
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
