#include "aimassist_jni.h"
#include "jni_helper.h"
#include "minecraft_classes.h"
#include "gui.h"
#include <cmath>
#include <algorithm>
#include <windows.h>

#undef min
#undef max

namespace AimAssist {
    static jobject prevTarget = nullptr;
    static bool initialized = false;
    static bool canRun = false;
    static DWORD initTime = 0;
    static float smoothYaw = 0.0f;
    static float smoothPitch = 0.0f;
    
    // Математические функции
    double CropAngle180(double angle) {
        while (angle > 180.0) angle -= 360.0;
        while (angle < -180.0) angle += 360.0;
        return angle;
    }
    
    Vec2 GetYawPitch(Vec3 from, Vec3 to) {
        double dx = to.x - from.x;
        double dy = to.y - from.y;
        double dz = to.z - from.z;
        
        double dist = sqrt(dx * dx + dz * dz);
        
        Vec2 result;
        result.yaw = (float)(atan2(dz, dx) * 180.0 / 3.14159265358979323846 - 90.0);
        result.pitch = (float)(-atan2(dy, dist) * 180.0 / 3.14159265358979323846);
        
        return result;
    }
    
    double GetDistance(Vec3 a, Vec3 b) {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double dz = b.z - a.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
    
    Vec3 CalculateHeadshotPosition(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
        Vec3 targetPos;
        targetPos.x = minX + (maxX - minX) / 2.0;
        targetPos.z = minZ + (maxZ - minZ) / 2.0;
        
        float headshotOffset = GUI::GetHeadshotOffset();
        targetPos.y = minY + (maxY - minY) * headshotOffset;
        
        return targetPos;
    }
    
    // Найти ближайшую точку на хитбоксе к текущему прицелу
    Vec3 GetClosestPointOnHitbox(Vec3 playerPos, Vec2 playerRot, double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
        // Вычислить направление взгляда
        float yawRad = playerRot.yaw * 3.14159265f / 180.0f;
        float pitchRad = playerRot.pitch * 3.14159265f / 180.0f;
        
        Vec3 lookDir;
        lookDir.x = -sin(yawRad) * cos(pitchRad);
        lookDir.y = -sin(pitchRad);
        lookDir.z = cos(yawRad) * cos(pitchRad);
        
        // Найти ближайшую точку на AABB к лучу (fallback)
        Vec3 closestPoint;
        closestPoint.x = std::max(minX, std::min(maxX, playerPos.x));
        closestPoint.y = std::max(minY, std::min(maxY, playerPos.y));
        closestPoint.z = std::max(minZ, std::min(maxZ, playerPos.z));
        
        // Проверить на деление на ноль
        const double epsilon = 0.0001;
        if (fabs(lookDir.x) < epsilon || fabs(lookDir.y) < epsilon || fabs(lookDir.z) < epsilon) {
            // Если направление почти нулевое, вернуть центр хитбокса
            closestPoint.x = (minX + maxX) / 2.0;
            closestPoint.y = (minY + maxY) / 2.0;
            closestPoint.z = (minZ + maxZ) / 2.0;
            return closestPoint;
        }
        
        // Если луч пересекает хитбокс, найти точку пересечения
        Vec3 invDir;
        invDir.x = 1.0 / lookDir.x;
        invDir.y = 1.0 / lookDir.y;
        invDir.z = 1.0 / lookDir.z;
        
        double t1 = (minX - playerPos.x) * invDir.x;
        double t2 = (maxX - playerPos.x) * invDir.x;
        double t3 = (minY - playerPos.y) * invDir.y;
        double t4 = (maxY - playerPos.y) * invDir.y;
        double t5 = (minZ - playerPos.z) * invDir.z;
        double t6 = (maxZ - playerPos.z) * invDir.z;
        
        double tmin = fmax(fmax(fmin(t1, t2), fmin(t3, t4)), fmin(t5, t6));
        double tmax = fmin(fmin(fmax(t1, t2), fmax(t3, t4)), fmax(t5, t6));
        
        if (tmax >= 0 && tmin <= tmax && tmin > 0) {
            // Луч пересекает хитбокс - используем точку входа
            closestPoint.x = playerPos.x + lookDir.x * tmin;
            closestPoint.y = playerPos.y + lookDir.y * tmin;
            closestPoint.z = playerPos.z + lookDir.z * tmin;
        }
        
        return closestPoint;
    }
    
    void Initialize() {
        initialized = true;
        prevTarget = nullptr;
        initTime = GetTickCount();
        canRun = false;
    }
    
    void Shutdown() {
        initialized = false;
        prevTarget = nullptr;
    }
    
    void Update() {
        if (!initialized) {
            prevTarget = nullptr;
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
            prevTarget = nullptr;
            return;
        }
        
        if (!GUI::IsAimAssistEnabled()) {
            prevTarget = nullptr;
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
        double croppedYaw = CropAngle180(localRot.yaw);
        double croppedPitch = localRot.pitch;
        
        // Настройки из GUI
        float maxDistance = GUI::GetAimDistance();
        float maxAngle = GUI::GetAimAngle();
        float aimSpeed = GUI::GetAimSpeed();
        
        // Найти лучшую цель
        jobject selectedTarget = nullptr;
        double selectedYawToAdd = 0.0;
        double selectedPitchToAdd = 0.0;
        double selectedDistance = 0.0;
        
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
            
            // Вычислить позицию цели (ближайшая точка или headshot)
            Vec3 targetPos;
            if (GUI::IsHeadshotPriorityEnabled()) {
                targetPos = CalculateHeadshotPosition(minX, minY, minZ, maxX, maxY, maxZ);
            } else {
                targetPos = GetClosestPointOnHitbox(localPos, localRot, minX, minY, minZ, maxX, maxY, maxZ);
            }
            
            // Обновить статистику headshot attempts
            float headshotOffset = GUI::GetHeadshotOffset();
            if (GUI::IsHeadshotPriorityEnabled() && headshotOffset >= 0.90f) {
                GUI::GetStatistics().headshotAttempts++;
            }
            
            // Вычислить требуемую ротацию
            Vec2 requiredRot = GetYawPitch(localPos, targetPos);
            double yawToAdd = requiredRot.yaw - croppedYaw;
            double pitchToAdd = requiredRot.pitch - croppedPitch;
            yawToAdd = CropAngle180(yawToAdd);
            
            double distance = GetDistance(localPos, targetPos);
            
            // Проверить дистанцию и угол
            if (distance <= maxDistance && distance > 0.5 && fabs(yawToAdd) <= maxAngle) {
                // Приоритет предыдущей цели
                if (prevTarget && env->IsSameObject(prevTarget, entity)) {
                    selectedTarget = entity;
                    selectedYawToAdd = yawToAdd;
                    selectedPitchToAdd = pitchToAdd;
                    selectedDistance = distance;
                    env->DeleteLocalRef(entity);
                    break;
                }
                
                // Выбрать ближайшую по углу
                if (!selectedTarget || fabs(yawToAdd) < fabs(selectedYawToAdd)) {
                    if (selectedTarget) env->DeleteLocalRef(selectedTarget);
                    selectedTarget = entity;
                    selectedYawToAdd = yawToAdd;
                    selectedPitchToAdd = pitchToAdd;
                    selectedDistance = distance;
                } else {
                    env->DeleteLocalRef(entity);
                }
            } else {
                env->DeleteLocalRef(entity);
                
                // Сбросить предыдущую цель если она вышла из зоны
                if (prevTarget && env->IsSameObject(prevTarget, entity)) {
                    prevTarget = nullptr;
                }
            }
        }
        
        // Применить наведение
        if (selectedTarget) {
            prevTarget = selectedTarget;
            
            // Плавная интерполяция
            float yawDiff = (float)selectedYawToAdd;
            float pitchDiff = (float)selectedPitchToAdd;
            
            // Базовая скорость с рандомизацией (уменьшена для обхода античита)
            float speedMultiplier = aimSpeed * 0.08f;
            float randomFactor = 0.85f + (rand() % 30) / 100.0f; // 0.85-1.15
            speedMultiplier *= randomFactor;
            
            // Distance-based speed adjustment
            float distanceMultiplier = 1.0f;
            if (selectedDistance > 5.0) {
                distanceMultiplier = 0.7f;  // 30% медленнее для дальних целей
            } else if (selectedDistance > 3.0) {
                distanceMultiplier = 0.85f; // 15% медленнее для средних целей
            }
            speedMultiplier *= distanceMultiplier;
            
            // Aim speed limiting near target
            if (selectedDistance < 1.0) {
                speedMultiplier *= 0.5f; // 50% от максимума вблизи
            }
            
            // Увеличенное сглаживание для headshot mode
            float yawSmoothing = 0.7f;
            float pitchSmoothing = 0.7f;
            if (GUI::IsHeadshotPriorityEnabled()) {
                yawSmoothing = 0.75f;
                pitchSmoothing = 0.80f;  // Больше сглаживания по вертикали
            }
            
            // Интерполяция к целевым значениям
            smoothYaw = smoothYaw * yawSmoothing + yawDiff * (1.0f - yawSmoothing);
            smoothPitch = smoothPitch * pitchSmoothing + pitchDiff * (1.0f - pitchSmoothing);
            
            // Обновить статистику
            GUI::GetStatistics().aimCorrections++;
            GUI::GetStatistics().totalAimDistance += (float)selectedDistance;
            GUI::GetStatistics().aimTargetCount++;
            
            // Применяем только если разница значительная
            if (fabs(smoothYaw) > 0.05 || fabs(smoothPitch) > 0.05) {
                // Yaw adjustment (уменьшено для обхода античита)
                if (fabs(smoothYaw) > 0.05) {
                    float step = smoothYaw * speedMultiplier;
                    step = std::min(fabs(step), fabs(smoothYaw) * 0.25f); // Максимум 25% за кадр
                    localRot.yaw += step * (smoothYaw > 0.0f ? 1.0f : -1.0f);
                }
                
                // Pitch adjustment (уменьшено для обхода античита)
                if (fabs(smoothPitch) > 0.05) {
                    float step = smoothPitch * speedMultiplier * 0.75f;
                    step = std::min(fabs(step), fabs(smoothPitch) * 0.20f); // Максимум 20% за кадр
                    localRot.pitch += step * (smoothPitch > 0.0f ? 1.0f : -1.0f);
                }
                
                // Применить новую ротацию с проверкой
                try {
                    MinecraftClasses::SetEntityRotation(env, localPlayer, localRot);
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                    }
                } catch (...) {
                    // Игнорировать исключения
                }
            }
        } else {
            // Сбросить сглаживание если нет цели
            smoothYaw *= 0.8f;
            smoothPitch *= 0.8f;
        }
    }
}
