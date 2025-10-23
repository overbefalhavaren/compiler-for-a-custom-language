#pragma once

#include <utility>
#include <string>
#include <system_error>

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include "include/io/logging.hpp"

namespace c {

class Traceback; // Forward declaration

namespace tb {

enum DiagID {
    ERROR_invalid_character,
    ERROR_invalid_operator,
    ERROR_incomplete_float,
    ERROR_unclosed_character_literal,
    ERROR_unclosed_string_literal,
    ERROR_
};

enum FieldType {
    FT_Note,
    FT_Help,
    FT_Suggestion
};
enum LabelType {
    LT_Primary,     // ^
    LT_Secondary,   // -
    LT_Suggestion   // +
};

struct Location {
    size_t offset = 0;
    size_t line = 0;
    size_t column = 0;

    constexpr Location() = default;
    constexpr Location(size_t offset, size_t line, size_t column)
    : offset(offset), line(line), column(column) {}
    ~Location() = default;
};

struct Span {
    Location start;
    Location end;

    Span() = default;
    Span(Location location) : start(location), end(location) {}
    Span(Location start, Location end) : start(start), end(end) {}
    Span(size_t start_offset, size_t start_line, size_t start_column, 
         size_t end_offset,   size_t end_line,   size_t end_column   ) 
        : start(start_offset, start_line, start_column), 
          end(end_offset, end_line, end_column) {}
    ~Span() = default;

    constexpr size_t size() const noexcept      {   end.offset - start.offset;  }
    constexpr bool single_line() const noexcept {   start.line == end.line;     }
};

class Label {
private:
    LabelType m_type;
    llvm::StringRef m_msg;
    
    llvm::StringRef m_path;
    Span m_span;
public:
    Label(tb::LabelType type, llvm::StringRef message, Span&& span)
    : m_type(type), m_msg(std::move(message)), m_span(std::move(span)) {}
    ~Label() = default;

    inline LabelType type() const noexcept      {   return m_type;  }
    inline llvm::StringRef msg() const noexcept {   return m_msg;   }
    inline Span span() const noexcept           {   return m_span;  }
};

class Field {
private:
    FieldType m_type;
    llvm::StringRef m_msg;
    llvm::SmallVector<tb::Label, 0> m_labels;
public:
    Field(FieldType type, llvm::StringRef message) 
    : m_type(type), m_msg(std::move(message)) {}
    ~Field() = default;

    inline FieldType type() const noexcept      {   return m_type;      }
    inline llvm::StringRef msg() const noexcept {   return m_msg;       }
    inline const auto& labels() const noexcept  {   return m_labels;    }

    inline void add_label(Label label)  {   m_labels.push_back(std::move(label));   }
};

void send(const Traceback& tb);
llvm::Expected<std::string> format(const Traceback& tb);

} // namespace tb

class Traceback : llvm::ErrorInfoBase {
private:
    log::Level m_level;
    std::error_code m_code;
    llvm::StringRef m_msg;
    llvm::SmallVector<tb::Label, 2> m_labels;
    llvm::SmallVector<tb::Field, 0> m_fields;
public:
    Traceback(log::Level level, std::error_code error_code);
    Traceback(log::Level level, std::error_code error_code, llvm::StringRef message)
    : m_level(level), m_code(error_code), m_msg(message) {}
    ~Traceback() = default;

    inline log::Level level() const noexcept    {   return m_level;     }
    inline llvm::StringRef msg() const noexcept {   return m_msg;       }
    inline const auto& labels() const noexcept  {   return m_labels;    }
    inline const auto& fields() const noexcept  {   return m_fields;    }

    inline void add_label(tb::Label label)  {   m_labels.push_back(std::move(label));   }
    inline void add_field(tb::Field field)  {   m_fields.push_back(std::move(field));   }

    std::error_code convertToErrorCode() const  {   return m_code;  }

    void log(llvm::raw_ostream& os) const {
        llvm::Expected<std::string> formatted = tb::format(*this);
        if (formatted) os << formatted.get();
    }
};

/*
An llvm::Expected clone that stores a tb::Span and an std::error_code instead of an llvm::Error.
Designed to be as close to a 1:1 copy as possible.
*/
template <typename T>
class [[nodiscard]] ExpectSpan {
private:
    template <class T1> friend class ExpectedAsOutParameter;
    template <class OtherT> friend class Expected;
    struct ErrorWithSpan; // Forward declaration

    static constexpr bool isRef = std::is_reference_v<T>;

