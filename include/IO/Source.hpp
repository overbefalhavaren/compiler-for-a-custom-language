#pragma once

#include <memory>
#include <type_traits>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryBufferRef.h"

#include "include/IO/SrcSpan.hpp"

namespace c {

class SourceManager;

class Source {
private:
    SourceManager& Manager;

    bool IsMainFile = false;
    FileID ID = FileID();
    std::unique_ptr<llvm::MemoryBuffer> Buffer = nullptr;
public:
    Source(SourceManager& manager, FileID id, std::unique_ptr<llvm::MemoryBuffer>&& buff)
        : Manager(manager), ID(id), Buffer(std::move(buff)) {}

    FileID getFileID() const {
        return ID;
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

    bool isMainFile() const {
        return IsMainFile;
    }

    void setIsMainFile(bool b) {
        IsMainFile = b;
    }

    llvm::StringRef getFilename() const;

    llvm::StringRef getFilePath() const;

    llvm::StringRef getBufferSlice(SrcSpan span) const;

    SrcSpan getBufferSpan(size_t start = 0, std::optional<size_t> end = std::nullopt) const;
};

} // namespace c
