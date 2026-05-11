#include "include/Frontend/ASTVisitor.hpp"

#include <string>

#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/Lexer/utils.hpp"

using namespace c;
using namespace c::ast;

std::string strUnaryOperator(const UnaryOperator& op) {
    switch (op.getOpKind()) {
        default:
            llvm_unreachable("");
        
        case UnaryOperator::OpKind::Deref:
            if (op.isRawPtrOp())
                return "*";
            return "&";
        case UnaryOperator::OpKind::Adress:
            if (op.isRawPtrOp())
                return "x*";
            return "x&";
        case UnaryOperator::OpKind::AddOne:
            return "++";
        case UnaryOperator::OpKind::SubOne:
            return "--";
        case UnaryOperator::OpKind::Plus:
            return "+";
        case UnaryOperator::OpKind::Minus:
            return "-";
        case UnaryOperator::OpKind::Not:
            return "!";
    }
}

std::string strBinaryOperator(const BinaryOperator& op) {
    switch (op.getOpKind()) {
        default:
            llvm_unreachable("");
        
        case BinaryOperator::OpKind::Add:
            return "+";
        case BinaryOperator::OpKind::Sub:
            return "-";
        case BinaryOperator::OpKind::Mul:
            return "*";
        case BinaryOperator::OpKind::Div:
            return "/";
        
        case BinaryOperator::OpKind::Assign:
            return "=";
        case BinaryOperator::OpKind::AddAssign:
            return "+=";
        case BinaryOperator::OpKind::SubAssign:
            return "-=";
        case BinaryOperator::OpKind::MulAssign:
            return "*=";
        case BinaryOperator::OpKind::DivAssign:
            return "/=";
        
        case BinaryOperator::OpKind::Equal:
            return "==";
        case BinaryOperator::OpKind::NotEqual:
            return "!=";
        case BinaryOperator::OpKind::MoreThan:
            return ">";
        case BinaryOperator::OpKind::LessThan:
            return "<";
        case BinaryOperator::OpKind::MTEqual:
            return ">=";
        case BinaryOperator::OpKind::LTEqual:
            return "<=";

        case BinaryOperator::OpKind::And:
            return "&&";
        case BinaryOperator::OpKind::Or:
            return "||";
    }
}

