#pragma once

#include <cstdint>

#include "llvm/ADT/StringRef.h"

namespace c {
namespace log {

enum Level : uint8_t {
    Debug,
    Info,
    StyleWarn,  
    Warning,
    Error
};

template <typename... Args> 
std::string format(llvm::StringRef message, Args&&... args);
std::string format_error_code(Level level, std::error_code error_code);
std::string format_file_path(llvm::StringRef path);

llvm::raw_fd_ostream& out();

void log(Level level, llvm::StringRef message);
inline void debug(llvm::StringRef message)  {   log(Level::Debug,       std::forward<llvm::StringRef>(message));    }
inline void info(llvm::StringRef message)   {   log(Level::Info,        std::forward<llvm::StringRef>(message));    }
inline void style(llvm::StringRef message)  {   log(Level::StyleWarn,   std::forward<llvm::StringRef>(message));    }
inline void warn(llvm::StringRef message)   {   log(Level::Warning,     std::forward<llvm::StringRef>(message));    }
inline void error(llvm::StringRef message)  {   log(Level::Error,       std::forward<llvm::StringRef>(message));    }

} // namespace log
} // namespace c
