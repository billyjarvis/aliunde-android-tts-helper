#include <jni.h>
#include <dlfcn.h>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

static JavaVM* g_vm=nullptr;
static jobject g_tts=nullptr;
static std::mutex g_lock;
static std::string g_voice_list;

static JNIEnv* kodi_env() {
    using Fn=JNIEnv*(*)();
    auto fn=(Fn)dlsym(RTLD_DEFAULT,"xbmc_jnienv");
    if (!fn) fn=(Fn)dlsym(RTLD_DEFAULT,"_Z11xbmc_jnienvv");
    return fn ? fn() : nullptr;
}

static JNIEnv* env_for_thread() {
    JNIEnv* ke=kodi_env();
    if (ke) {
        if (!g_vm) ke->GetJavaVM(&g_vm);
        return ke;
    }
    if (!g_vm) {
        using Fn=jint(*)(JavaVM**,jsize,jsize*);
        auto fn=(Fn)dlsym(RTLD_DEFAULT,"JNI_GetCreatedJavaVMs");
        void* h=nullptr;
        if (!fn) {
            h=dlopen("libart.so",RTLD_NOW | RTLD_LOCAL);
            if (h) fn=(Fn)dlsym(h,"JNI_GetCreatedJavaVMs");
        }
        if (!fn) return nullptr;
        JavaVM* vm=nullptr; jsize n=0;
        if (fn(&vm,1,&n)!=JNI_OK || n<1) return nullptr;
        g_vm=vm;
    }
    JNIEnv* e=nullptr;
    jint r=g_vm->GetEnv((void**)&e,JNI_VERSION_1_6);
    if (r==JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&e,nullptr)!=JNI_OK) return nullptr;
    } else if (r!=JNI_OK) return nullptr;
    return e;
}

static jobject application_context(JNIEnv* e) {
    jclass at=e->FindClass("android/app/ActivityThread");
    if (at) {
        jmethodID mid=e->GetStaticMethodID(at,"currentApplication","()Landroid/app/Application;");
        if (mid) {
            jobject app=e->CallStaticObjectMethod(at,mid);
            if (!e->ExceptionCheck() && app) { e->DeleteLocalRef(at); return app; }
            e->ExceptionClear();
        }
        e->DeleteLocalRef(at);
    } else e->ExceptionClear();

    jclass ag=e->FindClass("android/app/AppGlobals");
    if (!ag) { e->ExceptionClear(); return nullptr; }
    jmethodID mid=e->GetStaticMethodID(ag,"getInitialApplication","()Landroid/app/Application;");
    if (!mid) { e->ExceptionClear(); e->DeleteLocalRef(ag); return nullptr; }
    jobject app=e->CallStaticObjectMethod(ag,mid);
    if (e->ExceptionCheck()) { e->ExceptionClear(); app=nullptr; }
    e->DeleteLocalRef(ag);
    return app;
}

