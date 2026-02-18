#include "triggerbot_jni.h"
#include "jni_helper.h"
#include "minecraft_classes.h"
#include "gui.h"
#include <windows.h>
#include <cmath>

namespace Triggerbot {
    static DWORD lastClickTime = 0;
    static bool initialized = false;
    static bool canRun = false;
    static DWORD initTime = 0;
    
    void SimulateClick() {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        
        Sleep(30);
        
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    
    double GetDistance(Vec3 a, Vec3 b) {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double dz = b.z - a.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    bool IsLookingAtEntity(Vec3 playerPos, Vec2 playerRot, Vec3 targetMin, Vec3 targetMax, float maxDistance) {
        // Вычислить направление взгляда
        float yawRad = playerRot.yaw * 3.14159265f / 180.0f;
        float pitchRad = playerRot.pitch * 3.14159265f / 180.0f;
        
        Vec3 lookDir;
        lookDir.x = -sin(yawRad) * cos(pitchRad);
        lookDir.y = -sin(pitchRad);
        lookDir.z = cos(yawRad) * cos(pitchRad);
        
        // Проверить на деление на ноль
        const double epsilon = 0.0001;
        if (fabs(lookDir.x) < epsilon || fabs(lookDir.y) < epsilon || fabs(lookDir.z) < epsilon) {
            return false;
        }
        
        // Проверить пересечение луча с AABB
        Vec3 invDir;
        invDir.x = 1.0 / lookDir.x;
        invDir.y = 1.0 / lookDir.y;
        invDir.z = 1.0 / lookDir.z;
        
        double t1 = (targetMin.x - playerPos.x) * invDir.x;
        double t2 = (targetMax.x - playerPos.x) * invDir.x;
        double t3 = (targetMin.y - playerPos.y) * invDir.y;
        double t4 = (targetMax.y - playerPos.y) * invDir.y;
        double t5 = (targetMin.z - playerPos.z) * invDir.z;
        double t6 = (targetMax.z - playerPos.z) * invDir.z;
        
        double tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
        double tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));
        
        if (tmax < 0 || tmin > tmax) return false;
        
        double distance = tmin > 0 ? tmin : tmax;
        return distance <= maxDistance;
    }
    
    bool IsLookingAtEntityHead(Vec3 playerPos, Vec2 playerRot, Vec3 targetMin, Vec3 targetMax, float maxDistance, float headshotThreshold) {
        // Вычислить направление взгляда
        float yawRad = playerRot.yaw * 3.14159265f / 180.0f;
        float pitchRad = playerRot.pitch * 3.14159265f / 180.0f;
        
        Vec3 lookDir;
        lookDir.x = -sin(yawRad) * cos(pitchRad);
        lookDir.y = -sin(pitchRad);
        lookDir.z = cos(yawRad) * cos(pitchRad);
        
        // Проверить на деление на ноль
        const double epsilon = 0.0001;
        if (fabs(lookDir.x) < epsilon || fabs(lookDir.y) < epsilon || fabs(lookDir.z) < epsilon) {
            return false;
        }
        
        // Ray-AABB intersection
        Vec3 invDir;
        invDir.x = 1.0 / lookDir.x;
        invDir.y = 1.0 / lookDir.y;
        invDir.z = 1.0 / lookDir.z;
        
        double t1 = (targetMin.x - playerPos.x) * invDir.x;
        double t2 = (targetMax.x - playerPos.x) * invDir.x;
        double t3 = (targetMin.y - playerPos.y) * invDir.y;
        double t4 = (targetMax.y - playerPos.y) * invDir.y;
        double t5 = (targetMin.z - playerPos.z) * invDir.z;
        double t6 = (targetMax.z - playerPos.z) * invDir.z;
        
        double tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
        double tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));
        
        // No intersection
        if (tmax < 0 || tmin > tmax) return false;
        
        // Calculate intersection point
        double t = tmin > 0 ? tmin : tmax;
        if (t > maxDistance) return false;
        
        Vec3 intersectionPoint;
        intersectionPoint.x = playerPos.x + lookDir.x * t;
        intersectionPoint.y = playerPos.y + lookDir.y * t;
        intersectionPoint.z = playerPos.z + lookDir.z * t;
        
        // Check if intersection is in headshot region
        double hitboxHeight = targetMax.y - targetMin.y;
        double intersectionHeight = intersectionPoint.y - targetMin.y;
        double heightRatio = intersectionHeight / hitboxHeight;
        
        return heightRatio >= headshotThreshold;
    }
    
    void Initialize() {
        initialized = true;
        lastClickTime = 0;
        initTime = GetTickCount();
        canRun = false;
    }
    
    void Shutdown() {
        initialized = false;
    }
    
    void Update() {
        if (!initialized) {
            return;
        }
        
        // Подождать 5 секунд после инициализации перед началом работы
        if (!canRun) {
            if (GetTickCount() - initTime > 5000) {
                canRun = true;
            } else {
                return;
            }
        }
        
        if (GUI::IsSafeModeEnabled()) {
            return;
        }
        
        if (!GUI::IsTriggerbotEnabled()) {
            return;
        }
        
        DWORD currentTime = GetTickCount();
        int delay = GUI::GetTriggerbotDelay();
        
        if (currentTime - lastClickTime < (DWORD)delay) {
            return;
        }
        
        // Не кликать если уже зажата кнопка
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            return;
        }
        
        JNIEnv* env = JNI::GetEnv();
        if (!env) return;
        
        // Проверка на исключения JNI
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return;
        }
        
        // Получить локального игрока
        jobject localPlayer = MinecraftClasses::GetLocalPlayer(env);
        if (!localPlayer || env->ExceptionCheck()) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }
        
        // Получить мир
        jobject world = MinecraftClasses::GetWorld(env);
        if (!world || env->ExceptionCheck()) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }
        
        // Получить список сущностей
        jobject entityList = MinecraftClasses::GetEntityList(env, world);
        if (!entityList || env->ExceptionCheck()) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            return;
        }
        
        // Получить позицию и ротацию локального игрока
        Vec3 localPos = MinecraftClasses::GetEntityPosition(env, localPlayer);
        float eyeHeight = MinecraftClasses::GetEyeHeight(env, localPlayer);
        localPos.y += eyeHeight;
        
        Vec2 localRot = MinecraftClasses::GetEntityRotation(env, localPlayer);
        
        float maxDistance = GUI::GetTriggerbotDistance();
        
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
            
            // Получить bounding box
            jobject bbox = MinecraftClasses::GetEntityBoundingBox(env, entity);
            if (!bbox) {
                env->DeleteLocalRef(entity);
                continue;
            }
            
            double minX, minY, minZ, maxX, maxY, maxZ;
            MinecraftClasses::GetAABBBounds(env, bbox, minX, minY, minZ, maxX, maxY, maxZ);
            
            Vec3 targetMin = {minX, minY, minZ};
            Vec3 targetMax = {maxX, maxY, maxZ};
            
            // Проверить смотрим ли мы на цель (любая точка хитбокса)
            bool shouldFire = IsLookingAtEntity(localPos, localRot, targetMin, targetMax, maxDistance);
            
            if (shouldFire) {
                SimulateClick();
                lastClickTime = currentTime;
                GUI::GetStatistics().triggersFired++;
                GUI::GetStatistics().lastTriggerTime = currentTime;
                env->DeleteLocalRef(entity);
                return;
            }
            
            env->DeleteLocalRef(entity);
        }
    }
}
