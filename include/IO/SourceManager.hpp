#pragma once

#include <memory>

#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VirtualFileSystem.h"

#include "include/IO/Source.hpp"

namespace c {

class SourceManager {
private:
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS;

    llvm::StringMap<FileID> IDCache = {};
    llvm::SmallVector<Source*> SourceCache = {};
public:
    SourceManager(llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs)
        : FS(fs) {}
    ~SourceManager() = default;

    static bool normalize(llvm::SmallString<0>& path);

    Source* getSourceFromID(FileID id);

    llvm::ErrorOr<Source&> getSource(llvm::StringRef path);
protected:
    Source& cacheBuffer(std::unique_ptr<llvm::MemoryBuffer>&& buffer, llvm::StringRef path);

    static size_t FileIDToIndex(FileID ID) {
        return ID.id() - 1;
    }
};

} // namespace c
