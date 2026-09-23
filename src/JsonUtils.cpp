// JsonUtils.cpp
#include "JsonUtils.h"
#include <cstdio>
#include <cctype>

namespace jsonutils {

// ---------------------------------------------------------------------
// Document tree. Scalars keep their exact raw source text (including the
// quotes, for strings) so round-tripping never loses precision or
// re-escapes anything that was already correctly escaped.
// ---------------------------------------------------------------------
struct Node {
    enum Kind { OBJECT, ARRAY, SCALAR } kind = SCALAR;
    std::string raw;                                   // SCALAR only
    std::vector<std::pair<std::string, Node>> members;  // OBJECT only (key raw text incl. quotes)
    std::vector<Node> items;                             // ARRAY only
};

class Document {
public:
    Node root;
};

namespace {

class Parser {
public:
    explicit Parser(const std::string& s) : text(s), len(s.size()), pos(0) {}

    bool parseDocument(Node& out, ParseError& err) {
        skipWs();
        if (!parseValue(out, err)) return false;
        skipWs();
        if (pos != len) {
            err.pos = pos;
            err.message = "Unexpected trailing characters after JSON value";
            return false;
        }
        return true;
    }

private:
    const std::string& text;
    size_t len;
    size_t pos;

    void skipWs() {
        while (pos < len) {
            char c = text[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++pos; continue; }
            break;
        }
    }

    bool atEnd() const { return pos >= len; }
    char peek() const { return text[pos]; }

    bool fail(ParseError& err, size_t at, const std::string& msg) {
        err.pos = at;
        err.message = msg;
        return false;
    }

    bool parseValue(Node& out, ParseError& err) {
        if (atEnd()) return fail(err, pos, "Unexpected end of input; expected a value");
        char c = peek();
        if (c == '{') return parseObject(out, err);
        if (c == '[') return parseArray(out, err);
        if (c == '"') return parseString(out, err);
        if (c == 't' || c == 'f') return parseBool(out, err);
        if (c == 'n') return parseNull(out, err);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(out, err);
        return fail(err, pos, std::string("Unexpected character '") + c + "'; expected a value");
    }

    bool parseObject(Node& out, ParseError& err) {
        size_t start = pos;
        ++pos; // consume '{'
        out.kind = Node::OBJECT;
        skipWs();
        if (!atEnd() && peek() == '}') { ++pos; return true; }
        while (true) {
            skipWs();
            if (atEnd() || peek() != '"')
                return fail(err, pos, "Expected a string key inside object");
            size_t keyStart = pos;
            std::string dummyStrRaw;
            Node keyNode;
            if (!parseString(keyNode, err)) return false;
            std::string key = text.substr(keyStart, pos - keyStart);
            skipWs();
            if (atEnd() || peek() != ':')
                return fail(err, pos, "Expected ':' after object key");
            ++pos; // consume ':'
            skipWs();
            Node value;
            if (!parseValue(value, err)) return false;
            out.members.emplace_back(key, std::move(value));
            skipWs();
            if (atEnd()) return fail(err, pos, "Unterminated object, expected ',' or '}'");
            if (peek() == ',') { ++pos; continue; }
            if (peek() == '}') { ++pos; break; }
            return fail(err, pos, "Expected ',' or '}' in object");
        }
        (void)start;
        return true;
    }

    bool parseArray(Node& out, ParseError& err) {
        ++pos; // consume '['
        out.kind = Node::ARRAY;
        skipWs();
        if (!atEnd() && peek() == ']') { ++pos; return true; }
        while (true) {
            skipWs();
            Node value;
            if (!parseValue(value, err)) return false;
            out.items.push_back(std::move(value));
            skipWs();
            if (atEnd()) return fail(err, pos, "Unterminated array, expected ',' or ']'");
            if (peek() == ',') { ++pos; continue; }
            if (peek() == ']') { ++pos; break; }
            return fail(err, pos, "Expected ',' or ']' in array");
        }
        return true;
    }

