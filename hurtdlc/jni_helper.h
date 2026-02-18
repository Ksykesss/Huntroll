#pragma once
#include <jni.h>
#include <windows.h>

namespace JNI {
    extern JavaVM* jvm;
    extern JNIEnv* env;
    extern jobject classLoader;
    extern jmethodID loadClassMethod;
    
    bool Initialize();
    void Shutdown();
    JNIEnv* GetEnv();
    jclass FindClass(const char* className);
}
