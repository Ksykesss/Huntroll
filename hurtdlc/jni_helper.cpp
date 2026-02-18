#include "jni_helper.h"
#include <vector>
#include <windows.h>
#include <string>

namespace JNI {
    JavaVM* jvm = nullptr;
    JNIEnv* env = nullptr;
    jobject classLoader = nullptr;
    jmethodID loadClassMethod = nullptr;
    
    typedef jint(JNICALL* JNI_GetCreatedJavaVMs_t)(JavaVM**, jsize, jsize*);
    
    bool FindClassLoaderByThreads(JNIEnv* env, const char* loaderName) {
        // Найти Thread класс
        jclass threadCls = env->FindClass("java/lang/Thread");
        if (!threadCls) return false;
        
        // Получить getAllStackTraces
        jmethodID allStackTracesMd = env->GetStaticMethodID(threadCls, "getAllStackTraces", "()Ljava/util/Map;");
        if (!allStackTracesMd) return false;
        
        jobject threadMap = env->CallStaticObjectMethod(threadCls, allStackTracesMd);
        if (!threadMap) return false;
        
        // Получить entrySet
        jclass mapCls = env->FindClass("java/util/Map");
        jmethodID entrySetMd = env->GetMethodID(mapCls, "entrySet", "()Ljava/util/Set;");
        jobject entrySet = env->CallObjectMethod(threadMap, entrySetMd);
        
        // Получить iterator
        jclass setCls = env->FindClass("java/util/Set");
        jmethodID iteratorMd = env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;");
        jobject it = env->CallObjectMethod(entrySet, iteratorMd);
        
        jclass itCls = env->FindClass("java/util/Iterator");
        jmethodID hasNextMd = env->GetMethodID(itCls, "hasNext", "()Z");
        jmethodID nextMd = env->GetMethodID(itCls, "next", "()Ljava/lang/Object;");
        
        jclass entryCls = env->FindClass("java/util/Map$Entry");
        jmethodID getKeyMd = env->GetMethodID(entryCls, "getKey", "()Ljava/lang/Object;");
        jmethodID getContextMd = env->GetMethodID(threadCls, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
        
        jclass classCls = env->FindClass("java/lang/Class");
        jmethodID getNameMd = env->GetMethodID(classCls, "getName", "()Ljava/lang/String;");
        
        // Итерация по потокам
        while (env->CallBooleanMethod(it, hasNextMd)) {
            jobject entry = env->CallObjectMethod(it, nextMd);
            jobject threadObj = env->CallObjectMethod(entry, getKeyMd);
            jobject loader = env->CallObjectMethod(threadObj, getContextMd);
            
            if (!loader) continue;
            
            jclass loaderCls = env->GetObjectClass(loader);
            jstring nameStr = (jstring)env->CallObjectMethod(loaderCls, getNameMd);
            const char* nameC = env->GetStringUTFChars(nameStr, nullptr);
            
            if (strcmp(nameC, loaderName) == 0) {
                classLoader = env->NewGlobalRef(loader);
                env->ReleaseStringUTFChars(nameStr, nameC);
                env->DeleteLocalRef(nameStr);
                env->DeleteLocalRef(loaderCls);
                return true;
            }
            
            env->ReleaseStringUTFChars(nameStr, nameC);
            env->DeleteLocalRef(nameStr);
            env->DeleteLocalRef(loaderCls);
        }
        
        return false;
    }
    
    bool Initialize() {
        // Загружаем jvm.dll
        HMODULE jvmModule = GetModuleHandleA("jvm.dll");
        if (!jvmModule) {
            MessageBoxA(nullptr, "jvm.dll not found!\nMake sure Minecraft is running.", "JNI Error", MB_OK);
            return false;
        }
        
        // Получаем функцию JNI_GetCreatedJavaVMs
        JNI_GetCreatedJavaVMs_t JNI_GetCreatedJavaVMs = 
            (JNI_GetCreatedJavaVMs_t)GetProcAddress(jvmModule, "JNI_GetCreatedJavaVMs");
        
        if (!JNI_GetCreatedJavaVMs) {
            MessageBoxA(nullptr, "JNI_GetCreatedJavaVMs not found!", "JNI Error", MB_OK);
            return false;
        }
        
        // Получаем существующую JVM
        jsize vmCount;
        JavaVM* vms[1];
        jint result = JNI_GetCreatedJavaVMs(vms, 1, &vmCount);
        
        if (result != JNI_OK || vmCount == 0) {
            MessageBoxA(nullptr, "No JVM found!", "JNI Error", MB_OK);
            return false;
        }
        
        jvm = vms[0];
        
        // Получаем JNIEnv для текущего потока
        result = jvm->AttachCurrentThread((void**)&env, nullptr);
        
        if (result != JNI_OK || !env) {
            MessageBoxA(nullptr, "Failed to attach thread!", "JNI Error", MB_OK);
            return false;
        }
        
        // Найти ClassLoader - пробуем разные варианты
        bool found = false;
        
        // Fabric
        if (FindClassLoaderByThreads(env, "net.fabricmc.loader.impl.launch.knot.KnotClassLoader")) {
            found = true;
        }
        // Fabric (старая версия)
        else if (FindClassLoaderByThreads(env, "net.fabricmc.loader.launch.knot.KnotClassLoader")) {
            found = true;
        }
        // Forge
        else if (FindClassLoaderByThreads(env, "cpw.mods.modlauncher.TransformingClassLoader")) {
            found = true;
        }
        // LaunchWrapper
        else if (FindClassLoaderByThreads(env, "net.minecraft.launchwrapper.LaunchClassLoader")) {
            found = true;
        }
        // Попробовать системный ClassLoader как последний вариант
        else {
            jclass threadCls = env->FindClass("java/lang/Thread");
            if (threadCls) {
                jmethodID currentThreadMd = env->GetStaticMethodID(threadCls, "currentThread", "()Ljava/lang/Thread;");
                if (currentThreadMd) {
                    jobject currentThread = env->CallStaticObjectMethod(threadCls, currentThreadMd);
                    if (currentThread) {
                        jmethodID getContextMd = env->GetMethodID(threadCls, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
                        if (getContextMd) {
                            jobject loader = env->CallObjectMethod(currentThread, getContextMd);
                            if (loader) {
                                classLoader = env->NewGlobalRef(loader);
                                found = true;
                            }
                        }
                    }
                }
            }
        }
        
        if (!found) {
            MessageBoxA(nullptr, "Failed to find ClassLoader!\nTried: Fabric (new/old), Forge, LaunchWrapper, System", "JNI Error", MB_OK);
            return false;
        }
        
        // Получить метод loadClass
        jclass loaderCls = env->GetObjectClass(classLoader);
        loadClassMethod = env->GetMethodID(loaderCls, "loadClass", "(Ljava/lang/String;Z)Ljava/lang/Class;");
        env->DeleteLocalRef(loaderCls);
        
        if (!loadClassMethod) {
            MessageBoxA(nullptr, "Failed to find loadClass method!", "JNI Error", MB_OK);
            return false;
        }
        
        MessageBoxA(nullptr, "JNI initialized successfully!", "JNI Success", MB_OK);
        return true;
    }
    
    void Shutdown() {
        if (classLoader && env) {
            env->DeleteGlobalRef(classLoader);
            classLoader = nullptr;
        }
        
        if (jvm && env) {
            jvm->DetachCurrentThread();
        }
        jvm = nullptr;
        env = nullptr;
    }
    
    JNIEnv* GetEnv() {
        if (!env && jvm) {
            jvm->AttachCurrentThread((void**)&env, nullptr);
        }
        return env;
    }
    
    jclass FindClass(const char* className) {
        if (!env || !classLoader || !loadClassMethod) return nullptr;
        
        // Заменить / на .
        std::string classNameDot(className);
        for (char& c : classNameDot) {
            if (c == '/') c = '.';
        }
        
        jstring jname = env->NewStringUTF(classNameDot.c_str());
        if (!jname) return nullptr;
        
        jclass cls = (jclass)env->CallObjectMethod(classLoader, loadClassMethod, jname, JNI_FALSE);
        env->DeleteLocalRef(jname);
        
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            return nullptr;
        }
        
        return cls;
    }
}
