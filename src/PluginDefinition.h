// PluginDefinition.h
// JSON Escape/Unescape/Beautify Notepad++ plugin.
// Based on the official Notepad++ plugin template (npp-plugins/plugintemplate).

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

#include "PluginInterface.h"

// Name shown in the Notepad++ "Plugins" menu.
const TCHAR NPP_PLUGIN_NAME[] = TEXT("Simplify JSON");

// Escape String, Unescape String, Beautify, Minify
const int nbFunc = 4;

void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void commandMenuCleanUp();

bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk = NULL, bool check0nInit = false);

// Plugin commands, wired up to the Plugins menu in commandMenuInit().
void escapeJsonStringCmd();
void unescapeJsonStringCmd();
void beautifyJsonCmd();
void minifyJsonCmd();

#endif // PLUGINDEFINITION_H
