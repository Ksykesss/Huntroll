#include "gui.h"
#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_win32.h"
#include "ext/imgui/imgui_impl_opengl3.h"
#include <filesystem>
#include <fstream>
#include "ext/json.hpp"
#include "esp.h"

namespace GUI {
    static bool g_Visible = true;
    static bool g_Initialized = false;
    static bool g_ShuttingDown = false;
    static int g_CurrentTab = 0;
    static Statistics g_Statistics;
    
    struct Settings {
        bool aimAssist = false;
        float aimSpeed = 2.0f;
        float aimDistance = 4.0f;
        float aimAngle = 70.0f;
        bool headshotPriority = true;
        float headshotOffset = 0.95f;
        bool showFOV = false;
        
        bool triggerbot = false;
        int triggerbotDelay = 70;
        float triggerbotDistance = 4.0f;
        bool triggerbotHeadshotOnly = false;
        float triggerbotHeadshotThreshold = 0.80f;
        
        bool esp = false;
        bool espBox = true;
        bool espHealth = true;
        bool espDistance = true;
        bool espName = true;
        float espDistance_max = 50.0f;
        
        float accentColor[3] = {0.0f, 1.0f, 0.84f};
        bool showFPS = true;
        bool showStats = true;
        bool safeMode = false;
    } settings;

    void Initialize(HWND hwnd) {
        if (g_Initialized) return;
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        
        // Использовать дефолтный шрифт ImGui (работает стабильно)
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = 1.0f;
        
        // Стиль
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 10.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.WindowPadding = ImVec2(15, 15);
        style.FramePadding = ImVec2(10, 6);
        style.ItemSpacing = ImVec2(12, 8);
        
        ImVec4* colors = style.Colors;
        ImVec4 accent = ImVec4(settings.accentColor[0], settings.accentColor[1], settings.accentColor[2], 1.0f);
        
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.12f, 0.14f, 0.95f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.14f, 0.16f, 0.95f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.10f, 0.12f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.12f, 0.14f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.17f, 0.19f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.27f, 0.29f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.17f, 0.19f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.20f, 0.22f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.24f, 1.0f);
        colors[ImGuiCol_CheckMark] = accent;
        colors[ImGuiCol_SliderGrab] = accent;
        colors[ImGuiCol_SliderGrabActive] = ImVec4(accent.x * 0.7f, accent.y * 0.7f, accent.z * 0.7f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(accent.x * 0.3f, accent.y * 0.3f, accent.z * 0.3f, 0.8f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x * 0.5f, accent.y * 0.5f, accent.z * 0.5f, 0.8f);
        colors[ImGuiCol_HeaderActive] = accent;
        
        // Инициализация ImGui backends (только один раз!)
        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplOpenGL3_Init();
        
        g_Initialized = true;
        
        // Загрузить конфигурацию
        LoadConfig();
    }

