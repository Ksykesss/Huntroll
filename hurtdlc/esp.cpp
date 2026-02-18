#include "esp.h"
#include "gui.h"
#include "jni_helper.h"
#include "minecraft_classes.h"
#include "ext/imgui/imgui.h"
#include <string>

namespace ESP {
    static bool initialized = false;
    
    void Initialize() {
        initialized = true;
    }
    
    void Shutdown() {
        initialized = false;
    }
    
    void Render() {
        if (!initialized) return;
        if (!GUI::IsESPEnabled()) return;
        
        JNIEnv* env = JNI::GetEnv();
        if (!env) return;
        
        // Получить локального игрока
        jobject localPlayer = MinecraftClasses::GetLocalPlayer(env);
        if (!localPlayer) return;
        
        // Получить мир
        jobject world = MinecraftClasses::GetWorld(env);
        if (!world) return;
        
        // Получить список сущностей
        jobject entityList = MinecraftClasses::GetEntityList(env, world);
        if (!entityList) return;
        
        // Получить позицию локального игрока
        Vec3 localPos = MinecraftClasses::GetEntityPosition(env, localPlayer);
        float eyeHeight = MinecraftClasses::GetEyeHeight(env, localPlayer);
        localPos.y += eyeHeight;
        
        float maxDistance = GUI::ESPMaxDistance();
        
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();
        
        jint listSize = env->CallIntMethod(entityList, MinecraftClasses::listSizeMethod);
        
        for (jint i = 0; i < listSize; i++) {
            jobject entity = env->CallObjectMethod(entityList, MinecraftClasses::listGetMethod, i);
            if (!entity) continue;
            
            // Пропустить самого себя
            if (env->IsSameObject(entity, localPlayer)) {
                env->DeleteLocalRef(entity);
                continue;
            }
            
            // Проверить что сущность жива
            if (!MinecraftClasses::IsEntityAlive(env, entity)) {
                env->DeleteLocalRef(entity);
                continue;
            }
            
            // Получить позицию сущности
            Vec3 entityPos = MinecraftClasses::GetEntityPosition(env, entity);
            
            // Проверить дистанцию
            double distance = localPos.distance(entityPos);
            if (distance > maxDistance) {
                env->DeleteLocalRef(entity);
                continue;
            }
            
            // Получить bounding box
            jobject bbox = MinecraftClasses::GetEntityBoundingBox(env, entity);
            if (!bbox) {
                env->DeleteLocalRef(entity);
                continue;
            }
            
            double minX, minY, minZ, maxX, maxY, maxZ;
            MinecraftClasses::GetAABBBounds(env, bbox, minX, minY, minZ, maxX, maxY, maxZ);
            
            // Простая проекция 3D -> 2D (примерная, для демонстрации)
            // В реальности нужна матрица проекции из Minecraft
            float screenX = io.DisplaySize.x / 2.0f + (float)(entityPos.x - localPos.x) * 50.0f;
            float screenY = io.DisplaySize.y / 2.0f - (float)(entityPos.y - localPos.y) * 50.0f;
            
            // Размер бокса на экране
            float boxWidth = (float)(maxX - minX) * 50.0f;
            float boxHeight = (float)(maxY - minY) * 50.0f;
            
            ImU32 color = IM_COL32(255, 0, 0, 255);
            
            // Рисовать бокс
            if (GUI::ESPBox()) {
                drawList->AddRect(
                    ImVec2(screenX - boxWidth / 2, screenY - boxHeight),
                    ImVec2(screenX + boxWidth / 2, screenY),
                    color, 0.0f, 0, 2.0f
                );
            }
            
            // Рисовать дистанцию
            if (GUI::ESPDistance()) {
                char distText[32];
                snprintf(distText, sizeof(distText), "%.1fm", distance);
                drawList->AddText(ImVec2(screenX, screenY + 5), color, distText);
            }
            
            env->DeleteLocalRef(entity);
        }
    }
}
