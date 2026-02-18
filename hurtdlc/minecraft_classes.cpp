#include "minecraft_classes.h"
#include "jni_helper.h"
#include <windows.h>

// Статические переменные
jclass MinecraftClasses::minecraftClass = nullptr;
jclass MinecraftClasses::worldClass = nullptr;
jclass MinecraftClasses::playerClass = nullptr;
jclass MinecraftClasses::entityClass = nullptr;
jclass MinecraftClasses::aabbClass = nullptr;
jclass MinecraftClasses::listClass = nullptr;

jfieldID MinecraftClasses::mcInstanceField = nullptr;
jfieldID MinecraftClasses::mcPlayerField = nullptr;
jfieldID MinecraftClasses::mcWorldField = nullptr;
jfieldID MinecraftClasses::worldEntitiesField = nullptr;

jfieldID MinecraftClasses::entityYawField = nullptr;
jfieldID MinecraftClasses::entityPitchField = nullptr;
jfieldID MinecraftClasses::entityBBoxField = nullptr;

jfieldID MinecraftClasses::aabbMinXField = nullptr;
jfieldID MinecraftClasses::aabbMinYField = nullptr;
jfieldID MinecraftClasses::aabbMinZField = nullptr;
jfieldID MinecraftClasses::aabbMaxXField = nullptr;
jfieldID MinecraftClasses::aabbMaxYField = nullptr;
jfieldID MinecraftClasses::aabbMaxZField = nullptr;

jmethodID MinecraftClasses::entityGetXMethod = nullptr;
jmethodID MinecraftClasses::entityGetYMethod = nullptr;
jmethodID MinecraftClasses::entityGetZMethod = nullptr;
jmethodID MinecraftClasses::entityIsAliveMethod = nullptr;
jmethodID MinecraftClasses::entityEyeHeightMethod = nullptr;

jmethodID MinecraftClasses::listSizeMethod = nullptr;
jmethodID MinecraftClasses::listGetMethod = nullptr;

