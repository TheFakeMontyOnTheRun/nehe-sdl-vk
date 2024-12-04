#include <vector>
#include <iostream>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

#define VK_VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"

#ifdef __APPLE__
#define VK_INST_EXT_PORTABILITY "VK_KHR_portability_enumeration"
#define VK_INST_EXT_GET_PHYS_DEVICE "VK_KHR_get_physical_device_properties2"
#define VK_DEV_EXT_PORTABILITY "KHR_portability_subset"
#endif

VkDebugUtilsMessengerEXT debugMessenger;

const char *toStringMessageSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT s) {
    switch (s) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "VERBOSE";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "ERROR";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "INFO";
        default:
            return "UNKNOWN";
    }
}

const char *toStringMessageType(VkDebugUtilsMessageTypeFlagsEXT s) {
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "General | Validation | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "Validation | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "General | Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
        return "Performance";
    if (s == (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
        return "General | Validation";
    if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        return "Validation";
    if (s == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) return "General";
    return "Unknown";
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void * /* pUserData */) {
    auto ms = toStringMessageSeverity(messageSeverity);
    auto mt = toStringMessageType(messageType);
    printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

bool setupDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    memset(&createInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = debugCallback;

    return CreateDebugUtilsMessengerEXT(instance,
                                        &createInfo,
                                        nullptr,
                                        &debugMessenger) == VK_SUCCESS;
}

int main(int argc, char **argv) {
    bool running;
    SDL_Window *window;
    SDL_Event event;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = static_cast<VkPhysicalDevice>(VK_NULL_HANDLE);
    uint32_t queueFamilyIndex;
    VkDevice logicalDevice;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow(
            "SDL Vulkan Window",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800, 600,
            SDL_WINDOW_VULKAN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Creating the Vulkan instance
    {
        unsigned int extensionCount = 0;
        std::vector<const char *> extensionsForSDL;

        if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                              nullptr)) {
            goto cleanup_SDL_window;
        }

        extensionsForSDL.resize(extensionCount);

        if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                              extensionsForSDL.data())) {
            goto cleanup_SDL_window;
        }

        extensionsForSDL.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        extensionsForSDL.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef __APPLE__
        extensionsForSDL.push_back(VK_INST_EXT_PORTABILITY);
        extensionsForSDL.push_back(VK_INST_EXT_GET_PHYS_DEVICE);
#endif

        VkApplicationInfo appInfo;
        memset(&appInfo, 0, sizeof(VkApplicationInfo));
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "NeHe Vulkan Lesson 1";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = "Custom engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo;
        memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensionsForSDL.size();
        createInfo.ppEnabledExtensionNames = extensionsForSDL.data();

        char *validationLayerName = VK_VALIDATION_LAYER;

        createInfo.ppEnabledLayerNames = &validationLayerName;
        createInfo.enabledLayerCount = 1;

#ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            goto cleanup_SDL_window;
        }
    }

    if (!setupDebugMessenger(instance)) {
        goto cleanup_VK_validation_layer;
    }

/// new for lesson 2
    {
        uint32_t deviceCount = 0;
        VkResult result;

        result = vkEnumeratePhysicalDevices(instance,
                                            &deviceCount,
                                            nullptr);

        if (result != VK_SUCCESS || deviceCount == 0) {
            goto cleanup_VK_validation_layer;
        }

        std::vector<VkPhysicalDevice> availableDevices(deviceCount);

        result = vkEnumeratePhysicalDevices(instance,
                                            &deviceCount,
                                            availableDevices.data());

        if (result != VK_SUCCESS) {
            goto cleanup_VK_validation_layer;
        }

        for (int c = 0; c < deviceCount; ++c) {
            uint32_t queueFamilyCount = 0;

            vkGetPhysicalDeviceQueueFamilyProperties(availableDevices[c],
                                                     &queueFamilyCount,
                                                     nullptr);

            VkQueueFamilyProperties queueFamilies[queueFamilyCount];

            vkGetPhysicalDeviceQueueFamilyProperties(availableDevices[c],
                                                     &queueFamilyCount,
                                                     queueFamilies);

            for (uint32_t j = 0; j < queueFamilyCount; ++j) {
                if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    physicalDevice = availableDevices[c];
                    goto physical_device_selected;
                }
            }
        }
        goto cleanup_VK_validation_layer;
    }
physical_device_selected:
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &queueFamilyCount,
                                                 nullptr);

        if (queueFamilyCount == 0) {
            goto cleanup_VK_validation_layer;
        }

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                                 &queueFamilyCount,
                                                 queueFamilies.data());

        if (queueFamilies.empty()) {
            goto cleanup_VK_validation_layer;
        }

        for (int c = 0; c < queueFamilies.size(); ++c) {
            if (queueFamilies[c].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamilyIndex = c;
                goto queue_family_selected;
            }
        }
        goto cleanup_VK_validation_layer;
    }
queue_family_selected:
    {
        VkDeviceQueueCreateInfo queueCreateInfo;
        memset(&queueCreateInfo, 0, sizeof(queueCreateInfo));
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures;
        memset(&deviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

        VkDeviceCreateInfo createInfo;
        memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
#ifdef __APPLE__
        const char *portabilityExtensionName = "VK_KHR_portability_subset";
        createInfo.ppEnabledExtensionNames = &portabilityExtensionName;
        createInfo.enabledExtensionCount = 1;
#endif
        if (vkCreateDevice(physicalDevice,
                           &createInfo,
                           nullptr,
                           &logicalDevice) != VK_SUCCESS) {
            goto cleanup_VK_validation_layer;
        }
    }
////////////////////
    running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }
/// new for lesson 2
cleanup_VK_logical_device:
    vkDestroyDevice(logicalDevice, nullptr);
////////////////////

cleanup_VK_validation_layer:
    destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

cleanup_VK_instance:
    vkDestroyInstance(instance, nullptr);

cleanup_SDL_window:
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
