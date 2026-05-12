#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define LOGFILE "/storage/emulated/0/samp/shadow_log.txt"
#define EXPORT __attribute__((visibility("default")))

// Macro THUMB wajib untuk GTA SA Android
#define THUMB(a) ((void*)((uintptr_t)(a) | 1u))

static void logf(const char* fmt, ...) {
    char buf[512]; va_list ap;
    va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    FILE* f = fopen(LOGFILE, "a");
    if (f) { fprintf(f, "%s\n", buf); fclose(f); }
}

static int (*orig_UseAdvancedShadows)(int) = nullptr;
static int hook_UseAdvancedShadows(int arg) {
    return orig_UseAdvancedShadows(1); // Paksa return true
}

static void* init_thread(void*) {
    // Beri waktu 5 detik agar memori game stabil (Bypass Scudo/SIGBUS Crash)
    sleep(5);
    logf("[Shadow] init_thread mulai berjalan...");

    void* hGTASA = dlopen("libGTASA.so", RTLD_NOW | RTLD_NOLOAD);
    if (!hGTASA) { logf("[Shadow] FATAL: libGTASA.so belum siap"); return nullptr; }

    void* hDobby = dlopen("libdobby.so", RTLD_NOW | RTLD_GLOBAL);
    if (!hDobby) { logf("[Shadow] FATAL: libdobby.so tidak ditemukan"); return nullptr; }

    auto dobbyHook = (int(*)(void*,void*,void**))dlsym(hDobby, "DobbyHook");
    if (!dobbyHook) { logf("[Shadow] FATAL: DobbyHook gagal diload"); return nullptr; }

    // Pakai dlsym bawaan C++ (Jauh lebih aman dari DobbySymbolResolver)
    void* target_addr = dlsym(hGTASA, "_Z18UseAdvancedShadowsi");

    if (target_addr) {
        void* final_target = THUMB(target_addr);
        int rc = dobbyHook(final_target, (void*)hook_UseAdvancedShadows, (void**)&orig_UseAdvancedShadows);
        if (rc == 0) {
            logf("[Shadow] ✅ SUKSES: PC Shadows Hook Terpasang di %p", final_target);
        } else {
            logf("[Shadow] ❌ GAGAL: DobbyHook error code %d", rc);
        }
    } else {
        logf("[Shadow] ❌ GAGAL: Simbol tidak ditemukan (stripped)");
    }
    return nullptr;
}

extern "C" {
    EXPORT void* __GetModInfo() {
        return (void*)"PCShadows|2.0|Threaded Shadows|Riski Boren";
    }
    EXPORT void OnModPreLoad() {
        remove(LOGFILE);
        logf("[Shadow] OnModPreLoad...");
    }
    EXPORT void OnModLoad() {
        logf("[Shadow] OnModLoad - Memulai thread anti-crash");
        pthread_t t;
        pthread_create(&t, nullptr, init_thread, nullptr);
        pthread_detach(t);
    }
}
