// NppJsonTools.cpp
// DLL entry point and the exported functions every Notepad++ plugin must
// provide (setInfo, getName, getFuncsArray, beNotified, messageProc,
// isUnicode). Based on the official plugin template's NppPluginDemo.cpp.

#include "PluginDefinition.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
{
    try {
        switch (reasonForCall)
        {
            case DLL_PROCESS_ATTACH:
                pluginInit(hModule);
                break;
            case DLL_PROCESS_DETACH:
                pluginCleanUp();
                break;
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
                break;
        }
    }
    catch (...) { return FALSE; }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;
    commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
{
    switch (notifyCode->nmhdr.code)
    {
        case NPPN_SHUTDOWN:
            commandMenuCleanUp();
            break;
        default:
            return;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif
