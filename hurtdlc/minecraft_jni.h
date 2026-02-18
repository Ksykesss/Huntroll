#pragma once
#include <jni.h>
#include <vector>

struct Vec3 {
    double x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
    
    double distance(const Vec3& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct Vec2 {
    float yaw, pitch;
    
    Vec2() : yaw(0), pitch(0) {}
    Vec2(float yaw, float pitch) : yaw(yaw), pitch(pitch) {}
};

class MinecraftJNI {
public:
    static bool Initialize();
    static void Shutdown();
    
    // Получить локального игрока
    static jobject GetLocalPlayer();
    
    // Получить список игроков в мире
    static std::vector<jobject> GetPlayers();
    
    // Получить позицию игрока
    static Vec3 GetPlayerPosition(jobject player);
    
    // Получить rotation игрока (yaw, pitch)
    static Vec2 GetPlayerRotation(jobject player);
    
    // Установить rotation игрока
    static void SetPlayerRotation(jobject player, Vec2 rotation);
    
    // Получить расстояние до игрока
    static double GetDistanceToPlayer(jobject player);
    
    // Проверить жив ли игрок
    static bool IsPlayerAlive(jobject player);
    
private:
    static jclass minecraftClass;
    static jclass entityPlayerClass;
    static jclass worldClass;
    
    static jmethodID getMinecraftMethod;
    static jmethodID getPlayerMethod;
    static jmethodID getWorldMethod;
    static jmethodID getPlayersMethod;
    
    static jfieldID posXField;
    static jfieldID posYField;
    static jfieldID posZField;
    static jfieldID rotationYawField;
    static jfieldID rotationPitchField;
};