    void Render() {
        if (!g_Initialized || g_ShuttingDown) return;
        
        // Обновить FPS
        DWORD currentTime = GetTickCount();
        if (g_Statistics.lastFrameTime > 0) {
            g_Statistics.frameTime = (float)(currentTime - g_Statistics.lastFrameTime);
            if (g_Statistics.frameTime > 0) {
                g_Statistics.fps = 1000.0f / g_Statistics.frameTime;
            }
        }
        g_Statistics.lastFrameTime = currentTime;
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        bool settingsChanged = false;
        
        if (g_Visible) {
            ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(750, 550), ImGuiCond_FirstUseEver);
            
            // Принудительно показать окно
            ImGui::Begin("HurtDLC", nullptr, ImGuiWindowFlags_NoCollapse);
            
            // Левая панель
            ImGui::BeginChild("Tabs", ImVec2(150, 0), true);
            
            if (ImGui::Selectable("Combat", g_CurrentTab == 0, 0, ImVec2(0, 40))) g_CurrentTab = 0;
            if (ImGui::Selectable("Visual", g_CurrentTab == 1, 0, ImVec2(0, 40))) g_CurrentTab = 1;
            if (ImGui::Selectable("Misc", g_CurrentTab == 2, 0, ImVec2(0, 40))) g_CurrentTab = 2;
            
            ImGui::EndChild();
            
            ImGui::SameLine();
            
            // Правая панель
            ImGui::BeginChild("Modules", ImVec2(0, 0), true);
            
            if (g_CurrentTab == 0) {
                ImGui::Text("COMBAT");
                ImGui::Separator();
                ImGui::Spacing();
                
                if (ImGui::CollapsingHeader("AimAssist", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Checkbox("Enabled##aim", &settings.aimAssist)) settingsChanged = true;
                    if (settings.aimAssist) {
                        if (ImGui::SliderFloat("Speed", &settings.aimSpeed, 0.1f, 5.0f)) settingsChanged = true;
                        if (ImGui::SliderFloat("Distance", &settings.aimDistance, 1.0f, 10.0f)) settingsChanged = true;
                        if (ImGui::SliderFloat("FOV", &settings.aimAngle, 10.0f, 180.0f)) settingsChanged = true;
                        if (ImGui::Checkbox("Show FOV Circle", &settings.showFOV)) settingsChanged = true;
                        
                        ImGui::Spacing();
                        if (ImGui::Checkbox("Headshot Priority", &settings.headshotPriority)) settingsChanged = true;
                        if (settings.headshotPriority) {
                            if (ImGui::SliderFloat("Headshot Offset", &settings.headshotOffset, 0.7f, 1.0f, "%.0f%%")) {
                                settingsChanged = true;
                            }
                            ImGui::Text("Offset: %.0f%%", settings.headshotOffset * 100.0f);
                        }
                    }
                }
                
                ImGui::Spacing();
                
                if (ImGui::CollapsingHeader("Triggerbot", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Checkbox("Enabled##trigger", &settings.triggerbot)) settingsChanged = true;
                    if (settings.triggerbot) {
                        if (ImGui::SliderInt("Delay (ms)", &settings.triggerbotDelay, 0, 200)) settingsChanged = true;
                        if (ImGui::SliderFloat("Distance##trigger", &settings.triggerbotDistance, 1.0f, 6.0f)) settingsChanged = true;
                    }
                }
            }
            else if (g_CurrentTab == 1) {
                ImGui::Text("VISUAL");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("Accent Color");
                if (ImGui::ColorEdit3("##accent", settings.accentColor, ImGuiColorEditFlags_NoInputs)) settingsChanged = true;
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // ESP Settings
                if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Checkbox("Enabled##esp", &settings.esp)) settingsChanged = true;
                    if (settings.esp) {
                        if (ImGui::Checkbox("Box", &settings.espBox)) settingsChanged = true;
                        if (ImGui::Checkbox("Health", &settings.espHealth)) settingsChanged = true;
                        if (ImGui::Checkbox("Distance", &settings.espDistance)) settingsChanged = true;
                        if (ImGui::Checkbox("Name", &settings.espName)) settingsChanged = true;
                        if (ImGui::SliderFloat("Max Distance", &settings.espDistance_max, 10.0f, 100.0f, "%.0fm")) settingsChanged = true;
                    }
                }
            }
            else if (g_CurrentTab == 2) {
                ImGui::Text("MISC");
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("HurtDLC v1.0");
                ImGui::Text("Fabric 1.21.4");
                ImGui::Spacing();
                
                // Safe Mode
                if (ImGui::Checkbox("Safe Mode", &settings.safeMode)) settingsChanged = true;
                if (settings.safeMode) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "WARNING: All aim modifications disabled!");
                }
                ImGui::Spacing();
                
                ImGui::Separator();
                ImGui::Text("Module Status:");
                ImGui::Spacing();
                
                if (settings.aimAssist) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ON]  AimAssist");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[OFF] AimAssist");
                }
                
                if (settings.triggerbot) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ON]  Triggerbot");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[OFF] Triggerbot");
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Панель статистики
                if (settings.showStats) {
                    ImGui::Text("STATISTICS");
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // FPS
                    ImGui::Text("FPS: %.0f (%.1fms)", g_Statistics.fps, g_Statistics.frameTime);
                    ImGui::Spacing();
                    
                    // AimAssist stats
                    ImGui::Text("AimAssist:");
                    ImGui::Text("  Corrections: %d", g_Statistics.aimCorrections);
                    ImGui::Text("  Avg Distance: %.1fm", g_Statistics.GetAverageDistance());
                    ImGui::Text("  Headshot Attempts: %d", g_Statistics.headshotAttempts);
                    ImGui::Spacing();
                    
                    // Triggerbot stats
                    ImGui::Text("Triggerbot:");
                    ImGui::Text("  Triggers Fired: %d", g_Statistics.triggersFired);
                    ImGui::Text("  Headshot Triggers: %d", g_Statistics.headshotTriggers);
                    if (g_Statistics.lastTriggerTime > 0) {
                        DWORD timeSince = GetTickCount() - g_Statistics.lastTriggerTime;
                        ImGui::Text("  Last Trigger: %.1fs ago", timeSince / 1000.0f);
                    }
                    ImGui::Spacing();
                    
                    if (ImGui::Button("Reset Statistics")) {
                        ResetStatistics();
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
                
                ImGui::Text("INSERT - Toggle GUI");
                ImGui::Text("DELETE - Unload");
            }
            
            ImGui::EndChild();
            
            ImGui::End();
        }
        
        // Сохранить конфигурацию если были изменения
        if (settingsChanged) {
            SaveConfig();
        }
        
        // Тестовый рендер - рисуем текст напрямую на экране
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddText(ImVec2(10, 10), IM_COL32(255, 255, 255, 255), "HurtDLC Loaded - Press INSERT");
        
        // Отрисовать FOV круг если включен (временно отключено)
        // if (settings.aimAssist && settings.showFOV) {
        //     ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        //     ImGuiIO& io = ImGui::GetIO();
        //     
        //     // Центр экрана
        //     float centerX = io.DisplaySize.x / 2.0f;
        //     float centerY = io.DisplaySize.y / 2.0f;
        //     
        //     // Радиус FOV (примерное преобразование угла в пиксели)
        //     float fovRadius = (settings.aimAngle / 90.0f) * (io.DisplaySize.y / 4.0f);
        //     
        //     // Нарисовать круг
        //     ImVec4 accent = ImVec4(settings.accentColor[0], settings.accentColor[1], settings.accentColor[2], 0.5f);
        //     ImU32 color = ImGui::ColorConvertFloat4ToU32(accent);
        //     drawList->AddCircle(ImVec2(centerX, centerY), fovRadius, color, 64, 2.0f);
        // }
        
        // Отрисовать ESP (временно отключено)
        // ESP::Render();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    void Shutdown() {
        if (!g_Initialized) return;
        
        g_ShuttingDown = true;
        Sleep(50); // Дать время завершить текущий рендер
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        g_Initialized = false;
    }
    
    bool IsVisible() { return g_Visible; }
    void ToggleVisibility() { g_Visible = !g_Visible; }
    
    bool IsAimAssistEnabled() { return settings.aimAssist; }
    float GetAimSpeed() { return settings.aimSpeed; }
    float GetAimDistance() { return settings.aimDistance; }
    float GetAimAngle() { return settings.aimAngle; }
    bool IsHeadshotPriorityEnabled() { return settings.headshotPriority; }
    float GetHeadshotOffset() { return settings.headshotOffset; }
    bool ShowFOV() { return settings.showFOV; }
    
    bool IsTriggerbotEnabled() { return settings.triggerbot; }
    int GetTriggerbotDelay() { return settings.triggerbotDelay; }
    float GetTriggerbotDistance() { return settings.triggerbotDistance; }
    bool IsTriggerbotHeadshotOnly() { return settings.triggerbotHeadshotOnly; }
    float GetTriggerbotHeadshotThreshold() { return settings.triggerbotHeadshotThreshold; }
    
    bool IsESPEnabled() { return settings.esp; }
    bool ESPBox() { return settings.espBox; }
    bool ESPHealth() { return settings.espHealth; }
    bool ESPDistance() { return settings.espDistance; }
    bool ESPName() { return settings.espName; }
    float ESPMaxDistance() { return settings.espDistance_max; }
    
    bool IsSafeModeEnabled() { return settings.safeMode; }
    
    Statistics& GetStatistics() { return g_Statistics; }
    void ResetStatistics() { g_Statistics.Reset(); }
    
    void SaveConfig() {
        using json = nlohmann::json;
        json j;
        
        j["version"] = "1.0";
        j["aimAssist"]["enabled"] = settings.aimAssist;
        j["aimAssist"]["speed"] = settings.aimSpeed;
        j["aimAssist"]["distance"] = settings.aimDistance;
        j["aimAssist"]["angle"] = settings.aimAngle;
        j["aimAssist"]["headshotPriority"] = settings.headshotPriority;
        j["aimAssist"]["headshotOffset"] = settings.headshotOffset;
        
        j["triggerbot"]["enabled"] = settings.triggerbot;
        j["triggerbot"]["delay"] = settings.triggerbotDelay;
        j["triggerbot"]["distance"] = settings.triggerbotDistance;
        j["triggerbot"]["headshotOnly"] = settings.triggerbotHeadshotOnly;
        j["triggerbot"]["headshotThreshold"] = settings.triggerbotHeadshotThreshold;
        
        j["visual"]["accentColor"] = {
            settings.accentColor[0],
            settings.accentColor[1],
            settings.accentColor[2]
        };
        j["visual"]["showFPS"] = settings.showFPS;
        j["visual"]["showStats"] = settings.showStats;
        
        std::ofstream file("hurtdlc_config.json");
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        }
    }
    
    void LoadConfig() {
        using json = nlohmann::json;
        
        if (!std::filesystem::exists("hurtdlc_config.json")) {
            SaveConfig();
            return;
        }
        
        try {
            std::ifstream file("hurtdlc_config.json");
            if (!file.is_open()) {
                SaveConfig();
                return;
            }
            
            json j;
            file >> j;
            file.close();
            
            if (j.contains("aimAssist")) {
                settings.aimAssist = j["aimAssist"].value("enabled", false);
                settings.aimSpeed = j["aimAssist"].value("speed", 2.0f);
                settings.aimDistance = j["aimAssist"].value("distance", 4.0f);
                settings.aimAngle = j["aimAssist"].value("angle", 70.0f);
                settings.headshotPriority = j["aimAssist"].value("headshotPriority", true);
                settings.headshotOffset = j["aimAssist"].value("headshotOffset", 0.95f);
            }
            
            if (j.contains("triggerbot")) {
                settings.triggerbot = j["triggerbot"].value("enabled", false);
                settings.triggerbotDelay = j["triggerbot"].value("delay", 70);
                settings.triggerbotDistance = j["triggerbot"].value("distance", 4.0f);
                settings.triggerbotHeadshotOnly = j["triggerbot"].value("headshotOnly", false);
                settings.triggerbotHeadshotThreshold = j["triggerbot"].value("headshotThreshold", 0.80f);
            }
            
            if (j.contains("visual")) {
                auto color = j["visual"].value("accentColor", std::vector<float>{0.0f, 1.0f, 0.84f});
                if (color.size() >= 3) {
                    settings.accentColor[0] = color[0];
                    settings.accentColor[1] = color[1];
                    settings.accentColor[2] = color[2];
                }
                settings.showFPS = j["visual"].value("showFPS", true);
                settings.showStats = j["visual"].value("showStats", true);
            }
        } catch (const std::exception&) {
            SaveConfig();
        }
    }
}
