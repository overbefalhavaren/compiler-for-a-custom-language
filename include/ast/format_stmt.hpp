#pragma once

#include "include/ast/stmt.hpp"

namespace c {
namespace ast {
namespace fmt {

// Even more forward declarations because fuck MSVC
std::string getIndent(size_t indent);
std::string stringOperator(TokenType Op);
std::string formatBlockStmt(const BlockStmt& stmt, size_t indent);
std::string formatCastStmt(const CastStmt& stmt, size_t indent);
std::string formatReturnStmt(const ReturnStmt& stmt, size_t indent);
std::string formatIfStmt(const IfStmt& stmt, size_t indent, bool isElseIf = false);
std::string formatWhileStmt(const WhileStmt& stmt, size_t indent);
std::string formatTypeDecl(const TypeDecl& stmt, size_t indent);
std::string formatVarDecl(const VarDecl& stmt, size_t indent);
std::string formatFunctionDecl(const FunctionDecl& stmt, size_t indent);
std::string formatImplDecl(const ImplDecl& stmt, size_t indent, bool isStructImpl = false);
std::string formatStructDecl(const StructDecl& stmt, size_t indent);
std::string formatCallExpr(const CallExpr& stmt, size_t indent);
std::string formatMoveExpr(const MoveExpr& stmt, size_t indent);
std::string formatNamedTypeExpr(const NamedTypeExpr& stmt, size_t indent);
std::string formatPointerTypeExpr(const PointerTypeExpr& stmt, size_t indent);
std::string formatTemplateTypeExpr(const TemplateTypeExpr& stmt, size_t indent);
std::string formatVarRef(const VarRef& stmt, size_t indent);
std::string formatImplRef(const ImplRef& stmt, size_t indent);
std::string formatBinaryOperator(const BinaryOperator& stmt, size_t indent);
std::string formatUnaryOperator(const UnaryOperator& stmt, size_t indent);
std::string formatIntLiteral(const IntLiteral& stmt, size_t indent);
std::string formatFloatLiteral(const FloatLiteral& stmt, size_t indent);
std::string formatStringLiteral(const StringLiteral& stmt, size_t indent);

static std::string IndentString = "  ";

std::string format(const ast::Stmt& stmt, size_t indent) {    
    std::string result;
    switch (stmt.getStmtClass()) {
        case Stmt::SC_BlockStmt:
            result.append(formatBlockStmt(cast<BlockStmt>(stmt), indent));
            break;

        case Stmt::SC_CastStmt:
            result.append(formatCastStmt(cast<CastStmt>(stmt), indent));
            break;

        case Stmt::SC_ReturnStmt:
            result.append(formatReturnStmt(cast<ReturnStmt>(stmt), indent));
            break;
        
        case Stmt::SC_IfStmt:
            result.append(formatIfStmt(cast<IfStmt>(stmt), indent));
            break;
        case Stmt::SC_WhileStmt:
            result.append(formatWhileStmt(cast<WhileStmt>(stmt), indent));
            break;

        case Stmt::SC_TypeDecl:
            result.append(formatTypeDecl(cast<TypeDecl>(stmt), indent));
            break;
        case Stmt::SC_VarDecl:
            result.append(formatVarDecl(cast<VarDecl>(stmt), indent));
            break;
        case Stmt::SC_FunctionDecl:
            result.append(formatFunctionDecl(cast<FunctionDecl>(stmt), indent));
            break;
        case Stmt::SC_ImplDecl:
            result.append(formatImplDecl(cast<ImplDecl>(stmt), indent));
            break;
        case Stmt::SC_StructDecl:
            result.append(formatStructDecl(cast<StructDecl>(stmt), indent));
            break;

        case Stmt::SC_CallExpr:
            result.append(formatCallExpr(cast<CallExpr>(stmt), indent));
            break;
        
        case Stmt::SC_MoveExpr:
            result.append(formatMoveExpr(cast<MoveExpr>(stmt), indent));
            break;
            
        case Stmt::SC_NamedTypeExpr:
            result.append(formatNamedTypeExpr(cast<NamedTypeExpr>(stmt), indent));
            break;
        case Stmt::SC_PointerTypeExpr:
            result.append(formatPointerTypeExpr(cast<PointerTypeExpr>(stmt), indent));
            break;
        case Stmt::SC_TemplateTypeExpr:
            result.append(formatTemplateTypeExpr(cast<TemplateTypeExpr>(stmt), indent));
            break;
        
        case Stmt::SC_VarRef:
            result.append(formatVarRef(cast<VarRef>(stmt), indent));
            break;
        case Stmt::SC_ImplRef:
            result.append(formatImplRef(cast<ImplRef>(stmt), indent));
            break;
        
        case Stmt::SC_BinaryOperator:
            result.append(formatBinaryOperator(cast<BinaryOperator>(stmt), indent));
            break;
        case Stmt::SC_UnaryOperator:
            result.append(formatUnaryOperator(cast<UnaryOperator>(stmt), indent));
            break;
        
        case Stmt::SC_IntLiteral:
            result.append(formatIntLiteral(cast<IntLiteral>(stmt), indent));
            break;
        case Stmt::SC_FloatLiteral:
            result.append(formatFloatLiteral(cast<FloatLiteral>(stmt), indent));
            break;
        case Stmt::SC_StringLiteral:
            result.append(formatStringLiteral(cast<StringLiteral>(stmt), indent));
            break;

        default:
            result.append("Unknown Statement\n");
    }
    
    if (!result.ends_with("\n"))
        result.append("\n");
    return result;
}

std::string getIndent(size_t indent) {
    std::string result;
    result.reserve(IndentString.size() * indent);
    for (size_t i = 0; i < indent; i++) 
        result.append(IndentString);
    return result;
}

std::string formatBlockStmt(const BlockStmt& stmt, size_t indent) {
    std::string result(getIndent(indent) + "BlockStmt: {\n");
    for (const std::unique_ptr<Stmt>& s : stmt.getStmts())
        result.append(format(*s, indent + 1));
    result.append(getIndent(indent) + "}\n");
    return result;
}

std::string formatCastStmt(const CastStmt& stmt, size_t indent) {
    std::string result(getIndent(indent) + "CastStmt: cast<");
    if (!stmt.isVoidCast()) {
        result.append("\n");
        result.append(format(*stmt.getTypeExpr(), indent + 1));
    } 
    result.append(getIndent(indent) + ">\n");
    result.append(format(*stmt.getValue(), indent + 1));
    return result;
}

std::string formatReturnStmt(const ReturnStmt& stmt, size_t indent) {
    std::string result(getIndent(indent) + "ReturnStmt: return\n");
    if (stmt.hasValue())
        result.append(format(stmt.getValue(), indent + 1));
    return result;
}

std::string formatIfStmt(const IfStmt& stmt, size_t indent, bool isElseIf) {
    std::string result(getIndent(indent) + "IfStmt: ");
    if (isElseIf) {
        if (stmt.isElse()) {
            result.append("else\n");
        } else result.append("elif\n");
    } else result.append("if\n");

    if (!stmt.isElse())
        result.append(format(*stmt.getCondition(), indent + 1));

    if (isa<ast::BlockStmt>(stmt.getBody())) {
        result.append(formatBlockStmt(cast<ast::BlockStmt>(*stmt.getBody()), indent));
    } else result.append(format(*stmt.getBody(), indent + 1));

    if (stmt.hasElse())
        result.append(formatIfStmt(*stmt.getElse(), indent, true));
    result.append("\n");
    return result;
}

std::string formatWhileStmt(const WhileStmt& stmt, size_t indent) {
    std::string result(getIndent(indent) + "WhileStmt: while\n");
    result.append(format(*stmt.getCondition(), indent + 1));
    result.append(format(*stmt.getBody(), indent));
    return result;
}

std::string formatTypeDecl(const TypeDecl& stmt, size_t indent) {
    std::string result(getIndent(indent) + "TypeDecl: type ");
    result.append(stmt.getName().str() + "\n");
    result.append(format(*stmt.getTypeExpr(), indent + 1));
    return result;
}

std::string formatVarDecl(const VarDecl& stmt, size_t indent) {
    std::string result(getIndent(indent) + "VarDecl: ");

    if (stmt.isAttribute() && stmt.isPublic())
        result.append("pub ");
    
    switch (stmt.getVarKind()) {
        case TokenType::Const:
            result.append("const");
            break;
        case TokenType::Let:
            result.append("let");
            break;
        case TokenType::Mut:
            result.append("mut");
            break;
        case TokenType::Move:
            result.append("move");
            break;
    }
    result.append(" " + stmt.getName().str() + "\n");

    if (stmt.getTypeExpr())
        result.append(format(*stmt.getTypeExpr(), indent + 1));
    if (stmt.isDefined())
        result.append(format(*stmt.getValue(), indent + 1));
    return result;
}

std::string formatFunctionDecl(const FunctionDecl& stmt, size_t indent) {
    std::string result(getIndent(indent) + "FunctionDecl: fn ");
    result.append(stmt.getName().str());
    if (stmt.hasArgs()) {
        result.append("(\n");
        for (const std::unique_ptr<VarDecl>& s : stmt.getArgs())
            result.append(formatVarDecl(*s, indent + 1));
        result.append(getIndent(indent) + ")");
    } else result.append("()");

    if (stmt.getTypeExpr()) {
        result.append(" ->\n");
        result.append(format(*stmt.getTypeExpr(), indent + 1));
    } else result.append("\n");
    
    result.append(formatBlockStmt(*stmt.getBody(), indent));
    return result;
}

std::string formatImplDecl(const ImplDecl& stmt, size_t indent, bool isStructImpl) {
    std::string result(getIndent(indent) + "ImplDecl");

    if (!isStructImpl)
        result.append("impl " + stmt.getName().str() + "{\n");
    
    for (const std::unique_ptr<FunctionDecl>& s : stmt.getMethods())
        result.append(formatFunctionDecl(*s, indent + 1));
    result.append(getIndent(indent) + "}");
    return result;
}

std::string formatStructDecl(const StructDecl& stmt, size_t indent) {
    std::string result(getIndent(indent) + "StructDecl: ");
    result.append("struct " + stmt.getName().str() + " ");
    
    result.append(getIndent(indent) + "{\n");
    for (const std::unique_ptr<VarDecl>& s : stmt.getAttrs())
        result.append(formatVarDecl(*s, indent + 1));

    result.append(getIndent(indent) + "}\n");
    if (stmt.hasImpl()) {
        result.append(getIndent(indent));
        if (stmt.hasDerived()) {
            result.append("from\n");
            for (const std::unique_ptr<ImplRef>& s : stmt.getDerived())
                result.append(getIndent(indent + 1) + formatImplRef(*s, indent + 1));
            result.append(getIndent(indent));
        }

        result.append("{\n");
        result.append(formatImplDecl(*stmt.getImpl(), indent + 1));
    }
    return result;
}

std::string formatCallExpr(const CallExpr& stmt, size_t indent) {
    std::string result(getIndent(indent) + "CallExpr: ");
    result.append(stmt.getCalleeName());
    if (stmt.hasParams()) {
        result.append("(\n");
        for (const std::unique_ptr<ExprStmt>& s : stmt.getArgs())
            result.append(format(*s, indent + 1));
        
        for (llvm::StringRef k : stmt.getKWArgs().keys()) {
            result.append(getIndent(indent + 1) + k.str() + "=\n");
            result.append(format(*stmt.getKWArgs().find(k)->second, indent + 2));
        }
    } else result.append("()");
    return result;
}

std::string formatMoveExpr(const MoveExpr& stmt, size_t indent) {
    std::string result(getIndent(indent) + "MoveExpr: move\n");
    result.append(format(stmt.getValue(), indent + 1));
    return result;
}

std::string formatNamedTypeExpr(const NamedTypeExpr& stmt, size_t indent) {
    return getIndent(indent) + "NamedTypeExpr: " + stmt.getName().str() + "\n";
}

std::string formatPointerTypeExpr(const PointerTypeExpr& stmt, size_t indent) {
    return getIndent(indent) + "PointerTypeExpr: \n" + format(stmt.getPointee(), indent + 1) + "\n";
}

std::string formatTemplateTypeExpr(const TemplateTypeExpr& stmt, size_t indent) {
    std::string result(getIndent(indent) + "TemplateTypeExpr: " + stmt.getName().str() + "<\n");
    for (const std::unique_ptr<TypeExpr>& s : stmt.getTypes())
        result.append(format(*s, indent + 1));
    result.append(getIndent(indent) + ">\n");
    return result;
}

std::string formatVarRef(const VarRef& stmt, size_t indent) {
    return getIndent(indent) + "VarRef: " + stmt.getName().str() + "\n";
}

std::string formatImplRef(const ImplRef& stmt, size_t indent) {
    return getIndent(indent) + "ImplRef: " + stmt.getName().str();
}

std::string stringOperator(TokenType Op) {
    switch (Op) {
        case TokenType::Plus:
            return "+";
        case TokenType::Minus:
            return "-";
        case TokenType::Star:
            return "*";
        case TokenType::Slash:
            return "/";

        case TokenType::More:
            return ">";
        case TokenType::Less:
            return "<";

        case TokenType::PlusPlus:
            return "++";
        case TokenType::MinusMinus:
            return "--";

        case TokenType::PlusEqual:
            return "+=";
        case TokenType::MinusEqual:
            return "-=";
        case TokenType::StarEqual:
            return "*=";
        case TokenType::SlashEqual:
            return "/=";

        case TokenType::EqualEqual:
            return "==";
        case TokenType::ExclEqual:
            return "!=";
        case TokenType::MoreEqual:
            return ">=";
        case TokenType::LessEqual:
            return "<=";

        case TokenType::LogAND:
            return "&&";
        case TokenType::LogOR:
            return "||";

        case TokenType::Exclamation:
            return "!";
        case TokenType::Equal:
            return "=";
        case TokenType::Ampersand:
            return "&";
        case TokenType::Pipe:
            return "|";
        
        case TokenType::Arrow:
            return "->";
        
        default:
            return "UNKNOWN OPERATOR: '" + strTokenType(Op).str() + "'";
    }
}

std::string formatBinaryOperator(const BinaryOperator& stmt, size_t indent) {
    std::string result(getIndent(indent) + "BinaryOperator: ");
    result.append("Operator: " + stringOperator(stmt.getOperator()) + "\n");
    result.append(format(*stmt.getLHS(), indent + 1));
    result.append(format(*stmt.getRHS(), indent + 1));
    return result;
}

std::string formatUnaryOperator(const UnaryOperator& stmt, size_t indent) {
    std::string result(getIndent(indent) + "UnaryOperator: ");
    result.append("Operator: " + stringOperator(stmt.getOperator()) + "\n");
    result.append(format(*stmt.getExpr(), indent + 1));
    return result;
}

std::string formatIntLiteral(const IntLiteral& stmt, size_t indent) {
    return getIndent(indent) + "IntLiteral: " + std::to_string(stmt.getValue());
}

std::string formatFloatLiteral(const FloatLiteral& stmt, size_t indent) {
    return getIndent(indent) + "FloatLiteral" + std::to_string(stmt.getValue());
}

std::string formatStringLiteral(const StringLiteral& stmt, size_t indent) {
    return getIndent(indent) + "StringLiteral: \"" + stmt.getValue().str() + "\"";
}

} // namespace fmt

inline std::string FormatStmt(const Stmt& stmt) {
    return fmt::format(stmt, 0);
}

inline std::string FormatStmt(const Stmt* stmt) {
    return FormatStmt(*stmt);
}

inline std::string FormatStmt(const std::unique_ptr<Stmt>& stmt) {
    return FormatStmt(*stmt);
}

} // namespace ast
} // namespace c
