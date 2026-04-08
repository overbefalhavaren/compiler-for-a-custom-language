// #include "llvm/ADT/StringRef.h"

// #include "include/ast/parser.hpp"
// #include "include/lexer/lexer.hpp"
// #include "include/io/source_manager.hpp"

// #include "include/ast/format_stmt.hpp"

// using namespace c;

// llvm::StringRef exePath;
// llvm::StringRef srcPath;

// int main(int argc, const char* argv[]) {
//     // srcPath = "C:/Users/LUOST001/Documents/GitHub/gymnasiearbete-2526-LucasISkolan/CompileMe.txt";
//     srcPath = "C:\\Users\\LUOST001\\Documents\\GitHub\\gymnasiearbete-2526-LucasISkolan\\CompileMe.txt";

//     llvm::outs() << "Stage 1\n";

//     FileID id = SourceManager::cacheFromPath(srcPath);

//     llvm::outs() << "Stage 2\n";

//     Lexer lexer(id, SourceManager::getBuffer(id));

//     llvm::outs() << "Stage 3\n";

//     Parser parser(std::move(lexer));

//     llvm::outs() << "Stage 4\n";

//     ast::Module result;
//     bool err = parser.parseSourceFile(&result);
//     if (err) return 1;

//     llvm::outs() << "Stage 5\n";

//     for (const std::unique_ptr<ast::Stmt>& stmt : result.getStmts())
//         llvm::outs() << ast::FormatStmt(stmt) << "\n";

//     llvm::outs() << "Stage 6\n";

//     llvm::outs() << "Congratulations! Shit didn't crash!\n";

//     return 0;
// }
