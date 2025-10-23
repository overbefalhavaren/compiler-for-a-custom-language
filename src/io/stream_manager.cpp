#include "include/io/stream_manager.hpp"

#include <system_error>

#include "llvm/Support/Path.h"

using namespace c;

// class StreamManager
llvm::Expected<StreamManager::PathString> StreamManager::normalize(llvm::StringRef path) {
    PathString full_path(path);
    std::error_code err = llvm::sys::fs::make_absolute(full_path);
    if (err) return llvm::errorCodeToError(err);

    (void)llvm::sys::path::remove_dots(full_path);
    llvm::sys::path::native(full_path);

    err = llvm::sys::fs::real_path(full_path, full_path);
    if (err) return llvm::errorCodeToError(err);

    return full_path;
}

llvm::Expected<StreamManager::BufferPtr> StreamManager::open(llvm::StringRef path) {
    llvm::Expected<PathString> full_path = normalize(path);
    if (!full_path) return full_path.takeError();

    llvm::ErrorOr<BufferPtr> buffer = llvm::MemoryBuffer::getFile(full_path.get());
    if (!buffer) return llvm::errorCodeToError(buffer.getError());
    return std::move(*buffer);
}

void StreamManager::release(llvm::StringRef full_path) {
    llvm::StringMapIterator item = global_cache.find(full_path);
    if (item != global_cache.end()) {
        if (item->second.references == 1) {
            global_cache.erase(full_path);
        } else dereferenceGlobalBuffer(item->second);
    }
}

void StreamManager::cacheBuffer(BufferPtr&& buffer, llvm::StringRef full_path = "") {
    if (full_path.empty()) {
        auto buffer_path = normalize(buffer.get()->getBufferIdentifier());
        if (!buffer_path) return;
        full_path = buffer_path.get();
    }

    global_cache.try_emplace(full_path, ReferenceCountedBuffer{std::move(buffer), /*references=*/1});
}

llvm::Error StreamManager::cache(llvm::StringRef full_path) {
    if (isCached(full_path)) return llvm::Error::success();
    llvm::Expected<BufferPtr> buffer = open(full_path);
    if (!buffer) return buffer.takeError();

    cacheBuffer(std::move(buffer.get()), full_path);
    return llvm::Error::success();
}

llvm::Expected<llvm::MemoryBufferRef> StreamManager::getOrCache(llvm::StringRef path) {
    llvm::StringMapIterator item = global_cache.find(path);
    if (item != global_cache.end()) 
        return referenceGlobalBuffer(item->second);
    
    llvm::Expected<PathString> full_path = normalize(path);
    if (!full_path) return full_path.takeError();

    item = global_cache.find(full_path.get());
    if (item != global_cache.end()) 
        return referenceGlobalBuffer(item->second);

    llvm::Expected<BufferPtr> buffer = open(full_path.get());
    if (!buffer) return buffer.takeError();

    llvm::MemoryBufferRef ref = *buffer.get();
    cacheBuffer(std::move(buffer.get()), full_path.get());
    return ref;
}

llvm::Expected<llvm::MemoryBufferRef> StreamManager::getBuffer(llvm::StringRef full_path) {
    llvm::StringMapIterator item = global_cache.find(full_path);
    if (item != global_cache.end())
        return referenceGlobalBuffer(item->second);
    return llvm::errorCodeToError(std::make_error_code(std::errc::no_such_file_or_directory));
}

void StreamManager::releaseLocal(llvm::StringRef full_path) {
    llvm::StringMapIterator item = m_cache.find(full_path);
    if (item == m_cache.end()) return;

    if (!itemIsLocal(item->second))
        release(full_path);
    m_cache.erase(full_path);
}

void StreamManager::cacheBufferLocal(BufferPtr&& buffer, llvm::StringRef full_path = "") {
    if (full_path.empty()) {
        auto buffer_path = normalize(buffer.get()->getBufferIdentifier());
        if (!buffer_path) return;
        full_path = buffer_path.get();
    }

    m_cache.try_emplace(full_path, BufferOrReference{std::move(buffer)});
}

llvm::Error StreamManager::cacheLocal(llvm::StringRef full_path) {
    if (isCachedLocal(full_path)) return llvm::Error::success();
    llvm::Expected<BufferPtr> buffer = open(full_path);
    if (!buffer) return buffer.takeError();

    cacheBufferLocal(std::move(buffer.get()), full_path);
    return llvm::Error::success();
}

llvm::Expected<llvm::MemoryBufferRef> StreamManager::getOrCacheLocal(llvm::StringRef path) {
    llvm::Expected<llvm::MemoryBufferRef> item = getBufferLocal(path);
    if (item) return item.get();

    llvm::Expected<PathString> full_path = normalize(path);
    if (!full_path) return full_path.takeError();

    item = getBufferLocal(full_path.get());
    if (item) return item.get();

    llvm::Expected<BufferPtr> buffer = open(full_path.get());
    if (!buffer) return buffer.takeError();

    llvm::MemoryBufferRef ref = *buffer.get();
    cacheBuffer(std::move(buffer.get()), full_path.get());
    return ref;
}

llvm::Expected<llvm::MemoryBufferRef> StreamManager::getBufferLocal(llvm::StringRef full_path) {
    llvm::StringMapIterator item = m_cache.find(full_path);
    if (item != m_cache.end())
        return getRefToLocalBuffer(item->second);

    if (m_global_fallback) {
        llvm::Expected<llvm::MemoryBufferRef> buffer = getBuffer(full_path);
        if (!buffer) return buffer.takeError();

        llvm::MemoryBufferRef ref = buffer.get();
        m_cache.try_emplace(full_path, BufferOrReference{std::move(buffer.get())});
        return buffer;
    }

    return llvm::errorCodeToError(std::make_error_code(std::errc::no_such_file_or_directory));
}
// StremManager