namespace c {

void ASTVisitor::print(llvm::StringRef data, bool endLine, std::optional<size_t> indentLevel, char indentChar) {
    if (!indentLevel)
        indentLevel.emplace(IndentLevel);
    std::string indent(indentLevel.value() * Flags.IndentationSpaces, indentChar);
    Stream << indent << data;
    if (endLine)
        Stream << '\n';
}

void ASTDumper::visitDecl(const Decl& DC) {
    Stream << DC.getDeclKindName();
    Stream << " " << &DC << " ";
    Stream << "[" << DC.getStartLoc().getOffset()<< ":" << DC.getEndLoc().getOffset() << "] ";
    
    if (llvm::isa<NamedDecl>(DC) && !llvm::isa<VarDecl>(DC))
        Stream << llvm::cast<NamedDecl>(DC).getName();

    switch (DC.getKind()) {
        default:
            llvm_unreachable("Decl::Kind missin case.");

        case Decl::TypeAliasDeclKind: {
            const TypeAliasDecl& TD = llvm::cast<TypeAliasDecl>(DC);
            indent();
            newline(true);
            visitTypeInfo(*TD.getTypeInfo());
            dedent();
            break;
        }
        case Decl::StructDeclKind: {
            const StructDecl& SD = llvm::cast<StructDecl>(DC);
            indent();
            for (Decl* D : SD.decls()) {
                if (D == SD.decls().back()) {
                    newline(true);
                } else 
                    newline(false);
                visitDecl(*D);
            }
            dedent();
            break;
        }
        case Decl::VarDeclKind: {
            const VarDecl& VD = llvm::cast<VarDecl>(DC);
            Stream << ' ';
            if (VD.isConstant()) {
                Stream << "const";
            } else if (VD.isMutable()) {
                Stream << "mut";
            } else 
                Stream << "let";
            Stream << " " << VD.getName();

            indent();
            if (VD.getInit() != nullptr) {
                newline(false);
                visitExpr(*VD.getInit());
            }

            newline(true);
            visitTypeInfo(*VD.getTypeInfo());

            dedent();
            break;
        }
        case Decl::ParamDeclKind: {
            const ParamDecl& PD = llvm::cast<ParamDecl>(DC);
            indent();
            if (PD.hasInit()) {
                newline(false);
                visitExpr(*PD.getInit());
            }

            newline(true);
            visitTypeInfo(*PD.getTypeInfo());

            dedent();
            break;
        }
        case Decl::FieldDeclKind: {
            const FieldDecl& FD = llvm::cast<FieldDecl>(DC);
            indent();
            if (FD.hasInit()) {
                newline(false);
                visitExpr(*FD.getInit());
            }

            newline(true);
            visitTypeInfo(*FD.getTypeInfo());

            dedent();
            break;
        }
        case Decl::FunctionDeclKind: {
            const FunctionDecl& FD = llvm::cast<FunctionDecl>(DC);
            indent();
            if (!FD.isVoid()) {
                newline(false);
                visitTypeInfo(*FD.getTypeInfo());
            }
            if (FD.hasParams())
                for (auto D : FD.parameters()) {
                    newline(false);
                    visitDecl(*D);
                }
            if (FD.hasBody()) {
                newline(true);
                visitStmt(*FD.getBody());
            }
            dedent();
            break;
        }
        case Decl::ModuleDeclKind: {
            const ModuleDecl& MD = llvm::cast<ModuleDecl>(DC);
            indent();
            for (Decl* D : MD.decls()) {
                if (D == MD.decls().back()) {
                    newline(true);
                } else 
                    newline(false);
                visitDecl(*D);
            }
            dedent();
            break;
        }
    }
}

void ASTDumper::visitStmt(const Stmt& ST) {
    if (llvm::isa<Expr>(ST)) {
        visitExpr(llvm::cast<Expr>(ST));
        return;
    }

    Stream << ST.getStmtKindName() << " " << &ST << " ";
    Stream << "[" << ST.getStartLoc().getOffset() << ":" << ST.getEndLoc().getOffset() << "] ";

    switch (ST.getKind()) {
        default:
            llvm_unreachable("Stmt::Kind missing condition");
        
        case Stmt::IfStmtKind: {
            auto IS = llvm::cast<IfStmt>(ST);
            Stream << "if";

            const IfStmt* cur = &IS;
            while (true) {
                indent();
                newline(false);
                visitExpr(*cur->getCondition());
                
                if (cur->hasElse()) {
                    newline(false);
                } else
                    newline(true);
                visitStmt(*cur->getBody());

                if (cur->hasElse()) {
                    if (auto next = llvm::dyn_cast<IfStmt>(cur->getElse())) {
                        newline(false);
                        cur = next;
                        Stream << "IfStmt " << cur << " ";
                        Stream << "[" << cur->getStartLoc().getOffset() << ":" 
                               << cur->getEndLoc().getOffset() << "] ";
                        Stream << "elif";
                        continue;
                    } else {
                        newline(true);
                        Stream << "IfStmt else";
                        indent();
                        newline(true);
                        visitStmt(*cur->getElse());
                        dedent();
                        dedent();
                        break;
                    }
                } else {
                    dedent();
                    break;
                }
            }
            break;
        }
        case Stmt::WhileStmtKind: {
            auto WS = llvm::cast<WhileStmt>(ST);
            indent();
            newline(false);
            visitExpr(*WS.getCondition());
            newline(true);
            visitStmt(*WS.getBody());
            dedent();
            break;
        }
        case Stmt::BlockStmtKind: {
            auto BS = llvm::cast<BlockStmt>(ST);
            indent();
            for (Stmt* S : BS.stmts()) {
                if (S == BS.stmts().back()) {
                    newline(true);
                } else
                    newline(false);
                visitStmt(*S);
            }
            dedent();
            break;
        }
        case Stmt::ReturnStmtKind: {
            auto RS = llvm::cast<ReturnStmt>(ST);
            if (RS.isVoid()) {
                indent();
                newline(true);
                visitExpr(*RS.getValue());
                dedent();
            }
            break;
        }
        case Stmt::DeclStmtKind: {
            auto DS = llvm::cast<DeclStmt>(ST);
            indent();
            newline(true);
            visitDecl(*DS.getDecl());
            dedent();
            break;
        }
    }
}

void ASTDumper::visitExpr(const ast::Expr& EX) {
    Stream << EX.getStmtKindName() << " " << &EX << " ";
    Stream << "[" << EX.getStartLoc().getOffset()<< ":" << EX.getEndLoc().getOffset() << "] ";

    switch (EX.getKind()) {
        default:
            llvm_unreachable("Stmt::Kind missing case");

        case Stmt::AccessExprKind: {
            auto AE = llvm::cast<AccessExpr>(EX);
            Stream << AE.getFieldName();
            indent();
            if (AE.isResolved()) {
                newline(false);
                visitType(AE.getType());
                newline(false);
                visitExpr(*AE.getBase());
                newline(true);
                Stream << AE.getFieldDecl()->getKind() << " " 
                       << AE.getFieldDecl() << " " << AE.getFieldDecl()->getName();
                indent();
                visitType(AE.getFieldDecl()->getType());
                dedent();
            } else {
                newline(true);
                visitExpr(*AE.getBase());
            }
            dedent();
            break;
        }
        case Stmt::SliceExprKind: {
            auto SE = llvm::cast<SliceExpr>(EX);
            indent();
            if (SE.isResolved()) {
                newline(false);
                visitType(SE.getType());
            }
            newline(false);
            visitExpr(*SE.getBase());
            newline(true);
            visitExpr(*SE.getStart());
            dedent();
            break;
        }
        case Stmt::BinaryOperatorKind: {
            auto BO = llvm::cast<BinaryOperator>(EX);
            Stream << strBinaryOperator(BO);
            indent();
            if (BO.isResolved()) {
                newline(false);
                visitType(BO.getType());
            }
            newline(false);
            visitExpr(*BO.getLHS());
            newline(true);
            visitExpr(*BO.getRHS());
            dedent();
            break;
        }
        case Stmt::UnaryOperatorKind: {
            auto UO = llvm::cast<UnaryOperator>(EX);
            Stream << strUnaryOperator(UO);
            indent();
            if (UO.isResolved()) {
                newline(false);
                visitType(UO.getType());
            }
            newline(true);
            visitExpr(*UO.getSubExpr());
            dedent();
            break;
        }
        case Stmt::CallExprKind: {
            auto CE = llvm::cast<CallExpr>(EX);
            Stream << CE.getCallee();
            indent();
            if (CE.isResolved()) {
                newline(false);
                visitType(CE.getType());
                if (CE.getAmntArgs() == 0) {
                    newline(true);
                } else
                    newline(false);
                // Stream << CE.getCalleeDecl()->getDeclKindName() << " " 
                //        << CE.getCalleeDecl() << " " << CE.getCalleeDecl()->getName();
                visitDecl(*CE.getCalleeDecl());
            }

            for (const Expr* E : CE.arguments()) {
                if (E == CE.arguments().back()) {
                    newline(true);
                } else
                    newline(false);
                visitExpr(*E);
            }
            dedent();
            break;
        }
        case Stmt::DeclRefExprKind: {
            auto DE = llvm::cast<DeclRefExpr>(EX);
            Stream << DE.getName();
            if (DE.isResolved()) {
                indent();
                newline(false);
                visitType(DE.getType());
                newline(true);
                visitDecl(*DE.getDecl());
                // Stream << DE.getDecl()->getDeclKindName() << " " 
                //        << DE.getDecl() << " " << DE.getDecl()->getName();
                dedent();
            }
            break;
        }
        case Stmt::ArrayLiteralKind: {
            auto AL = llvm::cast<ArrayLiteral>(EX);
            indent();
            if (AL.isResolved()) {
                newline(false);
                visitType(AL.getType());
            }

            for (const Expr* E : AL.items()) {
                if (E == AL.items().back()) {
                    newline(true);
                } else
                    newline(false);
                visitExpr(*E);
            }
            dedent();
            break;
        }
        case Stmt::IntegerLiteralKind: {
            auto IL = llvm::cast<IntegerLiteral>(EX);
            Stream << IL.getValue();
            if (IL.isResolved()) {
                indent();
                newline(true);
                visitType(IL.getType());
                dedent();
            }
            break;
        }
        case Stmt::FloatingLiteralKind: {
            auto FL = llvm::cast<FloatingLiteral>(EX);
            Stream << FL.getValue();
            if (FL.isResolved()) {
                indent();
                newline(true);
                visitType(FL.getType());
                dedent();
            }
            break;
        }
        case Stmt::BooleanLiteralKind: {
            auto BL = llvm::cast<BooleanLiteral>(EX);
            if (BL.getValue())
                Stream << "true";
            else
                Stream << "false";
            if (BL.isResolved()) {
                indent();
                newline(true);
                visitType(BL.getType());
                dedent();
            }
            break;
        }
    }
}

void ASTDumper::visitType(const Type* Ty, bool unfoldPointer) {
    Stream << "Type " << Ty << " ";
    if (Ty->isStructTy()) {
        Stream << llvm::cast<StructType>(Ty)->getDecl()->getName();
    } else if (Ty->isBuiltinTy()) {
        auto BT = llvm::cast<BuiltinType>(Ty);
        for (auto& entry : lexer::getBuiltinMap()) 
            if (entry.getValue() == BT->getBuiltinKind()) {
                Stream << " " << entry.getKey();
                break;
            }
    } else if (Ty->isPointerTy()) {
        auto PT = llvm::cast<PointerType>(Ty);
        if (PT->isRaw()) {
            Stream << "*";
        } else
            Stream << "&";
        
        if (unfoldPointer) {
            indent();
            newline(true);
            visitType(Ty);
            dedent();
        } else
            Stream << PT->getPointee();
    } else if (Ty->isArrayTy()) {
        auto AT = llvm::cast<ArrayType>(Ty);
        Stream << "[" << AT->getElemType();
        if (AT->getInitSize() != 0)
            Stream << ": " << AT->getInitSize();
        Stream << "]";
    } else
        llvm_unreachable("Type::Kind missing condition");
}

void ASTDumper::visitTypeInfo(const ast::TypeInfo& Ty) {
    Stream << "TypeInfo ";
    if (!Ty.isImplicitTy()) {
        Stream << "[" << Ty.getStartLoc().getOffset()<< ":" << Ty.getEndLoc().getOffset() << "] ";

        if (Ty.isNamedTy()) {
            Stream << Ty.getName();
        } else if (Ty.isRawPointerTy()) {
            Stream << "*";
        } else if (Ty.isPointerTy()) {
            Stream << "&";
        } else if (Ty.isArrayTy()) {
            Stream << "[";
            if (Ty.getArraySize() > 0)
                Stream << Ty.getArraySize();
            Stream << "]";
        } else 
            llvm_unreachable("TypeInfo::Kind missing condition.");
    }
    
    indent();
    if (Ty.isResolved())
        visitType(Ty.getType(), false);

    if (Ty.isPointerTy()) {
        newline(true);
        visitTypeInfo(*Ty.getPointee());
    }
    dedent();
}

} // namespace c
