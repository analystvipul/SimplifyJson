#include "JsonUtils.h"
#include <cstdio>
#include <cassert>

using namespace jsonutils;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("FAIL: %s\n", msg); ++failures; } else { std::printf("ok:   %s\n", msg); } } while (0)

int main() {
    // Beautify
    {
        std::string out; ParseError err;
        bool ok = beautify("{\"a\":1,\"b\":[1,2,3],\"c\":{\"d\":null,\"e\":true},\"f\":\"hi\\nthere\"}", 2, out, err);
        CHECK(ok, "beautify parses valid JSON");
        std::printf("---beautify output---\n%s\n---\n", out.c_str());
    }

    // Minify
    {
        std::string out; ParseError err;
        bool ok = minify("{\n  \"a\" : 1,\n  \"b\":  [1, 2, 3]\n}\n", out, err);
        CHECK(ok, "minify parses valid JSON");
        CHECK(out == "{\"a\":1,\"b\":[1,2,3]}", "minify produces compact output");
        std::printf("minified: %s\n", out.c_str());
    }

    // Parse error reporting
    {
        std::string out; ParseError err;
        bool ok = beautify("{\"a\":}", 2, out, err);
        CHECK(!ok, "malformed JSON is rejected");
        std::printf("error at pos %zu: %s\n", err.pos, err.message.c_str());
    }

    // Trailing comma should fail (strict JSON)
    {
        std::string out; ParseError err;
        bool ok = minify("[1,2,3,]", out, err);
        CHECK(!ok, "trailing comma in array is rejected");
    }

    // Numbers keep exact formatting (no float precision loss)
    {
        std::string out; ParseError err;
        bool ok = beautify("{\"n\":123456789012345678,\"f\":0.100000000000000001}", 0, out, err);
        CHECK(ok, "large/precise numbers parse");
        CHECK(out.find("123456789012345678") != std::string::npos, "large integer preserved verbatim");
        CHECK(out.find("0.100000000000000001") != std::string::npos, "precise decimal preserved verbatim");
    }

    // Escape / unescape round trip
    {
        std::string raw = "Hello \"World\"\n\tLine2\\path/\x01end";
        std::string literal = escapeToJsonStringLiteral(raw);
        std::printf("escaped literal: %s\n", literal.c_str());
        std::string back, err;
        bool ok = unescapeJsonStringLiteral(literal, back, err);
        CHECK(ok, "unescape succeeds on escaped literal");
        CHECK(back == raw, "escape->unescape round trip is lossless");
    }

    // Unescape works with or without surrounding quotes
    {
        std::string back, err;
        bool ok = unescapeJsonStringLiteral("line1\\nline2", back, err);
        CHECK(ok, "unescape works without surrounding quotes");
        CHECK(back == "line1\nline2", "unescape decodes \\n correctly without quotes");
    }

    // Surrogate pair unescape (emoji)
    {
        std::string back, err;
        bool ok = unescapeJsonStringLiteral("\"\\ud83d\\ude00\"", back, err);
        CHECK(ok, "surrogate pair unescape succeeds");
        CHECK(back == "\xF0\x9F\x98\x80", "surrogate pair decodes to correct UTF-8 (grinning face emoji)");
    }

    // Malformed \u escape is rejected
    {
        std::string back, err;
        bool ok = unescapeJsonStringLiteral("\"\\uZZZZ\"", back, err);
        CHECK(!ok, "bad \\u escape is rejected");
    }

    // Non-ASCII passes through unescaped on escape
    {
        std::string raw = "caf\xC3\xA9"; // "café" in UTF-8
        std::string literal = escapeToJsonStringLiteral(raw);
        CHECK(literal == "\"caf\xC3\xA9\"", "UTF-8 bytes pass through unescaped");
    }

    std::printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
