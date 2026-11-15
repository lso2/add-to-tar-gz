// json_reader.cpp: recursive descent JSON parser over the UTF-16 form of the text.
// Globals: none beyond json_reader.h.
// Doc: docs/notes/src/tray/json_reader.md
#include "json_reader.h"
#include "errors.h"
#include <windows.h>
#include <climits>
#include <cwchar>
#include <utility>

namespace {

// Deep enough for any GitHub answer, and a bound on the parser's recursion.
constexpr int kMaxDepth = 64;

int HexDigit(wchar_t c)
{
    if (c >= L'0' && c <= L'9') {
        return c - L'0';
    }
    if (c >= L'a' && c <= L'f') {
        return c - L'a' + 10;
    }
    if (c >= L'A' && c <= L'F') {
        return c - L'A' + 10;
    }
    return -1;
}

bool IsDigit(wchar_t c)
{
    return c >= L'0' && c <= L'9';
}

class Parser {
public:
    explicit Parser(const std::wstring& text) : text_(text) {}

    bool Parse(JsonValue& out, std::wstring& error)
    {
        // A byte order mark is not part of the JSON value (RFC 8259 section 8.1).
        if (!text_.empty() && text_[0] == 0xFEFF) {
            pos_ = 1;
        }
        SkipSpace();
        bool ok = Value(out, 0);
        if (ok) {
            SkipSpace();
            if (pos_ != text_.size()) {
                ok = Fail(L"text after the end of the value");
            }
        }
        if (!ok) {
            error = error_;
        }
        return ok;
    }

private:
    bool AtEnd() const { return pos_ >= text_.size(); }
    wchar_t Peek() const { return AtEnd() ? L'\0' : text_[pos_]; }

    void SkipSpace()
    {
        while (!AtEnd()) {
            const wchar_t c = text_[pos_];
            if (c != L' ' && c != L'\t' && c != L'\n' && c != L'\r') {
                return;
            }
            ++pos_;
        }
    }

    // The first failure is the one reported. Callers returning false after it leave error_ unchanged.
    bool Fail(const std::wstring& reason)
    {
        if (error_.empty()) {
            error_ = reason + L" at character " + std::to_wstring(pos_ + 1);
        }
        return false;
    }

    // `depth` is the number of objects and arrays around the value.
    bool Value(JsonValue& out, int depth)
    {
        if (AtEnd()) {
            return Fail(L"end of text where a value was expected");
        }
        switch (text_[pos_]) {
        case L'{':
        case L'[':
            if (depth >= kMaxDepth) {
                return Fail(L"nesting deeper than " + std::to_wstring(kMaxDepth) + L" levels");
            }
            return text_[pos_] == L'{' ? Object(out, depth + 1) : Array(out, depth + 1);
        case L'"':
            out.type = JsonValue::Type::String;
            return String(out.text);
        case L't':
            out.type = JsonValue::Type::Bool;
            out.boolean = true;
            return Literal(L"true");
        case L'f':
            out.type = JsonValue::Type::Bool;
            out.boolean = false;
            return Literal(L"false");
        case L'n':
            out.type = JsonValue::Type::Null;
            return Literal(L"null");
        default:
            return Number(out);
        }
    }

    bool Object(JsonValue& out, int depth)
    {
        out.type = JsonValue::Type::Object;
        ++pos_;
        SkipSpace();
        if (Peek() == L'}') {
            ++pos_;
            return true;
        }
        for (;;) {
            SkipSpace();
            if (Peek() != L'"') {
                return Fail(L"expected a member name");
            }
            JsonMember member;
            if (!String(member.name)) {
                return false;
            }
            SkipSpace();
            if (Peek() != L':') {
                return Fail(L"expected ':'");
            }
            ++pos_;
            SkipSpace();
            if (!Value(member.value, depth)) {
                return false;
            }
            out.members.push_back(std::move(member));
            SkipSpace();
            if (Peek() == L',') {
                ++pos_;
                continue;
            }
            if (Peek() == L'}') {
                ++pos_;
                return true;
            }
            return Fail(L"expected ',' or '}'");
        }
    }

    bool Array(JsonValue& out, int depth)
    {
        out.type = JsonValue::Type::Array;
        ++pos_;
        SkipSpace();
        if (Peek() == L']') {
            ++pos_;
            return true;
        }
        for (;;) {
            SkipSpace();
            JsonValue item;
            if (!Value(item, depth)) {
                return false;
            }
            out.items.push_back(std::move(item));
            SkipSpace();
            if (Peek() == L',') {
                ++pos_;
                continue;
            }
            if (Peek() == L']') {
                ++pos_;
                return true;
            }
            return Fail(L"expected ',' or ']'");
        }
    }

