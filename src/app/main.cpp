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

// using namespace c;

llvm::StringRef exePath;
llvm::StringRef srcPath;

int main(int argc, const char* argv[]) {
    llvm::StringRef test = "test";
    llvm::outs() << test << "\n";

    // exePath = argv[0];
    // if (argc == 1) {
    //     llvm::outs() << "Must input a file path.\n";
    //     return 1;
    // }

    // srcPath = argv[1];
    // std::unique_ptr<llvm::MemoryBuffer> buffer;
    // {
    //     auto maybe_buffer = llvm::MemoryBuffer::getFile(srcPath);
    //     if (!maybe_buffer) {
    //         llvm::outs() << "File not found: '" << srcPath << "'\n";
    //         return 1;
    //     }

    //     buffer = std::move(*maybe_buffer);
    // }

    // Lexer lexer(buffer->getMemBufferRef());
    // Parser parser(std::move(lexer));

    // ast::context = std::make_unique<llvm::LLVMContext>();
    // ast::module_ = std::make_unique<llvm::Module>("TestModule", *ast::context);
    // ast::builder = std::make_unique<llvm::IRBuilder<>>(*ast::context);

    // llvm::SmallVector<Parser::ExprPtr, 0> exprs(10);
    // for (std::optional<Parser::ExprPtr> expr = parser.next(); expr.has_value(); parser.next()) {
    //     exprs.push_back(std::move(expr.value()));
    //     if (exprs.size() == exprs.capacity())
    //         exprs.reserve(10);
    // }

    // if (exprs.size() < exprs.capacity())
    //     exprs.resize(exprs.size());

    // llvm::SmallString<0> path({srcPath, ".exe"});
    
    // std::error_code err;
    // llvm::raw_fd_ostream out(path, err, llvm::sys::fs::OF_Text);
    // if (err) {
    //     llvm::outs() << "Could not open file: '" << path << "'.\n";
    //     return 1;
    // }
    
    // ast::module_->print(out, nullptr);
    
    return 0;
}
