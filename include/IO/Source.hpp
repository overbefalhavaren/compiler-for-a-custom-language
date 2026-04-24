#pragma once

#include <memory>
#include <type_traits>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/Path.h"

#include "include/IO/SrcSpan.hpp"

namespace c {

class SourceManager;

class Source {
private:
    bool IsMainFile = false;

    FileID ID;
    SourceManager& Manager;
    std::unique_ptr<llvm::MemoryBuffer> Buffer;
public:
    Source(SourceManager& manager, FileID id, std::unique_ptr<llvm::MemoryBuffer>&& buff)
        : Manager(manager), ID(id), Buffer(std::move(buff)) {}

    // Source(Source&&) = delete;
    // Source& operator=(Source&&) = delete;

    // Source(const Source&) = delete;
    // Source& operator=(const Source&) = delete;

    FileID getFileID() const {
        return ID;
    }

    llvm::StringRef getFilename() const {
        return llvm::sys::path::filename(getFilePath());
    }

    llvm::StringRef getFilePath() const {
        return Buffer->getBufferIdentifier();
    }

    SourceManager& getSourceManager() const {
        return Manager;
    }

    llvm::MemoryBufferRef getBuffer() const {
        return *Buffer;
    }

    llvm::StringRef getBufferData() const {
        return Buffer->getBuffer();
    }

    llvm::StringRef getBufferSlice(SrcSpan span) const {
        if (span.getStartOffset() > Buffer->getBufferSize())
            return "";
        
        size_t end = span.getEndOffset();
        if (end > Buffer->getBufferSize())
            end = Buffer->getBufferSize();

        return getBufferData().slice(span.getStartOffset(), end);
    }

    SrcSpan getBufferSpan(size_t start = 0, std::optional<size_t> end = std::nullopt) {
        size_t end_pos = end.has_value() ? end.value() : Buffer->getBufferSize();
        return SrcSpan(ID, start, end_pos);
    }

    bool isMainFile() const {
        return IsMainFile;
    }

    void setIsMainFile(bool b) {
        IsMainFile = b;
    }
};

} // namespace c
