#pragma once

#include <cassert>
#include <cstdint>

namespace c {

/*
 */
class [[nodiscard]] FileID {
    friend class SourceManager;
private:
    uint8_t ID;

    FileID(uint8_t id) : ID(id) {}
public:
    FileID() = default;

    FileID(FileID&&) = default;
    FileID& operator=(FileID&&) = default;

    FileID(const FileID&) = default;
    FileID& operator=(const FileID&) = default;

    inline operator uint8_t() const {
        return ID;
    }

    inline operator bool() const {
        return isValid();
    }

    inline bool operator==(const FileID& other) const {
        return ID == other.ID;
    }

    inline uint8_t id() const {
        return ID;
    }

    inline bool isValid() const {
        return ID != 0;
    }
};

/*
 * Stores an offset to a location in a source file with a 
 * unique uint8 id defined by the StreamManager class.
 */
class SrcLoc {
private:
    // TODO: 
    // Can optimize this by splitting a uint32 into uint24 (offset) and uint8 (file_id). 
    // The max value of uint24 is 16 777 215 which is more than enough for an offset.
    uint32_t Offset;
    FileID ID;
public:
    SrcLoc() = default;
    SrcLoc(FileID id, uint32_t offset) 
        : ID(id), Offset(offset) {}
    ~SrcLoc() = default;

    SrcLoc(SrcLoc&&) = default;
    SrcLoc& operator=(SrcLoc&&) = default;

    SrcLoc(const SrcLoc&) = default;
    SrcLoc& operator=(const SrcLoc&) = default;

    inline bool operator==(const SrcLoc& other) const {
        return getID() == other.getID() && getOffset() == other.getOffset();
    }

    inline uint32_t getOffset() const { 
        return Offset;
    }

    inline FileID getID() const {
        return ID;
    }

    inline bool isValid() const {
        return ID.isValid();
    }
};

/* Stores two offsets in a source file to creata a span. */
class SrcSpan {
    // TODO: Maybe remove friend classes and make the constructor public
    friend class Lexer;
    friend class Token;
    // friend class Source;
private:
    // TODO: 
    // can be optimized in the same way a Location but using a uint56 instead.
    // (uint24) start, (uint24) end, (uint8) id
    uint32_t Start;
    uint32_t End;
    FileID ID;

public:
    SrcSpan(const FileID& id, uint32_t start, uint32_t end) 
        : ID(id), Start(start), End(end) {
        assert(ID.isValid() && "Can't make a span from an invalid FileID.");
        assert(Start <= End && "The start of a span can't be larger than the end.");
    }
// public:
    SrcSpan() = default;
    SrcSpan(const SrcLoc& loc)
        : SrcSpan(loc.getID(), loc.getOffset(), loc.getOffset()) {}
    SrcSpan(const SrcLoc& start, const SrcLoc& end)
        : SrcSpan(start.getID(), start.getOffset(), end.getOffset()) {
        assert(start.getID() == end.getID() && "Can't make a span from locations in different files.");
    }
    ~SrcSpan() = default;

    SrcSpan(SrcSpan&&) = default;
    SrcSpan& operator=(SrcSpan&&) = default;

    SrcSpan(const SrcSpan&) = default;
    SrcSpan& operator=(const SrcSpan&) = default;

    bool operator ==(const SrcSpan& other) const {
        return getID() == other.getID() && 
               getStartOffset() == other.getStartOffset() && 
               getEnd() == other.getEnd();
    }

    inline const FileID& getID() const {
        return ID;
    }

    inline SrcLoc getStart() const {
        return SrcLoc(ID, Start);
    }

    inline SrcLoc getEnd() const {
        return SrcLoc(ID, End);
    }

    inline size_t getLength() const {
        return End - Start;
    }

    inline uint32_t getStartOffset() const {
        return Start;
    }

    inline uint32_t getEndOffset() const {
        return End;
    }

    inline bool isValid() const {
        return ID.isValid();
    }

    bool contains(const SrcLoc& loc) const {
        assert(ID == loc.getID() && "Can't check if a SrcSpan contains an SrcLoc from a different file.");
        return Start <= loc.getOffset() && loc.getOffset() <= End;
    }

    bool contains(const SrcSpan& span) const {
        assert(ID == span.getID() && "Can't compare spans from different files.");
        return Start <= span.getStartOffset() && span.getEndOffset() <= End;
    }
};

} // namespace c
