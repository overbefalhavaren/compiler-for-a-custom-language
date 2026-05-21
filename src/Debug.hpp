#pragma once

#define COMPILER_DEBUG_ENABLED true

#if COMPILER_DEBUG_ENABLED

#include "llvm/Support/raw_ostream.h"

namespace c {

static void _print_debug_message(llvm::StringRef msg, const char* filename, int lineno, bool endln) {
    auto line = std::to_string(lineno);
    for (size_t i = 0; i < 5 - line.size(); i++)
        line.push_back(' ');

    llvm::outs() << "DEBUG: " << line << " - " << filename << ":\t" << msg;
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
