#include "hooks.h"
#include "gui.h"
#include "aimassist_jni.h"
#include "triggerbot_jni.h"
#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Hooks {
    twglSwapBuffers oWglSwapBuffers = nullptr;
    WNDPROC oWndProc = nullptr;
    HWND g_hWnd = nullptr;
    bool g_Initialized = false;
    unsigned char originalBytes[15];
    
    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (GUI::IsVisible() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;
        return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
    }
    
    BOOL __stdcall hkWglSwapBuffers(HDC hDc) {
        if (!g_Initialized) {
            g_hWnd = WindowFromDC(hDc);
            if (g_hWnd) {
                GUI::Initialize(g_hWnd);
                oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
                g_Initialized = true;
            }
        }
        
        if (g_Initialized) {
            GUI::Render();
        }
        
        // Проверить что хук еще активен
        if (!oWglSwapBuffers) {
            return TRUE;
        }
        
        // Восстанавливаем оригинальные байты
        DWORD oldProtect;
        if (VirtualProtect((LPVOID)oWglSwapBuffers, 15, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy((void*)oWglSwapBuffers, originalBytes, 15);
            VirtualProtect((LPVOID)oWglSwapBuffers, 15, oldProtect, &oldProtect);
        }
        
        // Вызываем оригинал
        BOOL result = oWglSwapBuffers(hDc);
        
        // Ставим хук обратно только если не выгружаемся
        if (g_Initialized) {
            if (VirtualProtect((LPVOID)oWglSwapBuffers, 15, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                unsigned char* ptr = (unsigned char*)oWglSwapBuffers;
                ptr[0] = 0x48;
                ptr[1] = 0xB8;
                *(void**)(ptr + 2) = (void*)hkWglSwapBuffers;
                ptr[10] = 0xFF;
                ptr[11] = 0xE0;
                VirtualProtect((LPVOID)oWglSwapBuffers, 15, oldProtect, &oldProtect);
            }
        }
        
        return result;
    }
    
    bool Initialize() {
        HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");
        if (!hOpenGL) return false;
        
        oWglSwapBuffers = (twglSwapBuffers)GetProcAddress(hOpenGL, "wglSwapBuffers");
        if (!oWglSwapBuffers) return false;
        
        DWORD oldProtect;
        if (!VirtualProtect((LPVOID)oWglSwapBuffers, 15, PAGE_EXECUTE_READWRITE, &oldProtect))
            return false;
        
        // Сохраняем оригинальные байты
        memcpy(originalBytes, (void*)oWglSwapBuffers, 15);
        
        unsigned char* ptr = (unsigned char*)oWglSwapBuffers;
        ptr[0] = 0x48;
        ptr[1] = 0xB8;
        *(void**)(ptr + 2) = (void*)hkWglSwapBuffers;
        ptr[10] = 0xFF;
        ptr[11] = 0xE0;
        
        VirtualProtect((LPVOID)oWglSwapBuffers, 15, oldProtect, &oldProtect);
        
        return true;
    }
    
    void Shutdown() {
        g_Initialized = false;
        
        Sleep(100); // Дать время завершить текущий SwapBuffers
        
        if (oWglSwapBuffers) {
            DWORD oldProtect;
            VirtualProtect((LPVOID)oWglSwapBuffers, 15, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy((void*)oWglSwapBuffers, originalBytes, 15);
            VirtualProtect((LPVOID)oWglSwapBuffers, 15, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), (LPVOID)oWglSwapBuffers, 15);
        }
        
        if (g_hWnd && oWndProc) {
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
        
        GUI::Shutdown();
    }
}
