#pragma once
#include <windows.h>

namespace Hooks {
    typedef BOOL(__stdcall* twglSwapBuffers)(HDC);
    
    bool Initialize();
    void Shutdown();
}