    bool parseString(Node& out, ParseError& err) {
        size_t start = pos;
        ++pos; // consume opening quote
        while (true) {
            if (atEnd()) return fail(err, start, "Unterminated string literal");
            unsigned char c = static_cast<unsigned char>(text[pos]);
            if (c == '"') { ++pos; break; }
            if (c == '\\') {
                ++pos;
                if (atEnd()) return fail(err, start, "Unterminated escape sequence in string");
                char e = text[pos];
                switch (e) {
                    case '"': case '\\': case '/': case 'b':
                    case 'f': case 'n': case 'r': case 't':
                        ++pos;
                        break;
                    case 'u': {
                        ++pos;
                        for (int i = 0; i < 4; ++i) {
                            if (atEnd() || !std::isxdigit(static_cast<unsigned char>(text[pos])))
                                return fail(err, pos, "Invalid \\u escape (need 4 hex digits)");
                            ++pos;
                        }
                        break;
                    }
                    default:
                        return fail(err, pos, std::string("Invalid escape character '\\") + e + "'");
                }
                continue;
            }
            if (c < 0x20) return fail(err, pos, "Unescaped control character in string");
            ++pos;
        }
        out.kind = Node::SCALAR;
        out.raw = text.substr(start, pos - start);
        return true;
    }

    bool parseNumber(Node& out, ParseError& err) {
        size_t start = pos;
        if (peek() == '-') ++pos;
        if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek())))
            return fail(err, pos, "Invalid number: expected digit");
        if (peek() == '0') {
            ++pos;
        } else {
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos;
        }
        if (!atEnd() && peek() == '.') {
            ++pos;
            if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek())))
                return fail(err, pos, "Invalid number: expected digit after '.'");
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos;
        }
        if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
            ++pos;
            if (!atEnd() && (peek() == '+' || peek() == '-')) ++pos;
            if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek())))
                return fail(err, pos, "Invalid number: expected digit in exponent");
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos;
        }
        out.kind = Node::SCALAR;
        out.raw = text.substr(start, pos - start);
        return true;
    }

    bool parseBool(Node& out, ParseError& err) {
        if (text.compare(pos, 4, "true") == 0) { out.kind = Node::SCALAR; out.raw = "true"; pos += 4; return true; }
        if (text.compare(pos, 5, "false") == 0) { out.kind = Node::SCALAR; out.raw = "false"; pos += 5; return true; }
        return fail(err, pos, "Invalid literal; expected 'true' or 'false'");
    }

    bool parseNull(Node& out, ParseError& err) {
        if (text.compare(pos, 4, "null") == 0) { out.kind = Node::SCALAR; out.raw = "null"; pos += 4; return true; }
        return fail(err, pos, "Invalid literal; expected 'null'");
    }
};

void serializeNode(const Node& n, int indentSpaces, int depth, std::string& out) {
    const bool pretty = indentSpaces > 0;
    auto writeIndent = [&](int d) {
        if (pretty) out.append(static_cast<size_t>(indentSpaces) * d, ' ');
    };

    switch (n.kind) {
        case Node::SCALAR:
            out += n.raw;
            break;
        case Node::OBJECT: {
            if (n.members.empty()) { out += "{}"; break; }
            out += '{';
            if (pretty) out += '\n';
            for (size_t i = 0; i < n.members.size(); ++i) {
                writeIndent(depth + 1);
                out += n.members[i].first; // key, already includes quotes
                out += pretty ? ": " : ":";
                serializeNode(n.members[i].second, indentSpaces, depth + 1, out);
                if (i + 1 < n.members.size()) out += ',';
                if (pretty) out += '\n';
            }
            writeIndent(depth);
            out += '}';
            break;
        }
        case Node::ARRAY: {
            if (n.items.empty()) { out += "[]"; break; }
            out += '[';
            if (pretty) out += '\n';
            for (size_t i = 0; i < n.items.size(); ++i) {
                writeIndent(depth + 1);
                serializeNode(n.items[i], indentSpaces, depth + 1, out);
                if (i + 1 < n.items.size()) out += ',';
                if (pretty) out += '\n';
            }
            writeIndent(depth);
            out += ']';
            break;
        }
    }
}

} // anonymous namespace

