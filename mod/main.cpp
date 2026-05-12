#include <stdint.h>
#include <dlfcn.h>
#include <android/log.h>
#include <stdio.h>

#define LOGFILE "/storage/emulated/0/samp/shadow_log.txt"
#define EXPORT __attribute__((visibility("default")))

static void logf(const char* msg) {
    FILE* f = fopen(LOGFILE, "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static int (*orig_UseAdvancedShadows)(int) = nullptr;
static int hook_UseAdvancedShadows(int arg) {
    return orig_UseAdvancedShadows(1);
}

extern "C" {
    EXPORT void* __GetModInfo() {
        return (void*)"PCShadows|1.0|Full PC Shadows|Riski Boren";
    }

    EXPORT void OnModPreLoad() {
        remove(LOGFILE);
        logf("[Shadow] OnModPreLoad...");
    }

    EXPORT void OnModLoad() {
        logf("[Shadow] OnModLoad mulai");
        
        void* hDobby = dlopen("libdobby.so", RTLD_NOW | RTLD_GLOBAL);
        if (!hDobby) { logf("[Shadow] GAGAL: libdobby.so tidak ditemukan"); return; }

        auto resolver = (void*(*)(const char*, const char*))dlsym(hDobby, "DobbySymbolResolver");
        auto dobbyHook = (int(*)(void*, void*, void**))dlsym(hDobby, "DobbyHook");

        if (!resolver || !dobbyHook) { logf("[Shadow] GAGAL: API Dobby tidak ditemukan"); return; }

        void* target = resolver("libGTASA.so", "_Z18UseAdvancedShadowsi");
        if (target) {
            if (dobbyHook(target, (void*)hook_UseAdvancedShadows, (void**)&orig_UseAdvancedShadows) == 0) {
                logf("[Shadow] SUKSES: Hook PC Shadows terpasang di _Z18UseAdvancedShadowsi");
            } else {
                logf("[Shadow] GAGAL: DobbyHook return error");
            }
        } else {
            logf("[Shadow] GAGAL: Simbol _Z18UseAdvancedShadowsi tidak ada di memori");
        }
    }
}
