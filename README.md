# JSON Tools — Notepad++ Plugin

Adds four commands to the **Plugins** menu:

| Command | Shortcut | What it does |
|---|---|---|
| Escape JSON String | Ctrl+Alt+J | Wraps the text in `"..."` and escapes `"`, `\`, and control characters, so it can be embedded as a JSON string value. |
| Unescape JSON String | Ctrl+Alt+U | Reverses the above: strips surrounding quotes (if present) and decodes `\n`, `\t`, `\uXXXX`, etc. |
| Beautify JSON | Ctrl+Alt+B | Parses the text as JSON and re-prints it with 2-space indentation. |
| Minify JSON | Ctrl+Alt+M | Parses the text as JSON and re-prints it with all insignificant whitespace removed. |

Each command works on the **current selection** if you have text selected, or the **whole document** if you don't. Beautify/Minify validate the JSON first — if it's malformed, you get a message box with the byte offset and reason instead of a mangled result.

The JSON engine (`src/JsonUtils.h/.cpp`) is a real recursive-descent parser, not a regex hack: it correctly handles nested objects/arrays, escaped quotes inside strings, Unicode `\uXXXX` escapes (including surrogate pairs for emoji), and it preserves numbers exactly as written (no float round-off on big integers).

## Project layout

```
src/
  PluginDefinition.h/.cpp   Menu wiring + the 4 commands (reads/writes Scintilla text)
  NppJsonTools.cpp          DLL entry point (setInfo, getName, getFuncsArray, ...)
  JsonUtils.h/.cpp          Parser, pretty-printer/minifier, escape/unescape (no Windows deps)
  test_jsonutils.cpp        Standalone unit tests for JsonUtils (run natively on any OS)
  PluginInterface.h         Notepad++ plugin SDK (official, unmodified)
  Notepad_plus_msgs.h       Notepad++ SDK message constants (official, unmodified)
  Scintilla.h               Scintilla SDK message constants (official, unmodified)
  Sci_Position.h            Scintilla position type (official, unmodified)
CMakeLists.txt
cmake/mingw-w64-x86_64.cmake
```

The four SDK headers are copied unmodified from the official [Notepad++ plugin template](https://github.com/npp-plugins/plugintemplate) (GPLv2) — every Notepad++ plugin needs them.

## Building on Windows (recommended path)

1. Install [Visual Studio](https://visualstudio.microsoft.com/) (Community edition is fine) with the "Desktop development with C++" workload, and CMake (bundled with recent VS, or install separately).
2. Open a "x64 Native Tools Command Prompt for VS" and run, from this folder:
   ```
   cmake -B build -A x64
   cmake --build build --config Release
   ```
3. The plugin DLL is now at `build\Release\JsonTools.dll`.

Alternatively, open the folder directly in Visual Studio ("Open a local folder") — VS's built-in CMake support will configure it automatically; just pick the x64-Release preset and build.

## Building on Linux/macOS (cross-compile)

You can produce the Windows DLL without owning a Windows machine, using mingw-w64:

```bash
# Debian/Ubuntu:
sudo apt install mingw-w64 g++-mingw-w64-x86-64 cmake

cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The DLL ends up at `build/JsonTools.dll`. This has been built and verified in this environment: it's a valid PE32+ Windows DLL exporting the six functions Notepad++ requires (`setInfo`, `getName`, `getFuncsArray`, `beNotified`, `messageProc`, `isUnicode`). It has **not** been loaded inside an actual running Notepad++ (that requires Windows), so do a quick smoke test after installing (see below).

## Installing into Notepad++

1. Close Notepad++.
2. Copy `JsonTools.dll` into:
   ```
   %ProgramFiles%\Notepad++\plugins\JsonTools\JsonTools.dll
   ```
   (create the `JsonTools` subfolder — Notepad++ 7.x+ expects each plugin in its own folder). If you installed Notepad++ per-user, use `%AppData%\Notepad++\plugins\JsonTools\` instead.
3. Start Notepad++. You should see **Plugins → JSON Tools** with the four commands listed above.

Make sure you build the DLL bitness that matches your Notepad++ install — 64-bit Notepad++ needs the x64 DLL (the instructions above build x64 by default); for 32-bit Notepad++, build with `-A Win32` (MSVC) or a mingw32 toolchain instead.

## Running the unit tests (no Windows/Notepad++ needed)

`JsonUtils.h/.cpp` has zero Windows dependencies, so its logic can be tested on any machine:

```bash
g++ -std=c++17 -O2 -o test_jsonutils src/JsonUtils.cpp src/test_jsonutils.cpp
./test_jsonutils
```

This covers: beautify/minify round-tripping, malformed-JSON rejection with correct error offsets, trailing-comma rejection, exact preservation of large/precise numbers, escape/unescape round-tripping, surrogate-pair decoding, and passthrough of raw UTF-8 bytes. All of these passed when built and run in this environment.

## Notes / possible extensions

- Indentation width (2 spaces) is currently fixed in `beautifyJsonCmd()` in `PluginDefinition.cpp` — change the `2` there if you want a different width, or wire it up to a settings dialog.
- `Escape`/`Unescape` treat the *entire* selection (or document) as one string. If you want to escape just the text between existing quotes rather than the whole thing, select just that inner text before running the command.
- Licensed GPLv2, consistent with the Notepad++ plugin template it's based on (see `license.txt`).
