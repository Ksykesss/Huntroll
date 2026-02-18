#pragma once
#include <windows.h>

namespace GUI {
    // Структура статистики
    struct Statistics {
        // AimAssist stats
        int aimCorrections = 0;
        float totalAimDistance = 0.0f;
        int aimTargetCount = 0;
        int headshotAttempts = 0;
        
        // Triggerbot stats
        int triggersFired = 0;
        int headshotTriggers = 0;
        DWORD lastTriggerTime = 0;
        
        // Performance stats
        float fps = 0.0f;
        float frameTime = 0.0f;
        DWORD lastFrameTime = 0;
        
        void Reset() {
            aimCorrections = 0;
            totalAimDistance = 0.0f;
            aimTargetCount = 0;
            headshotAttempts = 0;
            triggersFired = 0;
            headshotTriggers = 0;
        }
        
        float GetAverageDistance() const {
            return aimTargetCount > 0 ? totalAimDistance / aimTargetCount : 0.0f;
        }
    };
    
    void Initialize(HWND hwnd);
    void Shutdown();
    void Render();
    bool IsVisible();
    void ToggleVisibility();
    
    // Getters
    bool IsAimAssistEnabled();
    float GetAimSpeed();
    float GetAimDistance();
    float GetAimAngle();
    bool IsHeadshotPriorityEnabled();
    float GetHeadshotOffset();
    bool ShowFOV();
    
    bool IsTriggerbotEnabled();
    int GetTriggerbotDelay();
    float GetTriggerbotDistance();
    bool IsTriggerbotHeadshotOnly();
    float GetTriggerbotHeadshotThreshold();
    
    // ESP
    bool IsESPEnabled();
    bool ESPBox();
    bool ESPHealth();
    bool ESPDistance();
    bool ESPName();
    float ESPMaxDistance();
    
    // Safe Mode
    bool IsSafeModeEnabled();
    
    // Statistics access
    Statistics& GetStatistics();
    void ResetStatistics();
    
    // Configuration persistence
    void SaveConfig();
    void LoadConfig();
}
