#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include "include/io/source_location.hpp"

namespace c {

class SourceManager {
private:
    struct SourceEntry {
        FileID id;
        std::string path;
        std::unique_ptr<llvm::MemoryBuffer> buffer;
    };

    inline static llvm::SmallVector<SourceEntry> cache;
public:
    static std::optional<std::string> normalize(llvm::StringRef path) {
        llvm::SmallString<256> fullPath(path);
        if (llvm::sys::fs::make_absolute(fullPath))
            return std::nullopt;
        
        if (llvm::sys::path::remove_dots(fullPath))
            return std::nullopt;

        return fullPath.str().str();
    }

    static FileID getFileID(llvm::StringRef fullPath) {
        for (SourceEntry& item : cache)
            if (item.path == fullPath)
                return item.id;
        return 0;
    }

    static llvm::StringRef getBuffer(const FileID& id) {
        if (id.isValid())
            for (SourceEntry& item : cache)
                if (item.id == id)
                    return item.buffer->getBuffer();
        return "";
    }

    static llvm::StringRef getSpan(const SrcSpan& span) {
        return getBuffer(span.getID()).slice(span.getStart(), span.getEnd());
    }

    static FileID cacheMemoryBuffer(std::unique_ptr<llvm::MemoryBuffer>&& buffer) {
        std::optional<std::string> path = normalize(buffer->getBufferIdentifier()); // Cannot be nullopt since path has already been used when opening the MemoryBuffer and therefore should be valid
        FileID id = getUniqueFileID();

        cache.push_back(SourceEntry(
            id, std::move(path).value(), std::move(buffer)
        ));
        return id;
    }

    static FileID cacheFromPath(llvm::StringRef fullPath) {
        llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> maybe_buffer = 
            llvm::MemoryBuffer::getFile(fullPath);

        if (!maybe_buffer) {
            llvm::outs() << "Invalid path: " << fullPath << "\n";
            return 0;
        }

        return cacheMemoryBuffer(std::move(maybe_buffer.get()));
    }
private:
    static FileID getUniqueFileID() {
        return FileID(cache.size() + 1);
    }
};

} // namespace c
