// json_reader.h: parses JSON text (RFC 8259) into a tree of values, for the GitHub API answers.
// Globals: JsonValue, JsonMember, ParseJson.
// Doc: docs/notes/src/tray/json_reader.md
#pragma once
#include <string>
#include <vector>

struct JsonMember;

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolean = false;
    std::wstring text;                // a string's decoded characters, or a number's literal text
    std::vector<JsonValue> items;     // an array's elements in order
    std::vector<JsonMember> members;  // an object's members in order

    // The first member of an object with this name, or nullptr.
    const JsonValue* Member(const wchar_t* name) const;
    // The text of that member when it is a string, otherwise an empty string.
    std::wstring MemberText(const wchar_t* name) const;
};

struct JsonMember {
    std::wstring name;
    JsonValue value;
};

// Parses UTF-8 text holding one JSON value. On failure `error` has the reason and the character position.
bool ParseJson(const std::string& utf8, JsonValue& out, std::wstring& error);
