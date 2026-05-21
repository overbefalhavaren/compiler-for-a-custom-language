#include "include/IO/Source.hpp"
#include "include/IO/SourceManager.hpp"

#include "llvm/Support/Path.h"

#include "src/Debug.hpp"

namespace c {

llvm::StringRef Source::getFilename() const {
    return llvm::sys::path::filename(getFilePath());
}

llvm::StringRef Source::getFilePath() const {
    return Buffer->getBufferIdentifier();
}

llvm::StringRef Source::getBufferSlice(SrcSpan span) const {
    if (span.getStartOffset() > Buffer->getBufferSize())
        return "";
    
    size_t end = span.getEndOffset();
    if (end > Buffer->getBufferSize())
        end = Buffer->getBufferSize();

    return getBufferData().slice(span.getStartOffset(), end);
}

SrcSpan Source::getBufferSpan(size_t start, std::optional<size_t> end) const {
    size_t end_pos = end.has_value() ? end.value() : Buffer->getBufferSize();
    return SrcSpan(ID, start, end_pos);
}

bool SourceManager::normalize(llvm::SmallString<0>& path) {
    if (llvm::sys::fs::make_absolute(path))
        return true;
    
    if (llvm::sys::path::remove_dots(path))
        return true;

    return false;
}

Source* SourceManager::getSourceFromID(FileID id) {
    if (!id.isValid())
        return nullptr;

    assert(SourceCache.size() >= FileIDToIndex(id) && "Invalid FileID for SourceManager.\n");
    return SourceCache[FileIDToIndex(id)];
}

llvm::ErrorOr<Source&> SourceManager::getSource(llvm::StringRef path) {
    auto id_lookup = IDCache.find(path);
    if (id_lookup != IDCache.end())
        return *SourceCache[FileIDToIndex(id_lookup->second)];

    auto file_or_err = FS->openFileForRead(path);
    if (!file_or_err)
        return file_or_err.getError();

    auto buff_or_err = file_or_err.get()->getBuffer(path);
    if (!buff_or_err)
        return buff_or_err.getError();
        
    return cacheBuffer(std::move(buff_or_err.get()), path);
}

Source& SourceManager::cacheBuffer(std::unique_ptr<llvm::MemoryBuffer>&& buffer, llvm::StringRef path) {
    FileID id(SourceCache.size() + 1);
    SourceCache.push_back(new Source(*this, id, std::move(buffer))); // FIXME: I don't like "new"
    IDCache.insert({path, id});
    return *SourceCache.back();
}

} // namespace c
