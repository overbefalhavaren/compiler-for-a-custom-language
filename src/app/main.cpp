#include <cerrno>
#include <memory>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
// #include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
// #include "llvm/Target/TargetMachine.h"

#include "include/AST/Decl.hpp"
#include "include/AST/Parser.hpp"
#include "include/CodeGen/CodeGenFunction.hpp"
#include "include/CodeGen/CodeGenModule.hpp"
#include "include/CodeGen/CodeGenTypes.hpp"
#include "include/Frontend/ASTAllocator.hpp"
#include "include/Frontend/ASTVisitor.hpp"
#include "include/IO/Source.hpp"
#include "include/IO/SourceManager.hpp"
#include "include/Lexer/Lexer.hpp"

#include "include/Sema/Sema.hpp"

#include "src/Debug.hpp"

using namespace c;

#define IS_TEST_BUILD true
#define DEBUG_DUMP_AST true
#define DEBUG_EMIT_FAULTY_IR true

llvm::StringRef EXEPath;

struct Compiler {
    SourceManager SrcManager;
    Source* Src = nullptr;

    llvm::StringRef CompilerPath = "";
    llvm::SmallString<0> MainFilePath = llvm::SmallString<0>();
    llvm::SmallString<0> OutputFilePath = llvm::SmallString<0>();

    ASTAllocator Alloc = ASTAllocator();
    ast::ModuleDecl* TopModule = nullptr;
    ASTDumper* Dumper = nullptr;
    Lexer* Lex = nullptr;
    Parser* Parse = nullptr;
    Sema* SemaAnalasys = nullptr;
    
    llvm::LLVMContext Context = llvm::LLVMContext();
    std::unique_ptr<llvm::Module> Mod = nullptr;

    codegen::CodeGenModule* CGModule = nullptr;
};

bool handleCommandLineArguments(Compiler& instance, llvm::ArrayRef<llvm::StringRef> args) {
    DEBUG("cl 1");
    instance.CompilerPath = args[0];
    DEBUG("cl 2");

#if IS_TEST_BUILD
    instance.MainFilePath = "C:/Users/LUOST001/Documents/GitHub/gymnasiearbete-2526-LucasISkolan/CompileMe.txt";
#else
    if (args.size() == 1) {
        llvm::outs() << "No input file.\n";
        return true;
    }

    if (args[1] == "--version") {
        llvm::outs() << "No versions yet!\n";
        std::exit(0);
    } else {
        instance.MainFilePath = args[0];
        if (SourceManager::normalize(instance.MainFilePath)) {
            llvm::outs() << "File not found: '" << args[0] << "'.\n";
            return true;
        }
    }
#endif // IS_TEST_BUILD
    DEBUG("cl 3");
    if (args.size() <= 2) {
        DEBUG("cl 3.1.1");
        instance.OutputFilePath = instance.MainFilePath;
        DEBUG("cl 3.1.2");
        llvm::sys::path::replace_extension(instance.OutputFilePath, ".ll");
        DEBUG("cl 3.1.3");
    } else {
        // TODO: Add checks to chek if the files exists and can be written to
        DEBUG("cl 3.2.1");
        instance.OutputFilePath = args[1];
    }
    
    DEBUG("cl 4");
    return false;
}

bool initTarget(Compiler& instance) {
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();
    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetMCs();
    // llvm::InitializeAllAsmPrinters();
    // llvm::InitializeAllAsmParsers();

    DEBUG("target 1");
    instance.Mod = std::make_unique<llvm::Module>(
        instance.Src->getFilename(),
        instance.Context
    );
    DEBUG("target 2");
    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    DEBUG("target 3");
    instance.Mod->setTargetTriple(triple);
    DEBUG("target 4");

    // FIXME: My version of llvm is apparently with Visual Studio 2019
    // But I only have Visual Studio 2022, and MicroSLOP want's me
    // to log in to download Visual Studio 2019 (bullshit i know)
    // which I can't so let's just hardcode DataLayout instead.
    // NOTE: This version will probably only work for x86_64 MSVC Win64

    // std::string err;
    // auto target = llvm::TargetRegistry::lookupTarget(triple, err);
    // if (!target) {
    //     DEBUG("target 12.5");
    //     llvm::outs() << err;
    //     return true;
    // }

    // auto TM = target->createTargetMachine(
    //     triple,
    //     "generic",
    //     "",
    //     llvm::TargetOptions(),
    //     std::nullopt
    // );

    // instance.Mod->setDataLayout(TM->createDataLayout());

    // FIXME: Hardcode DataLayout
    instance.Mod->setDataLayout("e-m:w-i64:64-f80:128-n8:16:32:64-S128");
    DEBUG("target 5");
    return false;
}