Document* parseToDocument(const std::string& input, ParseError& error) {
    Document* doc = new Document();
    Parser p(input);
    if (!p.parseDocument(doc->root, error)) {
        delete doc;
        return nullptr;
    }
    return doc;
}

void deleteDocument(Document* doc) {
    delete doc;
}

std::string serialize(const Document* doc, int indentSpaces) {
    std::string out;
    serializeNode(doc->root, indentSpaces, 0, out);
    return out;
}

bool beautify(const std::string& input, int indentSpaces, std::string& output, ParseError& error) {
    Document* doc = parseToDocument(input, error);
    if (!doc) return false;
    output = serialize(doc, indentSpaces);
    deleteDocument(doc);
    return true;
}

bool minify(const std::string& input, std::string& output, ParseError& error) {
    Document* doc = parseToDocument(input, error);
    if (!doc) return false;
    output = serialize(doc, 0);
    deleteDocument(doc);
    return true;
}

// ---------------------------------------------------------------------
// Escape / unescape a JSON string literal.
// Raw UTF-8 bytes (>= 0x80) are passed through unescaped: that's valid
// JSON and keeps the output readable instead of turning every accented
// character into \uXXXX.
// ---------------------------------------------------------------------

std::string escapeToJsonStringLiteral(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 2);
    out += '"';
    for (unsigned char c : input) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
    return out;
}

static bool appendUtf8FromCodepoint(unsigned int cp, std::string& out) {
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        return false;
    }
    return true;
}

static int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

bool unescapeJsonStringLiteral(const std::string& input, std::string& output, std::string& error) {
    std::string s = input;
    // Strip a single pair of matching surrounding double quotes, if present.
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s = s.substr(1, s.size() - 2);
    }

    output.clear();
    output.reserve(s.size());
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (c != '\\') { output += c; ++i; continue; }
        ++i;
        if (i >= n) { error = "Dangling backslash at end of input"; return false; }
        char e = s[i];
        switch (e) {
            case '"':  output += '"';  ++i; break;
            case '\\': output += '\\'; ++i; break;
            case '/':  output += '/';  ++i; break;
            case 'b':  output += '\b'; ++i; break;
            case 'f':  output += '\f'; ++i; break;
            case 'n':  output += '\n'; ++i; break;
            case 'r':  output += '\r'; ++i; break;
            case 't':  output += '\t'; ++i; break;
            case 'u': {
                ++i;
                if (i + 4 > n) { error = "Incomplete \\u escape"; return false; }
                int h0 = hexVal(s[i]), h1 = hexVal(s[i+1]), h2 = hexVal(s[i+2]), h3 = hexVal(s[i+3]);
                if (h0 < 0 || h1 < 0 || h2 < 0 || h3 < 0) { error = "Invalid \\u escape (bad hex digit)"; return false; }
                unsigned int cp = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
                i += 4;
                // Handle UTF-16 surrogate pairs.
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (i + 6 <= n && s[i] == '\\' && s[i+1] == 'u') {
                        int g0 = hexVal(s[i+2]), g1 = hexVal(s[i+3]), g2 = hexVal(s[i+4]), g3 = hexVal(s[i+5]);
                        if (g0 >= 0 && g1 >= 0 && g2 >= 0 && g3 >= 0) {
                            unsigned int low = (g0 << 12) | (g1 << 8) | (g2 << 4) | g3;
                            if (low >= 0xDC00 && low <= 0xDFFF) {
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                                i += 6;
                            }
                        }
                    }
                }
                if (!appendUtf8FromCodepoint(cp, output)) { error = "Invalid Unicode code point"; return false; }
                break;
            }
            default:
                error = std::string("Invalid escape character '\\") + e + "'";
                return false;
        }
    }
    return true;
}

} // namespace jsonutils
