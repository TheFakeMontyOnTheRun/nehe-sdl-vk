#include <vector>

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

#ifdef __APPLE__
#define VK_INST_EXT_PORTABILITY "VK_KHR_portability_enumeration"
#endif

int main(int argc, char **argv) {
    bool running;
    SDL_Window *window;
    SDL_Event event;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;
    VkImageView imageView;
    VkFramebuffer framebuffer;

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
#ifdef __APPLE__
        extensionsForSDL.push_back(VK_INST_EXT_PORTABILITY);
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

#ifdef __APPLE__
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            goto cleanup_SDL_window;
        }
    }

    running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }

cleanup_VK_instance:
    vkDestroyInstance(instance, nullptr);

cleanup_SDL_window:
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
