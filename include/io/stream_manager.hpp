#pragma once

#include <memory>
#include <variant>

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Error.h"

namespace c {

/*
A steam manager class to manage file buffers and opening/closing files. It has a
reference counted global cache and an instance based class. This class is not 
thread safe.
*/
class StreamManager {
public:
    using PathString = llvm::SmallString<256>;
    using BufferPtr = std::unique_ptr<llvm::MemoryBuffer>;
private:
    using BufferOrReference = std::variant<BufferPtr, llvm::MemoryBufferRef>;
    struct ReferenceCountedBuffer {
        BufferPtr buffer;
        uint16_t references;
    };

    static llvm::StringMap<ReferenceCountedBuffer> global_cache;

    bool m_global_fallback;
    llvm::StringMap<BufferOrReference> m_cache;
public:
    static llvm::Expected<PathString> normalize(llvm::StringRef path);
    static llvm::Expected<BufferPtr> open(llvm::StringRef path);
    
    static inline bool isCached(llvm::StringRef full_path) {
        return global_cache.contains(full_path);
    }
    
    static void release(llvm::StringRef full_path);
    static void cacheBuffer(BufferPtr&& buffer, llvm::StringRef path = "");
    static llvm::Error cache(llvm::StringRef full_path);

    static llvm::Expected<llvm::MemoryBufferRef> getOrCache(llvm::StringRef path);
    static llvm::Expected<llvm::MemoryBufferRef> getBuffer(llvm::StringRef full_path);

    StreamManager(bool global_fallback) : m_global_fallback(global_fallback) {}
    ~StreamManager() {
        if (m_global_fallback)
            for (const auto& cached_item : m_cache) 
                if (itemIsLocal(cached_item.second))
                    release(cached_item.getKey());
    }

    bool isCachedLocal(llvm::StringRef full_path) const {
        if (!m_cache.contains(full_path)) 
            if (m_global_fallback && !global_cache.contains(full_path))
                return false;
        return true;
    }

    void releaseLocal(llvm::StringRef full_path);
    void cacheBufferLocal(BufferPtr&& buffer, llvm::StringRef full_path = "");
    llvm::Error cacheLocal(llvm::StringRef full_path);

    llvm::Expected<llvm::MemoryBufferRef> getOrCacheLocal(llvm::StringRef path);
    llvm::Expected<llvm::MemoryBufferRef> getBufferLocal(llvm::StringRef path);
private:
    static inline llvm::MemoryBufferRef referenceGlobalBuffer(ReferenceCountedBuffer& item) {
        item.references++;
        return *item.buffer;
    }

    static inline void dereferenceGlobalBuffer(ReferenceCountedBuffer& item) {
        assert(item.references > 0 && "Cannot dereference a buffer if no references exists.");
        item.references--;
    }

    static inline bool itemIsLocal(const BufferOrReference& item) {
        return !std::holds_alternative<llvm::MemoryBufferRef>(item);
    }

    static inline llvm::MemoryBufferRef getRefToLocalBuffer(const BufferOrReference& item) {
        if (itemIsLocal(item)) 
            return std::get<llvm::MemoryBufferRef>(item);
        else return *std::get<BufferPtr>(item);
    }
};

} // namespace c