static bool ready(JNIEnv* e) {
    if (!g_tts) return false;
    jclass cls=e->GetObjectClass(g_tts);
    jmethodID mid=e->GetMethodID(cls,"getVoices","()Ljava/util/Set;");
    if (!mid) { e->ExceptionClear(); e->DeleteLocalRef(cls); return false; }
    for (int i=0;i<40;i++) {
        jobject voices=e->CallObjectMethod(g_tts,mid);
        if (!e->ExceptionCheck() && voices) { e->DeleteLocalRef(voices); e->DeleteLocalRef(cls); return true; }
        if (e->ExceptionCheck()) e->ExceptionClear();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    e->DeleteLocalRef(cls); return false;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_init() {
    std::lock_guard<std::mutex> guard(g_lock);
    JNIEnv* e=env_for_thread(); if (!e) return -1;
    if (g_tts) return ready(e)?0:-7;
    jobject ctx=application_context(e); if (!ctx) return -2;
    jclass cls=e->FindClass("android/speech/tts/TextToSpeech");
    if (!cls) { e->ExceptionClear(); e->DeleteLocalRef(ctx); return -3; }
    jmethodID ctor=e->GetMethodID(cls,"<init>","(Landroid/content/Context;Landroid/speech/tts/TextToSpeech$OnInitListener;)V");
    if (!ctor) { e->ExceptionClear(); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx); return -4; }
    jobject local=e->NewObject(cls,ctor,ctx,nullptr);
    if (e->ExceptionCheck() || !local) { e->ExceptionClear(); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx); return -5; }
    g_tts=e->NewGlobalRef(local);
    e->DeleteLocalRef(local); e->DeleteLocalRef(cls); e->DeleteLocalRef(ctx);
    if (!g_tts) return -6;
    return ready(e)?0:-7;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_stop() {
    std::lock_guard<std::mutex> guard(g_lock);
    JNIEnv* e=env_for_thread(); if (!e || !g_tts) return -1;
    jclass cls=e->GetObjectClass(g_tts); jmethodID mid=e->GetMethodID(cls,"stop","()I");
    if (!mid) { e->ExceptionClear(); e->DeleteLocalRef(cls); return -2; }
    jint r=e->CallIntMethod(g_tts,mid); if (e->ExceptionCheck()) { e->ExceptionClear(); r=-3; }
    e->DeleteLocalRef(cls); return (int)r;
}

extern "C" __attribute__((visibility("default"))) const char* aliunde_tts_list_voices() {
    std::lock_guard<std::mutex> guard(g_lock); g_voice_list.clear();
    JNIEnv* e=env_for_thread(); if (!e || !g_tts || !ready(e)) return "";
    jclass tcls=e->GetObjectClass(g_tts); jmethodID gv=e->GetMethodID(tcls,"getVoices","()Ljava/util/Set;");
    jobject set=e->CallObjectMethod(g_tts,gv); if (!set || e->ExceptionCheck()) { e->ExceptionClear(); return ""; }
    jclass scls=e->GetObjectClass(set); jmethodID itmid=e->GetMethodID(scls,"iterator","()Ljava/util/Iterator;");
    jobject it=e->CallObjectMethod(set,itmid); jclass icls=e->GetObjectClass(it);
    jmethodID has=e->GetMethodID(icls,"hasNext","()Z"), next=e->GetMethodID(icls,"next","()Ljava/lang/Object;");
    while (e->CallBooleanMethod(it,has)) {
        jobject v=e->CallObjectMethod(it,next); jclass vc=e->GetObjectClass(v);
        jmethodID gn=e->GetMethodID(vc,"getName","()Ljava/lang/String;");
        jmethodID gl=e->GetMethodID(vc,"getLocale","()Ljava/util/Locale;");
        jstring n=(jstring)e->CallObjectMethod(v,gn); jobject loc=e->CallObjectMethod(v,gl);
        jclass lc=e->GetObjectClass(loc); jmethodID langm=e->GetMethodID(lc,"getLanguage","()Ljava/lang/String;");
        jmethodID tagm=e->GetMethodID(lc,"toLanguageTag","()Ljava/lang/String;");
        jstring lang=(jstring)e->CallObjectMethod(loc,langm); jstring tag=(jstring)e->CallObjectMethod(loc,tagm);
        const char* cs=e->GetStringUTFChars(lang,nullptr);
        if (cs && std::string(cs)=="en") {
            const char* ns=e->GetStringUTFChars(n,nullptr); const char* ts=e->GetStringUTFChars(tag,nullptr);
            if (ns && ts) { if (!g_voice_list.empty()) g_voice_list+="\n"; g_voice_list+=ns; g_voice_list+="|"; g_voice_list+=ts; }
            if (ns) e->ReleaseStringUTFChars(n,ns); if (ts) e->ReleaseStringUTFChars(tag,ts);
        }
        if (cs) e->ReleaseStringUTFChars(lang,cs);
        e->DeleteLocalRef(tag); e->DeleteLocalRef(lang); e->DeleteLocalRef(lc); e->DeleteLocalRef(loc);
        e->DeleteLocalRef(n); e->DeleteLocalRef(vc); e->DeleteLocalRef(v);
    }
    e->DeleteLocalRef(icls); e->DeleteLocalRef(it); e->DeleteLocalRef(scls); e->DeleteLocalRef(set); e->DeleteLocalRef(tcls);
    return g_voice_list.c_str();
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_set_voice(const char* wanted) {
    std::lock_guard<std::mutex> guard(g_lock);
    if (!wanted || !*wanted) return 0;
    JNIEnv* e=env_for_thread(); if (!e || !g_tts || !ready(e)) return -1;
    jclass tc=e->GetObjectClass(g_tts); jmethodID gv=e->GetMethodID(tc,"getVoices","()Ljava/util/Set;");
    jmethodID sv=e->GetMethodID(tc,"setVoice","(Landroid/speech/tts/Voice;)I");
    jobject set=e->CallObjectMethod(g_tts,gv); jclass sc=e->GetObjectClass(set); jmethodID im=e->GetMethodID(sc,"iterator","()Ljava/util/Iterator;");
    jobject it=e->CallObjectMethod(set,im); jclass ic=e->GetObjectClass(it); jmethodID has=e->GetMethodID(ic,"hasNext","()Z"), next=e->GetMethodID(ic,"next","()Ljava/lang/Object;");
    int result=-2;
    while(e->CallBooleanMethod(it,has)) {
        jobject v=e->CallObjectMethod(it,next); jclass vc=e->GetObjectClass(v); jmethodID gn=e->GetMethodID(vc,"getName","()Ljava/lang/String;");
        jstring n=(jstring)e->CallObjectMethod(v,gn); const char* ns=e->GetStringUTFChars(n,nullptr);
        bool match=ns && std::string(ns)==wanted; if(ns)e->ReleaseStringUTFChars(n,ns);
        if(match){ result=(int)e->CallIntMethod(g_tts,sv,v); e->DeleteLocalRef(n); e->DeleteLocalRef(vc); e->DeleteLocalRef(v); break; }
        e->DeleteLocalRef(n); e->DeleteLocalRef(vc); e->DeleteLocalRef(v);
    }
    e->DeleteLocalRef(ic); e->DeleteLocalRef(it); e->DeleteLocalRef(sc); e->DeleteLocalRef(set); e->DeleteLocalRef(tc);
    if(e->ExceptionCheck()){e->ExceptionClear(); return -3;} return result;
}

extern "C" __attribute__((visibility("default"))) int aliunde_tts_speak(const char* utf8,float rate,float volume) {
    std::lock_guard<std::mutex> guard(g_lock);
    if (!utf8 || !*utf8) return -1;
    JNIEnv* e=env_for_thread(); if (!e || !g_tts || !ready(e)) return -2;
    jclass cls=e->GetObjectClass(g_tts); jmethodID setRate=e->GetMethodID(cls,"setSpeechRate","(F)I");
    if (setRate) e->CallIntMethod(g_tts,setRate,rate); if(e->ExceptionCheck())e->ExceptionClear();
    jclass bc=e->FindClass("android/os/Bundle"); jmethodID ctor=e->GetMethodID(bc,"<init>","()V"); jobject b=e->NewObject(bc,ctor);
    jmethodID pf=e->GetMethodID(bc,"putFloat","(Ljava/lang/String;F)V"); jstring vk=e->NewStringUTF("volume"); e->CallVoidMethod(b,pf,vk,volume);
    jstring text=e->NewStringUTF(utf8), utter=e->NewStringUTF("aliunde");
    jmethodID speak=e->GetMethodID(cls,"speak","(Ljava/lang/CharSequence;ILandroid/os/Bundle;Ljava/lang/String;)I");
    if(!speak){e->ExceptionClear(); return -3;} jint r=e->CallIntMethod(g_tts,speak,text,0,b,utter);
    if(e->ExceptionCheck()){e->ExceptionClear();r=-4;}
    e->DeleteLocalRef(utter);e->DeleteLocalRef(text);e->DeleteLocalRef(vk);e->DeleteLocalRef(b);e->DeleteLocalRef(bc);e->DeleteLocalRef(cls); return (int)r;
}
