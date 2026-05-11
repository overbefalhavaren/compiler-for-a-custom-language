#include <cerrno>
#include <memory>
#include <type_traits>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Parser.hpp"
#include "include/CodeGen/CodeGenModule.hpp"
#include "include/Frontend/ASTAllocator.hpp"
#include "include/Frontend/ASTVisitor.hpp"
#include "include/IO/Source.hpp"
#include "include/IO/SourceManager.hpp"
#include "include/Lexer/Lexer.hpp"

#include "include/Sema/Sema.hpp"

#include "src/Debug.hpp"

using namespace c;

#define IS_TEST_BUILD

llvm::StringRef EXEPath;

int main(int argc, const char* argv[]) {
    EXEPath = argv[0];
    std::string main_source_file_path;

#ifndef IS_TEST_BUILD
    if (argc == 1) {
        llvm::outs() << "Must input a file path.\n";
        return EINVAL;
    }

    main_source_file_path = argv[1];
    if (main_source_file_path == "--version") {
        llvm::outs() << "Versions are not yet supported. This is an early test build.\n Version: 0.0.0\n";
        return 0;
    }
#else
    main_source_file_path = "C:/Users/LUOST001/Documents/GitHub/gymnasiearbete-2526-LucasISkolan/CompileMe.txt";
#endif

    DEBUG("STEP 1");

    if (!SourceManager::normalize(main_source_file_path)) {
        llvm::outs() << "Invalid File path.\n";
        return ENOENT;
    }

    DEBUG("STEP 2");

    Source* src;
    SourceManager source_manager(llvm::vfs::getRealFileSystem());
    if (auto file_or_err = source_manager.getSource(main_source_file_path)) {
        src = &file_or_err.get();
    } else {
        llvm::outs() << "File not found: " << main_source_file_path << ".\n";
        return ENOENT;
    }

    DEBUG("STEP 3");

    ast::ModuleDecl global_module(src->getBufferSpan(), src->getFilename());

    ASTAllocator ast_allocator;
    ast_allocator.createBuiltinTypes();

    DEBUG("STEP 4");

    Lexer lexer(src->getFileID(), src->getBufferData());
    Parser parser(ast_allocator, lexer);

    DEBUG("STEP 5");

    if (parser.parse(global_module)) {
        llvm::outs() << "Parser Error\n";
        return 1;
    }

    DEBUG("STEP 6");

    FormatFlags fmt(4);
    ASTDumper dumper(fmt, llvm::outs());

    dumper.dump(&global_module);

    Sema sema(ast_allocator);
    if(sema.analyze(&global_module))
        return 1;

    DEBUG("STEP 7");

    dumper.dump(&global_module);

    llvm::LLVMContext ctx;
    auto mod = std::make_unique<llvm::Module>(src->getFilename(), ctx);
    codegen::CodeGenModule cgm(*mod, ctx);

    DEBUG("STEP 8");

    cgm.emitModule(global_module);

    DEBUG("STEP 9");

    llvm::verifyModule(*mod, &llvm::outs());

    std::error_code ec;
    std::string out_file_path = main_source_file_path + ".ll";
    llvm::raw_fd_ostream stream(out_file_path, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Error opening file: " << ec.message() << "\n";
        return 1;
    }

    DEBUG("STEP 10");

    mod->print(stream, nullptr, false, true);

    DEBUG("STEP 11");

    // FIXME: Call LLVM to compile the IR

#ifdef IS_TEST_BUILD
    llvm::outs() << "Congratulations! It didn't crash!\n";
#endif
    return 0;
}
