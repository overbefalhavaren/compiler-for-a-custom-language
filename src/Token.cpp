#include "include/Lexer/Token.hpp"

namespace c {

llvm::StringRef strTokenType(TokenType type) {
    switch (type) {
        case TokenType::Unknown:    return "Unknown";
        case TokenType::Eof:        return "Eof";

        case TokenType::Identifier: return "Identifier";
        case TokenType::Int:        return "Int";
        case TokenType::Float:      return "Float";
        case TokenType::String:     return "String";
        case TokenType::Character:  return "Character";

        case TokenType::Const:      return "Const";
        case TokenType::Let:        return "Let";
        case TokenType::Mut:        return "Mut";
        case TokenType::Type:       return "Type";
        case TokenType::Self:       return "Self";
        
        case TokenType::End:        return "End";

        case TokenType::If:         return "If";
        case TokenType::Elif:       return "Elif";
        case TokenType::Else:       return "Else";
        case TokenType::Match:      return "Match";
        case TokenType::Case:       return "Case";

        case TokenType::While:      return "While";
        case TokenType::For:        return "For";
        case TokenType::Break:      return "Break";

        case TokenType::Fn:         return "Fn";
        case TokenType::Return:     return "Return";

        case TokenType::Enum:       return "Enum";
        case TokenType::Struct:     return "Struct";
        case TokenType::Impl:       return "Impl";
        case TokenType::Trait:      return "Trait";

        case TokenType::Import:     return "Import";

        case TokenType::Plus:       return "Plus";
        case TokenType::Minus:      return "Minus";
        case TokenType::Star:       return "Star";
        case TokenType::Slash:      return "Slash";

        case TokenType::PlusPlus:   return "PlusPlus";
        case TokenType::MinusMinus: return "MinusMinus";

        case TokenType::PlusEqual:  return "PlusEqual";
        case TokenType::MinusEqual: return "MinusEqual";
        case TokenType::StarEqual:  return "StarEqual";
        case TokenType::SlashEqual: return "SlashEqual";

        case TokenType::EqualEqual: return "DoubleEqual";
        case TokenType::ExclEqual:  return "NotEqual";
        case TokenType::MoreEqual:  return "MoreEqual";
        case TokenType::LessEqual:  return "LessEqual";

        case TokenType::AmpAmp:     return "AmpAmp";
        case TokenType::PipePipe:   return "PipePipe";
        case TokenType::BitXOR:     return "BitXOR";

        case TokenType::Exclamation:return "Exclamation";
        case TokenType::Equal:      return "Equal";
        case TokenType::Ampersand:  return "Ampersand";
        case TokenType::Pipe:       return "Pipe";

        case TokenType::Colon:      return "Colon";
        case TokenType::ColonColon: return "ColonColon";
        case TokenType::Comma:      return "Comma";
        case TokenType::Dot:        return "Dot";

        case TokenType::Arrow:      return "Arrow";

        case TokenType::LShift:     return "LShift";
        case TokenType::RShift:     return "RShift";

        case TokenType::More:       return "More";
        case TokenType::Less:       return "Less";

        case TokenType::LParen:     return "LParen";
        case TokenType::RParen:     return "RParen";

        case TokenType::LBrace:     return "LBrace";
        case TokenType::RBrace:     return "RBrace";

        case TokenType::LBrack:     return "LBrack";
        case TokenType::RBrack:     return "RBrack";

        default:
            assert(false && "Unknown token can't be converted to string.");
            return ""; // To avoid compiler warning
    }
}

bool Token::isLiteral() const {
    switch (Type) {
        default:
            return false;
        case TokenType::Int:
        case TokenType::Float:
        case TokenType::String:
        case TokenType::Character:
            return true;
    }
}

bool Token::isKeyword() const {
    switch (Type) {
        default: 
            return false;
        case TokenType::Const:
        case TokenType::Let:
        case TokenType::Mut:
        case TokenType::Type:
        case TokenType::Self:
        case TokenType::Pub:
        case TokenType::From:
        case TokenType::End:
        case TokenType::If:
        case TokenType::Elif:
        case TokenType::Else:
        case TokenType::Match:
        case TokenType::Case:
        case TokenType::While:
        case TokenType::For:
        case TokenType::Break:
        case TokenType::Fn:
        case TokenType::Return:
        case TokenType::Enum:
        case TokenType::Struct:
        case TokenType::Impl:
        case TokenType::Trait:
        case TokenType::Import:
            return true;
    }
}

bool Token::isOperator() const {
    switch (Type) {
        default:
            return false;
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::PlusPlus:
        case TokenType::MinusMinus:
        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::StarEqual:
        case TokenType::SlashEqual:
        case TokenType::EqualEqual:
        case TokenType::ExclEqual:
        case TokenType::MoreEqual:
        case TokenType::LessEqual:
        case TokenType::AmpAmp:
        case TokenType::PipePipe:
        case TokenType::Exclamation:
        case TokenType::Equal:
        case TokenType::Ampersand:
        case TokenType::Pipe:
        case TokenType::Less:
        case TokenType::More:
        case TokenType::LShift:
        case TokenType::RShift:
            return true;
    }
}

} // namespace c
