#pragma once

#include <cassert>
#include <cstdint>

namespace c {

/*
 */
class [[nodiscard]] FileID {
private:
    uint8_t ID;
public:
    FileID(FileID&&) = delete;
    FileID(const FileID&) = delete;

    operator uint8_t() const { return ID; }
    operator bool() const { return is_valid(); }

    inline bool is_valid() const { return ID != 0; }
};

/*
 * Stores an offset to a location in a source file with a 
 * unique uint8 id defined by the StreamManager class.
 */
class Location {
    friend class Span;
    friend class Lexer;
private:
    // TODO: 
    // Can optimize this by splitting a uint32 into uint24 (offset) and uint8 (file_id). 
    // The max value of uint24 is 16 777 215 which is more than enough for an offset.
    uint32_t Offset;
    const FileID& ID;
public:
    bool operator ==(const Location& other) {
        return id() == other.id() && offset() == other.offset();
    }

    inline uint32_t offset() const { return Offset; }
    inline uint8_t id() const { return (uint8_t)ID; }
private:
    Location(uint32_t offset, FileID id) : Offset(offset), ID(id) {}

    bool add(uint32_t count) { Offset += count; } 
};

/* Stores two offsets in a source file to creata a span. */
class Span {
    friend class Lexer;
private:
    // TODO: 
    // can be optimized in the same way a Location but using a uint56 instead.
    // (uint24) start, (uint24) end, (uint8) id
    uint32_t Start;
    uint32_t End;
    const FileID& ID;
public:
    bool operator ==(const Span& other) {
        return id() == other.id() && 
               start() == other.start() && 
               end() == other.end();
    }

    inline uint32_t start() const { return Start; }
    inline uint32_t end() const { return End; }
    inline uint8_t id() const { return (uint8_t)ID; }
private:
    explicit Span(const Location& start, const Location& end) 
    : ID(start.ID), Start(start.offset()), End(end.offset()) {
        assert(start.id() == end.id() && "Can't make a span from locations in different files.");
    }

    Span(uint32_t start, uint32_t end, const FileID& id) 
    : ID(id), Start(start), End(end) {
        do_asserts();
    }

    inline void do_asserts() {
        assert(ID.is_valid() && "Can't make a span from an invalid file. Make sure the file is opened");
        assert(Start <= End && "The start of a span can't be larger than the end.");
    }

};

} // namespace c
