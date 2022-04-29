// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"
#include <stdint.h>
#include "..\sc_hook.h"
#include <string>
#include "..\strf.h"


HANDLE output_handle = INVALID_HANDLE_VALUE;

template<typename...T>
std::string format(const char* fmt, T&&...args) {
    std::string r;
    tsc::strf::format(r, fmt, std::forward<T>(args)...);
    return r;
}

template<typename...T>
void log(const char* fmt, T&&...args) {
    auto s = format(fmt, std::forward<T>(args)...);
    DWORD written;
    WriteFile(output_handle, s.data(), s.size(), &written, nullptr);
}

void fatal_error(const char* desc) {
    log("fatal error: %s\n", desc);
    TerminateProcess(GetCurrentProcess(), (UINT)-1);
}

// BWAPI < 4.x reads StarCraft's InstallPath from the registry to locate bwapi.ini.
// The fallback path is blank, which it appends a slash to, so we provide a
// better fallback here.
void _SRegLoadString_post(hook_struct* e, hook_function* _f) {
    auto set = [&](const std::string& str) {
        char* dst = (char*)e->arg[3];
        size_t size = (size_t)e->arg[4];
        if (size >= str.size() + 1) memcpy(dst, str.data(), str.size() + 1);
        else {
            if (size) {
                memcpy(dst, str.data(), size - 1);
                dst[size - 1] = 0;
            }
        }
    };
    
    if (!strcmp((const char*)e->arg[1], "InstallPath")) {
        set(std::getenv("BWAISHOTGUN_INSTALLPATH"));
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        //output_handle = CreateFileA("oldbwapi.debug", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        HMODULE storm = GetModuleHandleA("storm.dll");
        if (!storm) {
          //  fatal_error("storm.dll is null");
            return FALSE;
        }
        hook(GetProcAddress(storm, (char*)422), nullptr, _SRegLoadString_post, HOOK_STDCALL, 5);
       // log("Using this '%s' as Starcraft installation folder\n", std::getenv("BWAISHOTGUN_INSTALLPATH"));
        //CloseHandle(output_handle);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