    // Each \u escape adds one UTF-16 unit. An escaped surrogate pair adds its two halves in order.
    bool String(std::wstring& out)
    {
        ++pos_;
        out.clear();
        while (!AtEnd()) {
            const wchar_t c = text_[pos_];
            if (c == L'"') {
                ++pos_;
                return true;
            }
            if (c < 0x20) {
                return Fail(L"control character in a string");
            }
            ++pos_;
            if (c != L'\\') {
                out.push_back(c);
                continue;
            }
            if (!Escape(out)) {
                return false;
            }
        }
        return Fail(L"end of text inside a string");
    }

    bool Escape(std::wstring& out)
    {
        if (AtEnd()) {
            return Fail(L"end of text inside a string");
        }
        const wchar_t c = text_[pos_++];
        switch (c) {
        case L'"':
        case L'\\':
        case L'/':
            out.push_back(c);
            return true;
        case L'b':
            out.push_back(L'\b');
            return true;
        case L'f':
            out.push_back(L'\f');
            return true;
        case L'n':
            out.push_back(L'\n');
            return true;
        case L'r':
            out.push_back(L'\r');
            return true;
        case L't':
            out.push_back(L'\t');
            return true;
        case L'u': {
            unsigned unit = 0;
            for (int i = 0; i < 4; ++i) {
                const int digit = HexDigit(Peek());
                if (digit < 0) {
                    return Fail(L"expected 4 hex digits after \\u");
                }
                unit = unit * 16 + static_cast<unsigned>(digit);
                ++pos_;
            }
            out.push_back(static_cast<wchar_t>(unit));
            return true;
        }
        default:
            --pos_;
            return Fail(L"unknown escape in a string");
        }
    }

    size_t Digits()
    {
        const size_t start = pos_;
        while (IsDigit(Peek())) {
            ++pos_;
        }
        return pos_ - start;
    }

    // -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)?, kept as text: no GitHub field read here is a number.
    bool Number(JsonValue& out)
    {
        const size_t start = pos_;
        if (Peek() == L'-') {
            ++pos_;
        }
        if (Peek() == L'0') {
            ++pos_;
        } else if (Digits() == 0) {
            pos_ = start;
            return Fail(L"expected a value");
        }
        if (Peek() == L'.') {
            ++pos_;
            if (Digits() == 0) {
                return Fail(L"expected a digit after '.'");
            }
        }
        if (Peek() == L'e' || Peek() == L'E') {
            ++pos_;
            if (Peek() == L'+' || Peek() == L'-') {
                ++pos_;
            }
            if (Digits() == 0) {
                return Fail(L"expected a digit in the exponent");
            }
        }
        out.type = JsonValue::Type::Number;
        out.text = text_.substr(start, pos_ - start);
        return true;
    }

    bool Literal(const wchar_t* word)
    {
        const size_t length = wcslen(word);
        if (text_.compare(pos_, length, word) != 0) {
            return Fail(L"expected a value");
        }
        pos_ += length;
        return true;
    }

    const std::wstring& text_;
    size_t pos_ = 0;
    std::wstring error_;
};

}  // namespace

const JsonValue* JsonValue::Member(const wchar_t* name) const
{
    if (type != Type::Object) {
        return nullptr;
    }
    for (const JsonMember& member : members) {
        if (member.name == name) {
            return &member.value;
        }
    }
    return nullptr;
}

std::wstring JsonValue::MemberText(const wchar_t* name) const
{
    const JsonValue* value = Member(name);
    return value != nullptr && value->type == Type::String ? value->text : std::wstring();
}

bool ParseJson(const std::string& utf8, JsonValue& out, std::wstring& error)
{
    out = JsonValue();
    std::wstring text;
    if (!utf8.empty()) {
        if (utf8.size() > static_cast<size_t>(INT_MAX)) {
            error = L"text longer than " + std::to_wstring(INT_MAX) + L" bytes";
            return false;
        }
        const int size = static_cast<int>(utf8.size());
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), size, nullptr, 0);
        if (count == 0) {
            error = L"UTF-8 decoding failed: " + WindowsErrorText(GetLastError());
            return false;
        }
        text.resize(static_cast<size_t>(count));
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), size, text.data(), count);
    }
    Parser parser(text);
    return parser.Parse(out, error);
}
