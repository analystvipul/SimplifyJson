// JsonUtils.h
// Small, self-contained JSON engine used by the plugin:
//   - a lenient-but-correct recursive-descent parser that keeps scalar
//     tokens (strings/numbers/true/false/null) as raw text, so numbers
//     never lose precision and strings are never re-escaped unnecessarily
//   - a serializer that can emit either pretty-printed or minified output
//     from the same parsed tree
//   - standalone escape/unescape helpers for turning arbitrary text into
//     a single JSON string literal and back
//
// No Notepad++/Windows dependency here - this file is plain, portable C++
// so it can be unit-tested / compiled standalone.

#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <string>
#include <vector>
#include <utility>

namespace jsonutils {

// A JSON parse error, with the (0-based) byte offset into the input at
// which the problem was found, for a helpful error message to the user.
struct ParseError {
    size_t pos;
    std::string message;
};

// Parses `input` as JSON. On success, returns true and `errorOut` is
// untouched. On failure, returns false and fills `errorOut`.
// On success, the parsed tree is retained internally in `Document` so it
// can be serialized; see parseToDocument/serialize below for the split
// API used by Beautify/Minify.
class Document; // opaque forward declaration, defined in the .cpp

// Parses `input`. Returns a heap-allocated Document (caller must delete)
// on success, or nullptr on failure (with `error` filled in).
Document* parseToDocument(const std::string& input, ParseError& error);

void deleteDocument(Document* doc);

// Serializes a previously parsed Document.
//   indentSpaces == 0  -> compact/minified output (no extra whitespace)
//   indentSpaces  > 0  -> pretty-printed with that many spaces per level
std::string serialize(const Document* doc, int indentSpaces);

// Convenience wrappers combining parse + serialize in one call.
// Return true on success (output filled), false on failure (error filled).
bool beautify(const std::string& input, int indentSpaces, std::string& output, ParseError& error);
bool minify(const std::string& input, std::string& output, ParseError& error);

// Escapes arbitrary UTF-8 text into a JSON string literal, including the
// surrounding double quotes, e.g.  hello"world  ->  "hello\"world"
std::string escapeToJsonStringLiteral(const std::string& input);

// Reverses escapeToJsonStringLiteral. Accepts input with or without the
// surrounding double quotes (if present and matched, they are stripped).
// Returns false with an error message if a \uXXXX escape or backslash
// sequence is malformed; otherwise returns true.
bool unescapeJsonStringLiteral(const std::string& input, std::string& output, std::string& error);

} // namespace jsonutils

#endif // JSONUTILS_H
