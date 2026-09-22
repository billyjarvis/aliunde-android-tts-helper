#include <jni.h>
#include <dlfcn.h>
#include <mutex>
#include <string>

static JavaVM* g_vm=nullptr;
static jobject g_tts=nullptr;
static std::mutex g_lock;

static JNIEnv* env_for_thread() {
    if (!g_vm) {
        void* h=dlopen("libart.so",RTLD_NOW);
        if (!h) return nullptr;
        using Fn=jint(*)(JavaVM**,jsize,jsize*);
        auto fn=(Fn)dlsym(h,"JNI_GetCreatedJavaVMs");
        if (!fn) return nullptr;
        JavaVM* vm=nullptr; jsize n=0;
        if (fn(&vm,1,&n)!=JNI_OK || n<1) return nullptr;
        g_vm=vm;
    }
    JNIEnv* env=nullptr;
    jint r=g_vm->GetEnv((void**)&env,JNI_VERSION_1_6);
    if (r==JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env,nullptr)!=JNI_OK) return nullptr;
    } else if (r!=JNI_OK) return nullptr;
    return env;
}

static jobject application_context(JNIEnv* e) {
    jclass at=e->FindClass("android/app/ActivityThread");
    if (!at) { e->ExceptionClear(); return nullptr; }
    jmethodID mid=e->GetStaticMethodID(at,"currentApplication","()Landroid/app/Application;");
    if (!mid) { e->ExceptionClear(); e->DeleteLocalRef(at); return nullptr; }
    jobject app=e->CallStaticObjectMethod(at,mid);
    if (e->ExceptionCheck()) { e->ExceptionClear(); app=nullptr; }
    e->DeleteLocalRef(at);
    return app;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_init() {
    std::lock_guard<std::mutex> guard(g_lock);
    JNIEnv* e=env_for_thread(); if (!e) return -1;
    if (g_tts) return 0;
    jobject ctx=application_context(e); if (!ctx) return -2;
    jclass cls=e->FindClass("android/speech/tts/TextToSpeech");
    if (!cls) { e->ExceptionClear(); e->DeleteLocalRef(ctx); return -3; }
    jmethodID ctor=e->GetMethodID(cls,"<init>","(Landroid/content/Context;Landroid/speech/tts/TextToSpeech$OnInitListener;)V");
    if (!ctor) { e->ExceptionClear(); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx); return -4; }
    jobject local=e->NewObject(cls,ctor,ctx,nullptr);
    if (e->ExceptionCheck() || !local) { e->ExceptionClear(); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx); return -5; }
    g_tts=e->NewGlobalRef(local);
    e->DeleteLocalRef(local); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx);
    return g_tts?0:-6;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_stop() {
    std::lock_guard<std::mutex> guard(g_lock);
    JNIEnv* e=env_for_thread(); if (!e || !g_tts) return -1;
    jclass cls=e->GetObjectClass(g_tts);
    jmethodID mid=e->GetMethodID(cls,"stop","()I");
    if (!mid) { e->ExceptionClear(); e->DeleteLocalRef(cls); return -2; }
    jint r=e->CallIntMethod(g_tts,mid);
    if (e->ExceptionCheck()) { e->ExceptionClear(); r=-3; }
    e->DeleteLocalRef(cls); return (int)r;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_speak(const char* utf8,float rate,float volume) {
    std::lock_guard<std::mutex> guard(g_lock);
    if (!utf8 || !*utf8) return -1;
    JNIEnv* e=env_for_thread(); if (!e || !g_tts) return -2;
    jclass cls=e->GetObjectClass(g_tts);
    jmethodID setRate=e->GetMethodID(cls,"setSpeechRate","(F)I");
    if (setRate) e->CallIntMethod(g_tts,setRate,rate);
    if (e->ExceptionCheck()) e->ExceptionClear();
    jclass bundleCls=e->FindClass("android/os/Bundle");
    jmethodID bundleCtor=e->GetMethodID(bundleCls,"<init>","()V");
    jobject bundle=e->NewObject(bundleCls,bundleCtor);
    jmethodID putFloat=e->GetMethodID(bundleCls,"putFloat","(Ljava/lang/String;F)V");
    jstring volKey=e->NewStringUTF("volume");
    e->CallVoidMethod(bundle,putFloat,volKey,volume);
    jstring text=e->NewStringUTF(utf8);
    jstring utter=e->NewStringUTF("aliunde");
    jmethodID speak=e->GetMethodID(cls,"speak","(Ljava/lang/CharSequence;ILandroid/os/Bundle;Ljava/lang/String;)I");
    if (!speak) { e->ExceptionClear(); return -3; }
    jint r=e->CallIntMethod(g_tts,speak,text,0,bundle,utter);
    if (e->ExceptionCheck()) { e->ExceptionClear(); r=-4; }
    e->DeleteLocalRef(utter); e->DeleteLocalRef(text); e->DeleteLocalRef(volKey);
    e->DeleteLocalRef(bundle); e->DeleteLocalRef(bundleCls); e->DeleteLocalRef(cls);
    return (int)r;
}
