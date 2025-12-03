#include <cerrno>

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "include/lexer/lexer.hpp"
#include "include/ast/parser.hpp"
#include "include/ast/ast.hpp"

using namespace c;

#define PRINT_TOKENS false

llvm::StringRef exePath;
llvm::StringRef srcPath;

int main(int argc, const char* argv[]) {

    // FIXME: Temporarily remove argument handling and hard code input file for testing

    // exePath = argv[0];
    // if (argc == 1) {
    //     llvm::outs() << "Must input a file path.\n";
    //     return EINVAL;
    // }

    // srcPath = argv[1];
    // if (srcPath == "--version") {
    //     llvm::outs() << "Versions are not yet supported. This is an early test build.\n Version: 0.0.0\n";
    //     return 0;
    // }

    // FIXME: Input file hardcoded here
    srcPath = "C:/Users/LUOST001/Documents/GitHub/gymnasiearbete-2526-LucasISkolan/CompileMe.txt";

    std::unique_ptr<llvm::MemoryBuffer> buffer;
    {
        auto maybe_buffer = llvm::MemoryBuffer::getFile(srcPath);
        if (!maybe_buffer) {
            llvm::outs() << "File not found: '" << srcPath << "'\n";
            return 1;
        }

        buffer = std::move(*maybe_buffer);
    }

    Lexer lexer(buffer->getMemBufferRef());

#if PRINT_TOKENS
    Token prev;
    while (!prev.is(TokenType::Eof)) {
        prev = lexer.next();
        llvm::outs() << prev.str() << "\n";
    }
#else
    Parser parser(std::move(lexer));

    ast::context = std::make_unique<llvm::LLVMContext>();
    ast::module_ = std::make_unique<llvm::Module>("TestModule", *ast::context);
    ast::builder = std::make_unique<llvm::IRBuilder<>>(*ast::context);

    llvm::SmallVector<Parser::ExprPtr, 0> exprs(0);
    for (std::optional<Parser::ExprPtr> expr; !parser.eof();) {
        expr = parser.next();
        if (!expr.has_value())
            assert(false && "expr didn't have a value.");

        llvm::outs() << "Expr: " << expr.value()->str() << "\n\n";
        exprs.push_back(std::move(expr.value()));
        if (exprs.size() == exprs.capacity())
            exprs.reserve(10); 
    }

    llvm::outs() << "EXPRESSIONS: " << exprs.size() << "\n";

    if (exprs.size() < exprs.capacity())
        exprs.resize(exprs.size());

    assert(exprs.size() && "No expressions."); 

    // for (const std::unique_ptr<ast::Expr>& e : exprs) {
    //     llvm::outs() << "Expr:\n";
    //     llvm::outs() << e->str() << "\n";
    // }
    
    llvm::SmallString<0> path({srcPath, ".exe"});
    
    std::error_code err;
    llvm::raw_fd_ostream out(path, err, llvm::sys::fs::OF_Text);
    if (err) {
        llvm::outs() << "Could not open file: '" << path << "'.\n";
        return 1;
    }
    
    ast::module_->print(out, nullptr);

    llvm::outs() << "Successfully compiled.\n";
#endif
    
    return 0;
}
