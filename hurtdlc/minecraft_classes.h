#pragma once
#include <jni.h>
#include <string>
#include <vector>

// Fabric 1.21.4 Mappings
namespace Mappings {
    // Классы
    constexpr const char* MINECRAFT_CLASS = "net/minecraft/class_310";
    constexpr const char* WORLD_CLASS = "net/minecraft/class_638";
    constexpr const char* PLAYER_CLASS = "net/minecraft/class_746";
    constexpr const char* ENTITY_CLASS = "net/minecraft/class_1297";
    constexpr const char* AABB_CLASS = "net/minecraft/class_238";
    constexpr const char* VEC3_CLASS = "net/minecraft/class_243";
    
    // Поля Minecraft
    constexpr const char* MC_INSTANCE = "field_1700";
    constexpr const char* MC_PLAYER = "field_1724";
    constexpr const char* MC_WORLD = "field_1687";
    
    // Поля Entity
    constexpr const char* ENTITY_YAW = "field_6031";
    constexpr const char* ENTITY_PITCH = "field_5965";
    constexpr const char* ENTITY_BBOX = "field_6005";
    
    // Поля AABB
    constexpr const char* AABB_MIN_X = "field_1323";
    constexpr const char* AABB_MIN_Y = "field_1322";
    constexpr const char* AABB_MIN_Z = "field_1321";
    constexpr const char* AABB_MAX_X = "field_1320";
    constexpr const char* AABB_MAX_Y = "field_1325";
    constexpr const char* AABB_MAX_Z = "field_1324";
    
    // Поля World
    constexpr const char* WORLD_ENTITIES = "field_18226";
    
    // Методы Entity
    constexpr const char* ENTITY_GET_X = "method_23317";
    constexpr const char* ENTITY_GET_Y = "method_23318";
    constexpr const char* ENTITY_GET_Z = "method_23321";
    constexpr const char* ENTITY_IS_ALIVE = "method_5805";
    constexpr const char* ENTITY_EYE_HEIGHT = "method_5751";
}

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
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
};

struct Vec2 {
    float yaw, pitch;
    
    Vec2() : yaw(0), pitch(0) {}
    Vec2(float yaw, float pitch) : yaw(yaw), pitch(pitch) {}
};

class MinecraftClasses {
public:
    static bool Initialize(JNIEnv* env);
    static void Shutdown(JNIEnv* env);
    
    // Получить инстанс Minecraft
    static jobject GetMinecraftInstance(JNIEnv* env);
    
    // Получить локального игрока
    static jobject GetLocalPlayer(JNIEnv* env);
    
    // Получить мир
    static jobject GetWorld(JNIEnv* env);
    
    // Получить список сущностей
    static jobject GetEntityList(JNIEnv* env, jobject world);
    
    // Получить позицию сущности
    static Vec3 GetEntityPosition(JNIEnv* env, jobject entity);
    
    // Получить rotation сущности
    static Vec2 GetEntityRotation(JNIEnv* env, jobject entity);
    
    // Установить rotation сущности
    static void SetEntityRotation(JNIEnv* env, jobject entity, Vec2 rotation);
    
    // Получить bounding box
    static jobject GetEntityBoundingBox(JNIEnv* env, jobject entity);
    
    // Получить размеры AABB
    static void GetAABBBounds(JNIEnv* env, jobject aabb, double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ);
    
    // Проверить жива ли сущность
    static bool IsEntityAlive(JNIEnv* env, jobject entity);
    
    // Получить высоту глаз
    static float GetEyeHeight(JNIEnv* env, jobject entity);
    
    // Публичные методы для работы со списками
    static jmethodID listSizeMethod;
    static jmethodID listGetMethod;
    
private:
    static jclass minecraftClass;
    static jclass worldClass;
    static jclass playerClass;
    static jclass entityClass;
    static jclass aabbClass;
    static jclass listClass;
    
    static jfieldID mcInstanceField;
    static jfieldID mcPlayerField;
    static jfieldID mcWorldField;
    static jfieldID worldEntitiesField;
    
    static jfieldID entityYawField;
    static jfieldID entityPitchField;
    static jfieldID entityBBoxField;
    
    static jfieldID aabbMinXField;
    static jfieldID aabbMinYField;
    static jfieldID aabbMinZField;
    static jfieldID aabbMaxXField;
    static jfieldID aabbMaxYField;
    static jfieldID aabbMaxZField;
    
    static jmethodID entityGetXMethod;
    static jmethodID entityGetYMethod;
    static jmethodID entityGetZMethod;
    static jmethodID entityIsAliveMethod;
    static jmethodID entityEyeHeightMethod;
};
