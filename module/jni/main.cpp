#include <jni.h>
#include <string.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>
#include "And64InlineHook.hpp"
#include "vulkan_hook.h"

#define LOG_TAG "MTK_SPOOF_DRIVER"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- DADOS FALSOS (SNAPDRAGON 8 GEN 1) ---
const char* FAKE_BOARD = "taro";
const char* FAKE_MODEL = "SM8450";
const char* FAKE_HARDWARE = "qcom";
const char* FAKE_PLATFORM = "msm8998";
const char* FAKE_GPU = "Adreno (TM) 730";

// --- HOOK PARA __system_property_get ---
static int (*orig_system_property_get)(const char *name, char *value);
int my_system_property_get(const char *name, char *value) {
    if (strcmp(name, "ro.board.platform") == 0 || strcmp(name, "ro.hardware") == 0) {
        strcpy(value, FAKE_HARDWARE);
        return strlen(FAKE_HARDWARE);
    }
    if (strcmp(name, "ro.product.board") == 0) {
        strcpy(value, FAKE_BOARD);
        return strlen(FAKE_BOARD);
    }
    if (strcmp(name, "ro.soc.model") == 0) {
        strcpy(value, FAKE_MODEL);
        return strlen(FAKE_MODEL);
    }
    return orig_system_property_get(name, value);
}

// --- HOOK PARA sysconf ---
static long (*orig_sysconf)(int name);
long my_sysconf(int name) {
    if (name == _SC_NPROCESSORS_CONF || name == _SC_NPROCESSORS_ONLN) {
        return 8; // Força reportar 8 núcleos
    }
    return orig_sysconf(name);
}

// O emulador vai usar dlopen() na nossa biblioteca. Assim que for carregada na memória, essa função é disparada:
__attribute__((constructor))
void spoof_init() {
    LOGI("Custom Driver Init: Injetando hooks de propriedade...");
    
    void* libc = dlopen("libc.so", RTLD_NOW);
    if (libc) {
        void* prop_get = dlsym(libc, "__system_property_get");
        if (prop_get) {
            A64HookFunction(prop_get, (void *)my_system_property_get, (void **)&orig_system_property_get);
        }
        void* sysconf_ptr = dlsym(libc, "sysconf");
        if (sysconf_ptr) {
            A64HookFunction(sysconf_ptr, (void *)my_sysconf, (void **)&orig_sysconf);
        }
    }
    
    // Inicia o Vulkan Proxy
    setup_vulkan_hooks();
}