bool MinecraftClasses::Initialize(JNIEnv* env) {
    if (!env) return false;
    
    // Очистить исключения перед началом
    env->ExceptionClear();
    
    // Найти класс Minecraft через ClassLoader
    jclass localMinecraftClass = JNI::FindClass("net/minecraft/class_310");
    if (!localMinecraftClass) {
        MessageBoxA(nullptr, "Failed to find Minecraft class: net/minecraft/class_310", "Error", MB_OK);
        return false;
    }
    minecraftClass = (jclass)env->NewGlobalRef(localMinecraftClass);
    env->DeleteLocalRef(localMinecraftClass);
    
    // Найти класс World
    jclass localWorldClass = JNI::FindClass("net/minecraft/class_638");
    if (!localWorldClass) {
        MessageBoxA(nullptr, "Failed to find World class: net/minecraft/class_638", "Error", MB_OK);
        return false;
    }
    worldClass = (jclass)env->NewGlobalRef(localWorldClass);
    env->DeleteLocalRef(localWorldClass);
    
    // Найти класс Entity
    jclass localEntityClass = JNI::FindClass("net/minecraft/class_1297");
    if (!localEntityClass) {
        MessageBoxA(nullptr, "Failed to find Entity class: net/minecraft/class_1297", "Error", MB_OK);
        return false;
    }
    entityClass = (jclass)env->NewGlobalRef(localEntityClass);
    env->DeleteLocalRef(localEntityClass);
    
    // Найти класс AABB
    jclass localAABBClass = JNI::FindClass("net/minecraft/class_238");
    if (!localAABBClass) {
        MessageBoxA(nullptr, "Failed to find AABB class: net/minecraft/class_238", "Error", MB_OK);
        return false;
    }
    aabbClass = (jclass)env->NewGlobalRef(localAABBClass);
    env->DeleteLocalRef(localAABBClass);
    
    // Найти класс List
    jclass localListClass = env->FindClass("java/util/List");
    if (!localListClass || env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find List class", "Error", MB_OK);
        return false;
    }
    listClass = (jclass)env->NewGlobalRef(localListClass);
    env->DeleteLocalRef(localListClass);
    
    // Получить поля Minecraft
    mcInstanceField = env->GetStaticFieldID(minecraftClass, Mappings::MC_INSTANCE, "Lnet/minecraft/class_310;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find MC_INSTANCE field", "Error", MB_OK);
        return false;
    }
    
    mcPlayerField = env->GetFieldID(minecraftClass, Mappings::MC_PLAYER, "Lnet/minecraft/class_746;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find MC_PLAYER field", "Error", MB_OK);
        return false;
    }
    
    mcWorldField = env->GetFieldID(minecraftClass, Mappings::MC_WORLD, "Lnet/minecraft/class_638;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find MC_WORLD field", "Error", MB_OK);
        return false;
    }
    
    // Получить поля World
    worldEntitiesField = env->GetFieldID(worldClass, Mappings::WORLD_ENTITIES, "Ljava/util/List;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find WORLD_ENTITIES field", "Error", MB_OK);
        return false;
    }
    
    // Получить поля Entity
    entityYawField = env->GetFieldID(entityClass, Mappings::ENTITY_YAW, "F");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityPitchField = env->GetFieldID(entityClass, Mappings::ENTITY_PITCH, "F");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityBBoxField = env->GetFieldID(entityClass, Mappings::ENTITY_BBOX, "Lnet/minecraft/class_238;");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    // Получить поля AABB
    aabbMinXField = env->GetFieldID(aabbClass, Mappings::AABB_MIN_X, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    aabbMinYField = env->GetFieldID(aabbClass, Mappings::AABB_MIN_Y, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    aabbMinZField = env->GetFieldID(aabbClass, Mappings::AABB_MIN_Z, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    aabbMaxXField = env->GetFieldID(aabbClass, Mappings::AABB_MAX_X, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    aabbMaxYField = env->GetFieldID(aabbClass, Mappings::AABB_MAX_Y, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    aabbMaxZField = env->GetFieldID(aabbClass, Mappings::AABB_MAX_Z, "D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    // Получить методы Entity
    entityGetXMethod = env->GetMethodID(entityClass, Mappings::ENTITY_GET_X, "()D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityGetYMethod = env->GetMethodID(entityClass, Mappings::ENTITY_GET_Y, "()D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityGetZMethod = env->GetMethodID(entityClass, Mappings::ENTITY_GET_Z, "()D");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityIsAliveMethod = env->GetMethodID(entityClass, Mappings::ENTITY_IS_ALIVE, "()Z");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    entityEyeHeightMethod = env->GetMethodID(entityClass, Mappings::ENTITY_EYE_HEIGHT, "()F");
    if (env->ExceptionCheck()) env->ExceptionClear();
    
    // Получить методы List
    listSizeMethod = env->GetMethodID(listClass, "size", "()I");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find List.size method", "Error", MB_OK);
        return false;
    }
    
    listGetMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        MessageBoxA(nullptr, "Failed to find List.get method", "Error", MB_OK);
        return false;
    }
    
    // Проверить критичные поля
    if (!mcInstanceField || !mcPlayerField || !mcWorldField || !listSizeMethod || !listGetMethod) {
        MessageBoxA(nullptr, "Failed to find critical fields or methods", "Error", MB_OK);
        return false;
    }
    
    return true;
}

void MinecraftClasses::Shutdown(JNIEnv* env) {
    if (!env) return;
    
    if (minecraftClass) env->DeleteGlobalRef(minecraftClass);
    if (worldClass) env->DeleteGlobalRef(worldClass);
    if (playerClass) env->DeleteGlobalRef(playerClass);
    if (entityClass) env->DeleteGlobalRef(entityClass);
    if (aabbClass) env->DeleteGlobalRef(aabbClass);
    if (listClass) env->DeleteGlobalRef(listClass);
    
    minecraftClass = nullptr;
    worldClass = nullptr;
    playerClass = nullptr;
    entityClass = nullptr;
    aabbClass = nullptr;
    listClass = nullptr;
}

jobject MinecraftClasses::GetMinecraftInstance(JNIEnv* env) {
    if (!env || !minecraftClass || !mcInstanceField) return nullptr;
    return env->GetStaticObjectField(minecraftClass, mcInstanceField);
}

jobject MinecraftClasses::GetLocalPlayer(JNIEnv* env) {
    if (!env) return nullptr;
    jobject mc = GetMinecraftInstance(env);
    if (!mc) return nullptr;
    return env->GetObjectField(mc, mcPlayerField);
}

jobject MinecraftClasses::GetWorld(JNIEnv* env) {
    if (!env) return nullptr;
    jobject mc = GetMinecraftInstance(env);
    if (!mc) return nullptr;
    return env->GetObjectField(mc, mcWorldField);
}

jobject MinecraftClasses::GetEntityList(JNIEnv* env, jobject world) {
    if (!env || !world || !worldEntitiesField) return nullptr;
    return env->GetObjectField(world, worldEntitiesField);
}

Vec3 MinecraftClasses::GetEntityPosition(JNIEnv* env, jobject entity) {
    Vec3 pos;
    if (!env || !entity) return pos;
    
    pos.x = env->CallDoubleMethod(entity, entityGetXMethod);
    pos.y = env->CallDoubleMethod(entity, entityGetYMethod);
    pos.z = env->CallDoubleMethod(entity, entityGetZMethod);
    
    return pos;
}

Vec2 MinecraftClasses::GetEntityRotation(JNIEnv* env, jobject entity) {
    Vec2 rot;
    if (!env || !entity) return rot;
    
    rot.yaw = env->GetFloatField(entity, entityYawField);
    rot.pitch = env->GetFloatField(entity, entityPitchField);
    
    return rot;
}

void MinecraftClasses::SetEntityRotation(JNIEnv* env, jobject entity, Vec2 rotation) {
    if (!env || !entity) return;
    
    env->SetFloatField(entity, entityYawField, rotation.yaw);
    env->SetFloatField(entity, entityPitchField, rotation.pitch);
}

jobject MinecraftClasses::GetEntityBoundingBox(JNIEnv* env, jobject entity) {
    if (!env || !entity || !entityBBoxField) return nullptr;
    return env->GetObjectField(entity, entityBBoxField);
}

void MinecraftClasses::GetAABBBounds(JNIEnv* env, jobject aabb, double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ) {
    if (!env || !aabb) return;
    
    minX = env->GetDoubleField(aabb, aabbMinXField);
    minY = env->GetDoubleField(aabb, aabbMinYField);
    minZ = env->GetDoubleField(aabb, aabbMinZField);
    maxX = env->GetDoubleField(aabb, aabbMaxXField);
    maxY = env->GetDoubleField(aabb, aabbMaxYField);
    maxZ = env->GetDoubleField(aabb, aabbMaxZField);
}

bool MinecraftClasses::IsEntityAlive(JNIEnv* env, jobject entity) {
    if (!env || !entity || !entityIsAliveMethod) return false;
    return env->CallBooleanMethod(entity, entityIsAliveMethod);
}

float MinecraftClasses::GetEyeHeight(JNIEnv* env, jobject entity) {
    if (!env || !entity || !entityEyeHeightMethod) return 0.0f;
    return env->CallFloatMethod(entity, entityEyeHeightMethod);
}
