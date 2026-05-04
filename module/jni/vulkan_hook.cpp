#include "vulkan_hook.h"
#include <vulkan/vulkan.h>
#include <string.h>
#include <vector>
#include <dlfcn.h>
#include <android/log.h>
#include "And64InlineHook.hpp"

#define LOG_TAG "MTK_VULKAN_PROXY"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void* sys_vulkan_handle = nullptr;

static PFN_vkEnumerateDeviceExtensionProperties real_vkEnumerateDeviceExtensionProperties = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL my_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) 
{
    if (!real_vkEnumerateDeviceExtensionProperties) {
        if (!sys_vulkan_handle) sys_vulkan_handle = dlopen("libvulkan.so", RTLD_NOW);
        real_vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)dlsym(sys_vulkan_handle, "vkEnumerateDeviceExtensionProperties");
    }

    uint32_t realCount = 0;
    real_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, nullptr);
    
    std::vector<VkExtensionProperties> realProps(realCount);
    real_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, realProps.data());

    if (pProperties == nullptr) {
        *pPropertyCount = realCount + 2; 
        return VK_SUCCESS;
    }

    uint32_t requestedCount = *pPropertyCount;
    uint32_t copyCount = (requestedCount < realCount) ? requestedCount : realCount;
    
    for (uint32_t i = 0; i < copyCount; i++) {
        pProperties[i] = realProps[i];
    }

    if (copyCount + 1 <= requestedCount) {
        strncpy(pProperties[copyCount].extensionName, "VK_QCOM_render_pass_transform", VK_MAX_EXTENSION_NAME_SIZE);
        pProperties[copyCount].specVersion = 1;
        *pPropertyCount = copyCount + 1;
    }
    
    if (copyCount + 2 <= requestedCount) {
        strncpy(pProperties[copyCount + 1].extensionName, "VK_QCOM_render_pass_store_ops", VK_MAX_EXTENSION_NAME_SIZE);
        pProperties[copyCount + 1].specVersion = 1;
        *pPropertyCount = copyCount + 2;
    }

    return VK_SUCCESS;
}

// Proxy das funções base exportadas que o emulador (Vulkan Loader) vai procurar:
extern "C" __attribute__((visibility("default")))
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char* pName) {
    if (strcmp(pName, "vkEnumerateDeviceExtensionProperties") == 0) {
        return (PFN_vkVoidFunction)my_vkEnumerateDeviceExtensionProperties;
    }

    if (!sys_vulkan_handle) sys_vulkan_handle = dlopen("libvulkan.so", RTLD_NOW);
    if (!sys_vulkan_handle) {
        LOGI("Falha ao abrir libvulkan.so do sistema!");
        return nullptr;
    }

    auto real_get = (PFN_vkGetInstanceProcAddr)dlsym(sys_vulkan_handle, "vkGetInstanceProcAddr");
    if (!real_get) {
        real_get = (PFN_vkGetInstanceProcAddr)dlsym(sys_vulkan_handle, "vk_icdGetInstanceProcAddr");
    }

    if (real_get) {
        return real_get(instance, pName);
    }
    
    return nullptr;
}

extern "C" __attribute__((visibility("default")))
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return vk_icdGetInstanceProcAddr(instance, pName);
}

extern "C" __attribute__((visibility("default")))
VKAPI_ATTR VkResult VKAPI_CALL vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion) {
    *pSupportedVersion = 3; // Versão de interface comum do Vulkan Loader
    return VK_SUCCESS;
}

void setup_vulkan_hooks() {
    LOGI("Custom Driver Init: Vulkan Proxy configurado com sucesso.");
}
