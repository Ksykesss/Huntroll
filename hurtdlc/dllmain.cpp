#include <windows.h>
#include "hooks.h"
#include "gui.h"
#include "jni_helper.h"
#include "minecraft_classes.h"
#include "aimassist_jni.h"
#include "triggerbot_jni.h"
#include "esp.h"

HWND g_hWnd = nullptr;
char g_OriginalTitle[256] = {0};
HANDLE g_hThread = nullptr;
bool g_Running = true;

DWORD WINAPI MainThread(LPVOID lpParam) {
    HMODULE hModule = (HMODULE)lpParam;
    
    Sleep(3000);
    
    // Найти окно Minecraft
    g_hWnd = FindWindowA("LWJGL", nullptr);
    if (!g_hWnd) g_hWnd = FindWindowA(nullptr, "Minecraft");
    if (!g_hWnd) g_hWnd = FindWindowA("GLFW30", nullptr);
    
    if (g_hWnd) {
        GetWindowTextA(g_hWnd, g_OriginalTitle, sizeof(g_OriginalTitle));
        
        char newTitle[512];
        wsprintfA(newTitle, "%s | HurtDLC", g_OriginalTitle);
        SetWindowTextA(g_hWnd, newTitle);
    }
    
    // Инициализация JNI
    if (!JNI::Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize JNI!", "HurtDLC Error", MB_OK | MB_ICONERROR);
        FreeLibraryAndExitThread(hModule, 0);
        return 0;
    }
    
    // Инициализация классов Minecraft
    JNIEnv* env = JNI::GetEnv();
    if (!MinecraftClasses::Initialize(env)) {
        MessageBoxA(nullptr, "Failed to initialize Minecraft classes!", "HurtDLC Error", MB_OK | MB_ICONERROR);
        JNI::Shutdown();
        FreeLibraryAndExitThread(hModule, 0);
        return 0;
    }
    
    // Инициализация модулей
    AimAssist::Initialize();
    Triggerbot::Initialize();
    ESP::Initialize();
    
    // Инициализация хуков
    if (!Hooks::Initialize()) {
        MessageBoxA(nullptr, "Failed to initialize hooks!", "HurtDLC Error", MB_OK | MB_ICONERROR);
        MinecraftClasses::Shutdown(env);
        JNI::Shutdown();
        FreeLibraryAndExitThread(hModule, 0);
        return 0;
    }
    
    MessageBoxA(nullptr, "HurtDLC injected!\nINSERT - Toggle GUI\nDELETE - Unload", "HurtDLC", MB_OK | MB_ICONINFORMATION);
    
    // Главный цикл
    while (g_Running) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            GUI::ToggleVisibility();
        }
        
        if (GetAsyncKeyState(VK_DELETE) & 1) {
            g_Running = false;
            break;
        }
        
        // Обновить модули (увеличен интервал для обхода античита)
        AimAssist::Update();
        Triggerbot::Update();
        
        Sleep(16); // ~60 FPS
    }
    
    // Cleanup
    AimAssist::Shutdown();
    Triggerbot::Shutdown();
    ESP::Shutdown();
    
    Sleep(100); // Дать время завершить все операции
    
    Hooks::Shutdown(); // Это отключит GUI рендеринг
    
    JNIEnv* jniEnv = JNI::GetEnv();
    if (jniEnv) {
        MinecraftClasses::Shutdown(jniEnv);
    }
    JNI::Shutdown();
    
    if (g_hWnd && g_OriginalTitle[0] != 0) {
        SetWindowTextA(g_hWnd, g_OriginalTitle);
    }
    
    MessageBoxA(nullptr, "HurtDLC unloaded!", "HurtDLC", MB_OK | MB_ICONINFORMATION);
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        g_hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