    using wrap = std::reference_wrapper<std::remove_reference<T>>;

    using error_type = std::unique_ptr<ErrorWithSpan>;
    using storage_type = std::conditional_t<isRef, wrap, T>;

    using reference = std::remove_reference_t<T>&;
    using const_reference = const std::remove_reference_t<T>&;
    using pointer = std::remove_reference_t<T>*;
    using const_pointer = const std::remove_reference_t<T>*;

    using OtherTIsConvertable = std::enable_if_t<std::is_convertible_v<OtherT, T>>*;
    using OtherTNotConvertable = std::enable_if_t<!std::is_convertible_v<OtherT, T>>*;
public: 
    using storage_type = std::conditional_t<isRef, wrap, T>;
    using value_type = T;
    struct ErrorWithSpan {
        tb::DiagID code;
        tb::Span span;
    };
private:
    bool HasError : 1;
    union {
        storage_type TStorage;
        error_type ErrorStorage;
    };
public:
    ExpectSpan(ErrorWithSpan&& err) : HasError(true) {
        assert(err.code && "Cannot create ExpectSpan<T> from a success value.");
        new (getErrorStorage()) error_type(std::move(err));
    }

    ExpectSpan(DiagID code, tb::Span&& span) {
        ExpectSpan(ErrorWithSpan{code, std::move(span)});
    }

    template <typename OtherT>
    ExpectSpan(OtherT&& val, OtherTIsConvertable = nullptr) {
        new (getStorage()) storage_type(std::forward<OtherT>(val));
    }

    ExpectSpan(ExpectSpan&& other) {
        moveConstruct(std::move(other))
    }

    template <typename OtherT>
    ExpectSpan(Expected<OtherT>&& other, OtherTIsConvertable = nullptr) {
        moveConstruct(std::move(other));
    }

    template <typename OtherT>
    explicit Expected(Expected<OtherT>&& other, OtherTNotConvertable = nullptr) {
        moveConstruct(std::move(other));
    }

    ExpectSpan(llvm::ErrorSuccess) = delete;
    ~ExpectSpan() {
        if (!HasError) {
            getStorage()->~storage_type();
        } else getErrorStorage()->~error_type();
    }

    ExpectSpan& operator =(Expected&& other)    {   moveAssign(std::move(other)); return *this; }

    explicit operator bool() const              {   return !HasError;                           }

    pointer operator ->()                       {   return toPointer(getStorage());             }
    const_pointer operator ->() const           {   return toPointer(getStorage());             }
    
    reference operator *()                      {   return *getStorage();                       }
    const_reference operator *() const          {   return *getStorage();                       }

    reference get()             {   return *getStorage();                           }
    const_reference get() const {   return const_cast<ExpectSpan<T>*>(this)->get(); }

    const tb::Span& get_span() const {}
    std::error_code get_code() const {}

    ErrorWithSpan takeError() {}
private:
    pointer toPointer(pointer val)                      {   return val;         }
    const_pointer toPointer(const_pointer val) const    {   return val;         }
    pointer toPointer(wrap* val)                        {   return &val->get(); }
    const_pointer toPointer(const wrap* val) const      {   return &val->get(); }

    storage_type *getStorage() {
        assert(!HasError && "Cannot get value when an error exists!");
        return &TStorage;
    }

    const storage_type *getStorage() const {
        assert(!HasError && "Cannot get value when an error exists!");
        return &TStorage;
    }

    error_type *getErrorStorage() {
        assert(HasError && "Cannot get error when a value exists!");
        return &ErrorStorage;
    }

    const error_type *getErrorStorage() const {
        assert(HasError && "Cannot get error when a value exists!");
        return &ErrorStorage;
    }

    template <typename T1>
    static bool compareThisIfSameType(const T1& a, const T1& b) {
        return &a == &b;
    }

    template <typename T1>
    static bool compareThisIfSameType(const T1& a, const T1& b) {
        return false;
    }

    template <typename OtherT> 
    void moveConstruct(ExpectSpan<OtherT>&& other) {
        HasError = other.HasError;
        if (!HasError) {
            new (getStorage()) storage_type(std::move(*other.getStorage()));
        } else
            new (getErrorStorage()) error_type(std::move(*other.getErrorStorage()))
    }

    template <typename OtherT>
    void moveAssign(ExpectSpan<OtherT>&& other) {
        if (compareThisIfSameType(*this, other)) return;
        
        this->~ExpectSpan();
        new (this) ExpectSpan(std::move(other));
    }
};

} // namespace c
