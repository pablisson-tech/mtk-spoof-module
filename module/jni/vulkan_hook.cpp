#include "vulkan_hook.h"
#include <vulkan/vulkan.h>
#include <string.h>
#include <vector>
#include <dlfcn.h>
#include "And64InlineHook.hpp"

static VkResult (*orig_vkEnumerateDeviceExtensionProperties)(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties);

VkResult my_vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties) 
{
    uint32_t realCount = 0;
    orig_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, nullptr);
    
    std::vector<VkExtensionProperties> realProps(realCount);
    orig_vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, &realCount, realProps.data());

    if (pProperties == nullptr) {
        *pPropertyCount = realCount + 2; // Adding 2 fake QCOM extensions
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

void setup_vulkan_hooks() {
    void *vulkan_lib = dlopen("libvulkan.so", RTLD_NOW);
    if (vulkan_lib) {
        void *target_func = dlsym(vulkan_lib, "vkEnumerateDeviceExtensionProperties");
        if (target_func) {
            A64HookFunction(target_func, (void *)my_vkEnumerateDeviceExtensionProperties, (void **)&orig_vkEnumerateDeviceExtensionProperties);
        }
    }
}
