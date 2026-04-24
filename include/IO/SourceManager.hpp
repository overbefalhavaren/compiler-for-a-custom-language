#pragma once

#include <memory>
#include <optional>

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

    llvm::StringMap<FileID> IDCache;
    llvm::SmallVector<Source*> SourceCache;
public:
    SourceManager(llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs)
        : FS(fs) {}
    ~SourceManager() = default;

    static bool normalize(std::string& path) {
        llvm::SmallString<0> full_path(path);
        if (llvm::sys::fs::make_absolute(full_path))
            return true;
        
        if (llvm::sys::path::remove_dots(full_path))
            return true;

        path = full_path.str();
        return false;
    }

    Source* getSourceFromID(FileID id) {
        if (!id.isValid())
            return nullptr;

        assert(SourceCache.size() >= FileIDToIndex(id) && "Invalid FileID for SourceManager.\n");
        return SourceCache[FileIDToIndex(id)];
    }

    llvm::ErrorOr<Source&> getSource(llvm::StringRef path) {
        auto id_lookup = IDCache.find(path);
        if (id_lookup != IDCache.end())
            return *SourceCache[FileIDToIndex(id_lookup->second)];

        auto file_or_err = FS->openFileForRead(path);
        file_or_err.get();
        if (!file_or_err)
            return file_or_err.getError();

        auto buff_or_err = file_or_err.get()->getBuffer(path);
        if (!buff_or_err)
            return buff_or_err.getError();

        return cacheBuffer(std::move(buff_or_err.get()), path);
    }
private:
    Source& cacheBuffer(std::unique_ptr<llvm::MemoryBuffer>&& buffer, llvm::StringRef path) {
        FileID id(SourceCache.size() + 1);
        SourceCache.push_back(new Source(*this, id, std::move(buffer))); // FIXME: I don't like "new"
        IDCache.insert({path, id});
        return *SourceCache.back();
    }

    static size_t FileIDToIndex(FileID ID) {
        return ID.id() - 1;
    }
};

} // namespace c
