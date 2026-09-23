// PluginDefinition.cpp
#include "PluginDefinition.h"
#include "JsonUtils.h"
#include <string>

FuncItem funcItem[nbFunc];
NppData nppData;

void pluginInit(HANDLE /*hModule*/) {}
void pluginCleanUp() {}

void commandMenuInit()
{
    // Ctrl+Alt+J / Ctrl+Alt+U / Ctrl+Alt+B / Ctrl+Alt+M as convenient defaults.
    static ShortcutKey skEscape  = { true, true, false, 'J' };
    static ShortcutKey skUnesc   = { true, true, false, 'U' };
    static ShortcutKey skBeautify = { true, true, false, 'B' };
    static ShortcutKey skMinify  = { true, true, false, 'M' };

    setCommand(0, TEXT("Escape JSON String"),   escapeJsonStringCmd,   &skEscape,    false);
    setCommand(1, TEXT("Unescape JSON String"), unescapeJsonStringCmd, &skUnesc,     false);
    setCommand(2, TEXT("Beautify JSON"),        beautifyJsonCmd,       &skBeautify,  false);
    setCommand(3, TEXT("Minify JSON"),          minifyJsonCmd,         &skMinify,    false);
}

void commandMenuCleanUp() {}

bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
{
    if (index >= nbFunc) return false;
    if (!pFunc) return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;
    return true;
}

// ---------------------------------------------------------------------
// Scintilla text helpers.
// Scintilla's own text buffer is plain bytes (UTF-8 when the document is
// UTF-8, which is Notepad++'s default), so std::string is used directly -
// no TCHAR/wide-char conversion needed here.
// ---------------------------------------------------------------------

static HWND getCurrentScintilla()
{
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1) return NULL;
    return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

static bool editorHasSelection(HWND sci)
{
    Sci_Position start = (Sci_Position)::SendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
    Sci_Position end   = (Sci_Position)::SendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
    return end > start;
}

static std::string getSelectedText(HWND sci)
{
    Sci_Position len = (Sci_Position)::SendMessage(sci, SCI_GETSELTEXT, 0, 0); // includes null terminator
    if (len <= 0) return std::string();
    std::string buf(static_cast<size_t>(len), '\0');
    ::SendMessage(sci, SCI_GETSELTEXT, 0, (LPARAM)&buf[0]);
    buf.resize(static_cast<size_t>(len - 1)); // drop the trailing NUL
    return buf;
}

static std::string getWholeText(HWND sci)
{
    Sci_Position len = (Sci_Position)::SendMessage(sci, SCI_GETLENGTH, 0, 0);
    std::string buf(static_cast<size_t>(len) + 1, '\0');
    ::SendMessage(sci, SCI_GETTEXT, len + 1, (LPARAM)&buf[0]);
    buf.resize(static_cast<size_t>(len));
    return buf;
}

// Replaces either the current selection or, if nothing is selected, the
// whole document - grouped as a single undo action either way.
static void replaceEditorText(HWND sci, bool wasSelection, const std::string& newText)
{
    ::SendMessage(sci, SCI_BEGINUNDOACTION, 0, 0);
    if (wasSelection) {
        ::SendMessage(sci, SCI_REPLACESEL, 0, (LPARAM)newText.c_str());
    } else {
        ::SendMessage(sci, SCI_SETTEXT, 0, (LPARAM)newText.c_str());
    }
    ::SendMessage(sci, SCI_ENDUNDOACTION, 0, 0);
}

static void showError(const TCHAR* title, const std::string& message)
{
    ::MessageBoxA(nppData._nppHandle, message.c_str(), NULL, MB_OK | MB_ICONERROR);
    (void)title; // title kept for future use; MessageBoxA above uses default caption
}

// ---------------------------------------------------------------------
// The four plugin commands.
// Each one: read selection (or whole doc if nothing selected) -> run the
// transform from JsonUtils -> write the result back, or show an error
// message box (with the byte offset for JSON parse errors) on failure.
// ---------------------------------------------------------------------

void escapeJsonStringCmd()
{
    HWND sci = getCurrentScintilla();
    if (!sci) return;

    bool sel = editorHasSelection(sci);
    std::string input = sel ? getSelectedText(sci) : getWholeText(sci);

    std::string output = jsonutils::escapeToJsonStringLiteral(input);
    replaceEditorText(sci, sel, output);
}

void unescapeJsonStringCmd()
{
    HWND sci = getCurrentScintilla();
    if (!sci) return;

    bool sel = editorHasSelection(sci);
    std::string input = sel ? getSelectedText(sci) : getWholeText(sci);

    std::string output, error;
    if (!jsonutils::unescapeJsonStringLiteral(input, output, error)) {
        showError(TEXT("Unescape JSON String"), "Could not unescape text:\n" + error);
        return;
    }
    replaceEditorText(sci, sel, output);
}

void beautifyJsonCmd()
{
    HWND sci = getCurrentScintilla();
    if (!sci) return;

    bool sel = editorHasSelection(sci);
    std::string input = sel ? getSelectedText(sci) : getWholeText(sci);

    std::string output;
    jsonutils::ParseError err;
    if (!jsonutils::beautify(input, 2, output, err)) {
        showError(TEXT("Beautify JSON"),
            "Invalid JSON at byte offset " + std::to_string(err.pos) + ":\n" + err.message);
        return;
    }
    replaceEditorText(sci, sel, output);
}

void minifyJsonCmd()
{
    HWND sci = getCurrentScintilla();
    if (!sci) return;

    bool sel = editorHasSelection(sci);
    std::string input = sel ? getSelectedText(sci) : getWholeText(sci);

    std::string output;
    jsonutils::ParseError err;
    if (!jsonutils::minify(input, output, err)) {
        showError(TEXT("Minify JSON"),
            "Invalid JSON at byte offset " + std::to_string(err.pos) + ":\n" + err.message);
        return;
    }
    replaceEditorText(sci, sel, output);
}
