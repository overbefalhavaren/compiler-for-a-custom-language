#pragma once

#ifndef COMPILER_DEBUG_ENABLED

#include <cassert>
#ifdef NDEBUG
#define COMPILER_DEBUG_ENABLED false
#else
#define COMPILER_DEBUG_ENABLED true
#endif // NDEBUG

#endif // COMPILER_DEBUG_ENABLED

#if COMPILER_DEBUG_ENABLED

#include "llvm/Support/raw_ostream.h"

namespace c {

static void _print_debug_message(llvm::StringRef msg, const char* filename, int lineno, bool endln) {
    auto line = std::to_string(lineno);
    auto line_pad = line.size() < 4
        ? std::string(4 - line.size(), ' ')
        : "";

    std::string name = filename;
    auto name_pad = name.size() < 25 
        ? std::string(25 - name.size(), ' ')
        : "";

    llvm::outs() << "DEBUG: " << line << line_pad << " - " << name << name_pad << msg;
    if (endln)
        llvm::outs() << '\n';
}

} // namespace c

#ifndef __FILE_NAME__
#include "llvm/Support/Path.h"
#define __FILE_NAME__ llvm::sys::path::filename(__FILE__).data()
#endif // __FILE_NAME__

#define DEBUG(MSG) c::_print_debug_message(MSG, __FILE_NAME__, __LINE__, true);
#define DBG(MSG) c::_print_debug_message(MSG, __FILE_NAME__, __LINE__, false);

#else

#define DEBUG(MSG)
#define DBG(MSG)

#endif // COMPILER_DEBUG_ENABLED
