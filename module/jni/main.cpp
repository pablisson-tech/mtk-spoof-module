#include <jni.h>
#include <string.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>
#include "zygisk.hpp"
#include "dobby.h"
#include "vulkan_hook.h"

#define LOG_TAG "MTK_SPOOF"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// --- DADOS FALSOS (SNAPDRAGON 8 GEN 1) ---
const char* FAKE_BOARD = "taro";
const char* FAKE_MODEL = "SM8450";
const char* FAKE_HARDWARE = "qcom";
const char* FAKE_PLATFORM = "msm8998"; // Para Winlator ou exagear às vezes precisa ser msm
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

class MtkSpoofModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        
        bool is_emulator = false;
        const char* emulators[] = {
            "org.yuzu.yuzu_emu",
            "org.yuzu.suyu_emu",
            "org.yuzu.sudachi_emu",
            "skyline.emu",
            "com.winlator",
            "com.termux.x11",
            "com.emulator.fpse",
            "org.ppsspp.ppsspp",
            "xyz.aethersx2.android",
            "com.aethersx2.android",
            "org.dolphinemu.dolphinemu"
        };

        for (const char* emu : emulators) {
            if (strstr(process_name, emu)) {
                is_emulator = true;
                break;
            }
        }
        
        if (is_emulator) {
            LOGI("MtkSpoof: Interceptando emulador -> %s", process_name);
            do_hook();
        }
        
        env->ReleaseStringUTFChars(args->nice_name, process_name);
    }

private:
    Api *api;
    JNIEnv *env;

    void do_hook() {
        void* libc = dlopen("libc.so", RTLD_NOW);
        if (libc) {
            void* prop_get = dlsym(libc, "__system_property_get");
            if (prop_get) {
                DobbyHook(prop_get, (void *)my_system_property_get, (void **)&orig_system_property_get);
            }
            void* sysconf_ptr = dlsym(libc, "sysconf");
            if (sysconf_ptr) {
                DobbyHook(sysconf_ptr, (void *)my_sysconf, (void **)&orig_sysconf);
            }
        }
        
        setup_vulkan_hooks();
    }
};

REGISTER_ZYGISK_MODULE(MtkSpoofModule)
