#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/io/source_location.hpp"

namespace c {
namespace diag {

enum DiagLevel {
    DL_Error,
    DL_Warning,
    DL_Info,
    DL_Help
};

enum SpanLevel {
    SL_Error,
    SL_Note,
    SL_Suggestion
};

class TBSpan {
private:
    SpanLevel Lvl;
    SrcSpan Span;
    llvm::StringRef MsgID;
public:
    TBSpan(SpanLevel lvl, SrcSpan span, llvm::StringRef id) 
        : Lvl(lvl), Span(span), MsgID(id) {}
    ~TBSpan() = default;
    
    SpanLevel getLevel() const {
        return Lvl;
    }

    SrcSpan getSpan() const {
        return Span;
    }

    llvm::StringRef getMessage() const {
        assert(false && "Not Implenmented");
        return "";
    }
};

class TBField {
private:
    DiagLevel Lvl;
    llvm::StringRef MsgID;
    llvm::SmallVector<TBSpan> Spans;
public:
    
};

class Traceback {
private:
    DiagLevel Lvl;
    llvm::StringRef MsgID;
    llvm::SmallVector<TBSpan> Spans;
    llvm::SmallVector<TBField> Fields;

};

} // namespace diag
} // namespace diag