bool emit(Compiler& instance) {
    DEBUG("emit 1");
    if (instance.Parse->parse(*instance.TopModule)) {
        llvm::outs() << "Parser error\n";
        return true;
    }
    
#if DEBUG_DUMP_AST
    DEBUG("emit 1.5");
    instance.Dumper->dump(instance.TopModule);
#endif // DEBUG_DUMP_AST

    DEBUG("emit 2");
    if (instance.SemaAnalasys->analyze(instance.TopModule)) {
        llvm::outs() << "Semantic analasys error\n";
        return true;
    }

    DEBUG("emit 3");
    instance.CGModule->emitModule(*instance.TopModule);
#if !DEBUG_EMIT_FAULTY_IR
    if (llvm::verifyModule(*instance.Mod, &llvm::outs())) {
        llvm::outs() << "IR verification error\n";
        return true;
    }
#endif // DEBUG_EMIT_FAULTY_IR

    DEBUG("emit 3");
    std::error_code ec;
    llvm::raw_fd_ostream stream(instance.OutputFilePath, ec, llvm::sys::fs::OF_Text);

    DEBUG("emit 4");
    if (ec) {
        llvm::errs() << "Error opening file: " << ec.message() << "\n";
        return true;
    }

    DEBUG("emit 5");
    // instance.Mod->print(stream, nullptr, false, true);
    instance.Mod->print(stream, nullptr);
    DEBUG("emit 6");
    stream.flush();

    DEBUG("emit 7");
    return false;
}

int main(int argc, const char* argv[]) { 
    DEBUG("main 1");
    llvm::SmallVector<llvm::StringRef> args(argc);
    for (int i = 0; i < argc; i++)
        args[i] = llvm::StringRef(argv[i]);

    DEBUG("main 2");
    auto source_manager = SourceManager(llvm::vfs::getRealFileSystem());
    Compiler instance(
        source_manager
    );

    DEBUG("main 3");
    if (handleCommandLineArguments(instance, args)) {
        llvm::outs() << "Error when handling command line arguments.\n";
        return EINVAL;
    }
    DEBUG("main 4");

    auto file_or_err = instance.SrcManager.getSource(instance.MainFilePath);
    DEBUG("main 4.1");
    // if (auto file_or_err = instance.SrcManager.getSource(instance.MainFilePath)) {
    if (file_or_err) {
        DEBUG("main 4.1.1");
        instance.Src = &file_or_err.get();
        DEBUG("main 4.1.2");
    } else {
        llvm::outs() << "File not found: '" << instance.MainFilePath << "'.\n";
        return ENOENT;
    }
    DEBUG("main 5");
    if (initTarget(instance)) {
        llvm::outs() << "Error when initializing target.\n";
        return 1;
    }

    DEBUG("main 6");
    instance.Alloc.createBuiltinTypes();
    DEBUG("main 7");
    instance.TopModule = instance.Alloc.Create<ast::ModuleDecl>(
        instance.Src->getBufferSpan(), 
        instance.Src->getFilename()
    );
    DEBUG("main 8");
    instance.Dumper = new ASTDumper(
        FormatFlags(),
        llvm::outs()
    );
    DEBUG("main 9");
    instance.Lex = new Lexer(
        instance.Src->getFileID(), 
        instance.Src->getBufferData()
    );
    DEBUG("main 10");
    instance.Parse = new Parser(
        instance.Alloc,
        *instance.Lex
    );
    DEBUG("main 11");
    instance.SemaAnalasys = new Sema(
        instance.Alloc
    );
    DEBUG("main 12");
    instance.CGModule = new codegen::CodeGenModule(
        *instance.Mod,
        instance.Context
    );

    DEBUG("main 13");
    if (emit(instance))
        return 1;

#ifdef IS_TEST_BUILD
    llvm::outs() << "Congratulations! It didn't crash!\n";
#endif
    return 0;
}
