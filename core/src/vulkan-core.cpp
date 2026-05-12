#include <vector>
#include <cassert>

#define STBI_FAILURE_USERMSG
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "types.h"
#include "vulkan-core.h"

#include "bitmap.h"
#include "ect_cubemap.h"
#include "gl_basic_mesh_entry.h"
#include "vulkan-util.h"
#include "vulkan-wrapper.h"
#include "windows-macros.h"
#include "cpu_model.h"

static PFN_vkCmdBeginDebugUtilsLabelEXT g_pfnBeginLabel = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT g_pfnEndLabel = nullptr;

namespace VK {
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
        VkDebugUtilsMessageTypeFlagsEXT Type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        printf("Debug callback: %s\n", pCallbackData->pMessage);
        printf("  Severity %s\n", GetDebugSeverityStr(Severity));
        printf("  Type %s\n", GetDebugType(Type));
        printf("  Objects ");

        for (u32 i = 0; i < pCallbackData->objectCount; i++) {
#ifdef _WIN32
            printf("%llux ", pCallbackData->pObjects[i].objectHandle);
#else
            printf("%lux ", pCallbackData->pObjects[i].objectHandle);
#endif
        }

        printf("\n");

        return VK_FALSE; // The calling function should not be aborted
    }


    VulkanCore::VulkanCore() : m_queue() {
    }


    VulkanCore::~VulkanCore() {
        printf("-------------------------------\n");

        vkFreeCommandBuffers(m_device, m_cmdBufPool, 1, &m_copyCmdBuf);

        vkDestroyCommandPool(m_device, m_cmdBufPool, nullptr);

        m_queue.Destroy();

        for (auto &m_imageView: m_imageViews) {
            vkDestroyImageView(m_device, m_imageView, nullptr);
        }

        if (m_depthEnabled) {
            for (auto &m_depthImage: m_depthImages) {
                m_depthImage.Destroy(m_device);
            }
        }

        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);

        vkDestroyDevice(m_device, nullptr);

        PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
        vkDestroySurface = (PFN_vkDestroySurfaceKHR) vkGetInstanceProcAddr(m_instance, "vkDestroySurfaceKHR");
        if (!vkDestroySurface) {
            VK_ENGINE_ERROR0("Cannot find address of vkDestroySurfaceKHR\n");
            exit(1);
        }

        vkDestroySurface(m_instance, m_surface, nullptr);

        printf("GLFW window surface destroyed\n");

        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
        vkDestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (!vkDestroyDebugUtilsMessenger) {
            VK_ENGINE_ERROR0("Cannot find address of vkDestroyDebugUtilsMessengerEXT\n");
            exit(1);
        }
        vkDestroyDebugUtilsMessenger(m_instance, m_debugMessenger, nullptr);

        printf("Debug callback destroyed\n");

        vkDestroyInstance(m_instance, nullptr);
        printf("Vulkan instance destroyed\n");

        if (m_pWindow) {
            glfwDestroyWindow(m_pWindow);
            glfwTerminate();
            printf("GLFW terminated\n");
        }
    }


    void VulkanCore::Init(const char *pAppName, GLFWwindow *pWindow, bool DepthEnabled) {
        m_pWindow = pWindow;
        m_depthEnabled = DepthEnabled;
        GetFramebufferSize(m_windowWidth, m_windowHeight);
        CreateInstance(pAppName);
        CreateDebugCallback();
        if (!pWindow) {
            printf("You are probably in one of the initial tutorials so we can end the Init function here.\n");
            return;
        }
        CreateSurface();
        m_physDevices.init(m_instance, m_surface);
        m_queueFamily = m_physDevices.selectDevice(VK_QUEUE_GRAPHICS_BIT, true);
        CreateDevice();
        CreateSwapChain();
        CreateCommandBufferPool();
        m_queue.Init(m_device, m_swapChain, m_queueFamily, 0);
        CreateCommandBuffers(1, &m_copyCmdBuf);
        if (DepthEnabled) {
            CreateDepthResources();
        }
    }

    const VkImage &VulkanCore::GetImage(int Index) const {
        if (Index >= m_images.size()) {
            VK_ENGINE_ERROR("Invalid image index %d\n", Index);
            exit(1);
        }

        return m_images[Index];
    }

    const VkImageView &VulkanCore::GetImageView(int Index) const {
        if (Index >= m_imageViews.size()) {
            VK_ENGINE_ERROR("Invalid image view index %d\n", Index);
            exit(1);
        }

        return m_imageViews[Index];
    }

    const VkImageView &VulkanCore::GetDepthView(int Index) const {
        if (Index >= m_depthImages.size()) {
            VK_ENGINE_ERROR("Invalid depth view index %d\n", Index);
            exit(1);
        }

        return m_depthImages[Index].m_view;
    }

    const VkPhysicalDeviceLimits &VulkanCore::GetPhysicalDeviceLimits() const {
        return m_physDevices.selected().m_devProps.limits;
    }

    u32 VulkanCore::GetInstanceVersion() const {
        const u32 Version = VK_MAKE_API_VERSION(0,
                                                m_instanceVersion.Major,
                                                m_instanceVersion.Minor,
                                                m_instanceVersion.Patch);

        return Version;
    }

    void VulkanCore::CreateInstance(const char *pAppName) {
        UpdateInstanceVersion();

        std::vector Layers = {
            "VK_LAYER_KHRONOS_validation"
        };

        std::vector Extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined (_WIN32)
            "VK_KHR_win32_surface",
#endif
#if defined (__APPLE__)
            "VK_MVK_macos_surface",
#endif
#if defined (__linux__)
            "VK_KHR_xcb_surface",
#endif
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        };

        VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &DebugCallback,
            .pUserData = nullptr
        };

        VkApplicationInfo AppInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = pAppName,
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "Vulkan Engine",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .apiVersion = VK_MAKE_API_VERSION(0, m_instanceVersion.Major, m_instanceVersion.Minor, 0)
        };

        VkInstanceCreateInfo CreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = &MessengerCreateInfo,
            .flags = 0, // Reserved for future use. Must be zero
            .pApplicationInfo = &AppInfo,
            .enabledLayerCount = (u32) (Layers.size()),
            .ppEnabledLayerNames = Layers.data(),
            .enabledExtensionCount = (u32) (Extensions.size()),
            .ppEnabledExtensionNames = Extensions.data()
        };

        VkResult res = vkCreateInstance(&CreateInfo, nullptr, &m_instance);
        CHECK_VK_RESULT(res, "Create instance");
        printf("Vulkan instance created\n");
    }


    void VulkanCore::CreateDebugCallback() {
        VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = &DebugCallback,
            .pUserData = nullptr
        };

        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
        vkCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            m_instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!vkCreateDebugUtilsMessenger) {
            VK_ENGINE_ERROR0("Cannot find address of vkCreateDebugUtilsMessenger\n");
            exit(1);
        }

        VkResult res = vkCreateDebugUtilsMessenger(m_instance, &MessengerCreateInfo, nullptr, &m_debugMessenger);
        CHECK_VK_RESULT(res, "debug utils messenger");

        g_pfnBeginLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(
            m_instance, "vkCmdBeginDebugUtilsLabelEXT"));
        g_pfnEndLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(
            m_instance, "vkCmdEndDebugUtilsLabelEXT"));

        printf("Debug utils messenger created\n");
    }

    void VulkanCore::CmdBeginLabel(VkCommandBuffer cmd, const char *name, float r, float g, float b) {
        if (g_pfnBeginLabel) {
            VkDebugUtilsLabelEXT labelInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
            labelInfo.pLabelName = name;
            labelInfo.color[0] = r;
            labelInfo.color[1] = g;
            labelInfo.color[2] = b;
            labelInfo.color[3] = 1.0f;
            g_pfnBeginLabel(cmd, &labelInfo);
        }
    }

    void VulkanCore::CmdEndLabel(VkCommandBuffer cmd) {
        if (g_pfnEndLabel) g_pfnEndLabel(cmd);
    }


    void VulkanCore::CreateSurface() {
        VkResult res = glfwCreateWindowSurface(m_instance, m_pWindow, nullptr, &m_surface);
        CHECK_VK_RESULT(res, "glfwCreateWindowSurface");

        printf("GLFW window surface created\n");
    }


    void VulkanCore::UpdateInstanceVersion() {
        u32 InstanceVersion = 0;

        VkResult res = vkEnumerateInstanceVersion(&InstanceVersion);
        CHECK_VK_RESULT(res, "vkEnumerateInstanceVersion");

        m_instanceVersion.Major = VK_API_VERSION_MAJOR(InstanceVersion);
        m_instanceVersion.Minor = VK_API_VERSION_MINOR(InstanceVersion);
        m_instanceVersion.Patch = VK_API_VERSION_PATCH(InstanceVersion);

        printf("Vulkan loader supports version %d.%d.%d\n",
               m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch);
    }

    VkDeviceAddress VulkanCore::GetBufferDeviceAddress(VkBuffer buffer) {
        VkBufferDeviceAddressInfo info{
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO
        };
        info.buffer = buffer;
        return vkGetBufferDeviceAddress(m_device, &info);
    }


    void VulkanCore::CreateDevice() {
        float qPriorities[] = {1.0f};

        VkDeviceQueueCreateInfo qInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
        };
        qInfo.queueFamilyIndex = m_queueFamily;
        qInfo.queueCount = 1;
        qInfo.pQueuePriorities = &qPriorities[0];

        // --- Device extensions ---
        std::vector<const char *> DevExts = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
        };

        bool DeviceSupportsDynamicRendering =
                m_physDevices.selected().IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        bool Instance_is_1_3_or_higher =
                (m_instanceVersion.Major > 1) || (m_instanceVersion.Minor >= 3);

        if (Instance_is_1_3_or_higher && DeviceSupportsDynamicRendering) {
            printf("Vulkan 1.3+ instance and device support dynamic rendering\n");
        } else if (m_instanceVersion.Minor == 2) {
            if (DeviceSupportsDynamicRendering) {
                printf("Vulkan 1.2 with dynamic rendering extension enabled\n");
            } else {
                printf("The system does not support dynamic rendering\n");
                exit(1);
            }
        } else {
            printf("The system does not support dynamic rendering\n");
            exit(1);
        }

        // --- Core features you use ---
        VkPhysicalDeviceFeatures DeviceFeatures{};
        DeviceFeatures.geometryShader = VK_TRUE;
        DeviceFeatures.tessellationShader = VK_TRUE;
        DeviceFeatures.shaderInt64 = VK_TRUE; // <-- ADD THIS LINE
        DeviceFeatures.pipelineStatisticsQuery = VK_TRUE;

        // --- Feature chain (pNext) ---
        // Buffer device address
        VkPhysicalDeviceBufferDeviceAddressFeatures bda{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES
        };
        bda.bufferDeviceAddress = VK_TRUE;

        // Scalar block layout
        VkPhysicalDeviceScalarBlockLayoutFeatures scalarLayout{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES
        };
        scalarLayout.scalarBlockLayout = VK_TRUE;
        scalarLayout.pNext = &bda;

        // Acceleration structures
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR
        };
        accel.accelerationStructure = VK_TRUE;
        accel.pNext = &scalarLayout;

        // Ray tracing pipeline
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtp{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR
        };
        rtp.rayTracingPipeline = VK_TRUE;
        rtp.pNext = &accel;

        // Ray query
        VkPhysicalDeviceRayQueryFeaturesKHR rq{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR
        };
        rq.rayQuery = VK_TRUE;
        rq.pNext = &rtp;

        // Dynamic rendering at the head of the chain
        VkPhysicalDeviceDynamicRenderingFeaturesKHR dyn{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR
        };
        dyn.dynamicRendering = VK_TRUE;
        dyn.pNext = &rq;

        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
        };
        indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        indexingFeatures.runtimeDescriptorArray = VK_TRUE;
        indexingFeatures.pNext = &dyn;

        VkDeviceCreateInfo DeviceCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
        };
        DeviceCreateInfo.pNext = &indexingFeatures;
        DeviceCreateInfo.queueCreateInfoCount = 1;
        DeviceCreateInfo.pQueueCreateInfos = &qInfo;
        DeviceCreateInfo.enabledExtensionCount = (u32) DevExts.size();
        DeviceCreateInfo.ppEnabledExtensionNames = DevExts.data();
        DeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

        VkResult res = vkCreateDevice(m_physDevices.selected().m_physicalDevice, &DeviceCreateInfo, nullptr, &m_device);
        CHECK_VK_RESULT(res, "Create device\n");
        printf("\nDevice created\n");

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };
        VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        props2.pNext = &rtProps;
        vkGetPhysicalDeviceProperties2(m_physDevices.selected().m_physicalDevice, &props2);

        printf("Ray Tracing Pipeline Properties:\n");
        printf("  Shader Group Handle Size: %u bytes\n", rtProps.shaderGroupHandleSize);
        printf("  Max Ray Recursion Depth:  %u\n", rtProps.maxRayRecursionDepth);
        printf("  Max Ray Dispatch Invocations: %u\n", rtProps.maxRayDispatchInvocationCount);
        printf("  Max Ray Hit Attribute Size: %u bytes\n", rtProps.maxRayHitAttributeSize);

        // LoadFromDisk RT functions
        rtExtensions.vkCreateAccelerationStructureKHR =
                reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
                    vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
        rtExtensions.vkDestroyAccelerationStructureKHR =
                reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
                    vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
        rtExtensions.vkGetAccelerationStructureBuildSizesKHR =
                reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
                    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
        rtExtensions.vkGetAccelerationStructureDeviceAddressKHR =
                reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
                    vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        rtExtensions.vkCmdBuildAccelerationStructuresKHR =
                reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
                    vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
        rtExtensions.vkCreateRayTracingPipelinesKHR =
                reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
                    vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
        rtExtensions.vkGetRayTracingShaderGroupHandlesKHR =
                reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
                    vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        rtExtensions.vkCmdTraceRaysKHR =
                reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
                    vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));

        if (!rtExtensions.vkCreateAccelerationStructureKHR ||
            !rtExtensions.vkCmdBuildAccelerationStructuresKHR ||
            !rtExtensions.vkCreateRayTracingPipelinesKHR ||
            !rtExtensions.vkGetRayTracingShaderGroupHandlesKHR ||
            !rtExtensions.vkCmdTraceRaysKHR) {
            printf("Failed to load Vulkan ray tracing function pointers.\n");
            exit(1);
        }

        printf("Vulkan ray tracing functions loaded successfully.\n");
    }

    void VulkanCore::GetVRAMUsage(size_t& outUsage, size_t& outBudget) const {
        VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
        VkPhysicalDeviceMemoryProperties2 memoryProperties2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
        memoryProperties2.pNext = &memoryBudgetProperties;

        vkGetPhysicalDeviceMemoryProperties2(m_physDevices.selected().m_physicalDevice, &memoryProperties2);

        outUsage = 0;
        outBudget = 0;
        for (uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryHeapCount; i++) {
            if (memoryProperties2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                outUsage += memoryBudgetProperties.heapUsage[i];
                outBudget += memoryBudgetProperties.heapBudget[i];
            }
        }
    }


    static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &PresentModes) {
        for (const auto PresentMode: PresentModes) {
            if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return PresentMode;
            }
        }

        // Fallback to FIFO which is always supported
        return VK_PRESENT_MODE_FIFO_KHR;
    }


    static u32 ChooseNumImages(const VkSurfaceCapabilitiesKHR &Capabilities) {
        const u32 RequestedNumImages = Capabilities.minImageCount + 1;

        int FinalNumImages = 0;

        if (Capabilities.maxImageCount > 0 && RequestedNumImages > Capabilities.maxImageCount) {
            FinalNumImages = Capabilities.maxImageCount;
        } else {
            FinalNumImages = RequestedNumImages;
        }

        return FinalNumImages;
    }


    static VkSurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<VkSurfaceFormatKHR> &SurfaceFormats) {
        for (const auto SurfaceFormat: SurfaceFormats) {
            if ((SurfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) &&
                (SurfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
                return SurfaceFormat;
            }
        }

        return SurfaceFormats[0];
    }

    void VulkanCore::CreateSwapChain() {
        const VkSurfaceCapabilitiesKHR &SurfaceCaps = m_physDevices.selected().m_surfaceCaps;

        u32 NumImages = ChooseNumImages(SurfaceCaps);

        const std::vector<VkPresentModeKHR> &PresentModes = m_physDevices.selected().m_presentModes;
        const VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

        m_swapChainSurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_physDevices.selected().m_surfaceFormats);

        const VkSwapchainCreateInfoKHR SwapChainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = m_surface,
            .minImageCount = NumImages,
            .imageFormat = m_swapChainSurfaceFormat.format,
            .imageColorSpace = m_swapChainSurfaceFormat.colorSpace,
            .imageExtent = SurfaceCaps.currentExtent,
            .imageArrayLayers = 1,
            .imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &m_queueFamily,
            .preTransform = SurfaceCaps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = PresentMode,
            .clipped = VK_TRUE
        };

        VkResult res = vkCreateSwapchainKHR(m_device, &SwapChainCreateInfo, nullptr, &m_swapChain);
        CHECK_VK_RESULT(res, "vkCreateSwapchainKHR\n");

        printf("Swap chain created\n");

        u32 NumSwapChainImages = 0;
        res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, nullptr);
        CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");
        assert(NumImages <= NumSwapChainImages);

        printf("Requested %d images, created %d images\n", NumImages, NumSwapChainImages);

        m_images.resize(NumSwapChainImages);

        res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, m_images.data());
        CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");

        m_imageViews.resize(NumSwapChainImages);
        for (u32 i = 0; i < NumSwapChainImages; i++) {
            m_imageViews[i] = CreateImageView(m_device, m_images[i], m_swapChainSurfaceFormat.format,
                                              VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
        }
    }


    void VulkanCore::CreateCommandBufferPool() {
        VkCommandPoolCreateInfo cmdPoolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_queueFamily
        };

        VkResult res = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, NULL, &m_cmdBufPool);
        CHECK_VK_RESULT(res, "vkCreateCommandPool\n");

        printf("Command buffer pool created\n");
    }

    void VulkanCore::CreateCommandBuffers(u32 count, VkCommandBuffer *cmdBufs) {
        VkCommandBufferAllocateInfo cmdBufAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = m_cmdBufPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count
        };

        VkResult res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, cmdBufs);
        CHECK_VK_RESULT(res, "vkAllocateCommandBuffers\n");

        printf("%d command buffers created\n", count);
    }


    void VulkanCore::FreeCommandBuffers(u32 Count, const VkCommandBuffer *pCmdBufs) {
        m_queue.WaitIdle();
        vkFreeCommandBuffers(m_device, m_cmdBufPool, Count, pCmdBufs);
    }

    void VulkanCore::BeginDynamicRendering(VkCommandBuffer CmdBuf, int ImageIndex,
                                           VkClearValue *pClearColor, VkClearValue *pDepthValue) {
        VkRenderingAttachmentInfoKHR ColorAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .pNext = NULL,
            .imageView = GetImageView(ImageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = pClearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };

        if (pClearColor) {
            ColorAttachment.clearValue = *pClearColor;
        }

        VkRenderingAttachmentInfo DepthAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = NULL,
            .imageView = GetDepthView(ImageIndex),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = pDepthValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };

        if (pDepthValue) {
            DepthAttachment.clearValue = *pDepthValue;
        }

        VkRenderingInfoKHR RenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = {{0, 0}, {static_cast<u32>(m_windowWidth), static_cast<u32>(m_windowHeight)}},
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachment
        };

        vkCmdBeginRendering(CmdBuf, &RenderingInfo);
    }

    BufferAndMemory VulkanCore::UploadDataToGPU(const void *pVertices, size_t Size) {
        // Step 1: create the STAGING buffer (CPU-visible, src of copy)
        VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // <-- ONLY this
        VkMemoryPropertyFlags MemProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        BufferAndMemory StagingBuffer = CreateBuffer(Size, Usage, MemProps);

        // Step 2: map + upload
        void *pMem = nullptr;
        VkResult res = vkMapMemory(m_device, StagingBuffer.m_mem, 0, StagingBuffer.m_allocationSize, 0, &pMem);
        CHECK_VK_RESULT(res, "vkMapMemory\n");
        memcpy(pMem, pVertices, Size);
        vkUnmapMemory(m_device, StagingBuffer.m_mem);

        // Step 5: create the FINAL GPU buffer (device-local, dst of copy, RTX-ready)
        Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR; // <-- required for BLAS
        MemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        const BufferAndMemory GPUBuffer = CreateBuffer(Size, Usage, MemProps);

        // Step 6: copy staging -> final
        CopyBufferToBuffer(GPUBuffer.m_buffer, StagingBuffer.m_buffer, Size);

        // Step 7: destroy staging
        StagingBuffer.Destroy(m_device);

        return GPUBuffer;
    }


    BufferAndMemory VulkanCore::CreateUniformBuffer(size_t Size) {
        BufferAndMemory Buffer;

        VkBufferUsageFlags Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags MemProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        Buffer = CreateBuffer(Size, Usage, MemProps);

        return Buffer;
    }


    BufferAndMemory VulkanCore::CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage,
                                             VkMemoryPropertyFlags Properties) {
        VkBufferCreateInfo vbCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = Size,
            .usage = Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        BufferAndMemory Buf;

        // Step 1: create a buffer
        VkResult res = vkCreateBuffer(m_device, &vbCreateInfo, nullptr, &Buf.m_buffer);
        CHECK_VK_RESULT(res, "vkCreateBuffer\n");
        printf("Buffer created\n");

        // Step 2: get memory requirements
        VkMemoryRequirements MemReqs = {};
        vkGetBufferMemoryRequirements(m_device, Buf.m_buffer, &MemReqs);
        printf("Buffer requires %d bytes\n", (int) MemReqs.size);

        Buf.m_allocationSize = MemReqs.size;

        // Step 3: get memory type index
        u32 MemoryTypeIndex = GetMemoryTypeIndex(MemReqs.memoryTypeBits, Properties);
        printf("Memory type index %d\n", MemoryTypeIndex);

        // Step 4: allocate memory (with device address support if needed)
        VkMemoryAllocateFlagsInfo allocFlagsInfo{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO
        };
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo MemAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = MemReqs.size,
            .memoryTypeIndex = MemoryTypeIndex,
        };
        MemAllocInfo.pNext = nullptr;

        // Attach the flags struct *only if* the buffer supports device addressing
        if (Usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            MemAllocInfo.pNext = &allocFlagsInfo;
        }

        res = vkAllocateMemory(m_device, &MemAllocInfo, nullptr, &Buf.m_mem);
        CHECK_VK_RESULT(res, "vkAllocateMemory error %d\n");

        // Step 5: bind memory
        res = vkBindBufferMemory(m_device, Buf.m_buffer, Buf.m_mem, 0);
        CHECK_VK_RESULT(res, "vkBindBufferMemory error %d\n");

        return Buf;
    }

    void VulkanCore::CreateTexture(const char *pFilename, VulkanTexture &Tex) {
        int ImageWidth = 0;
        int ImageHeight = 0;
        int ImageChannels = 0;

        stbi_set_flip_vertically_on_load(1);

        stbi_uc *pPixels = stbi_load(pFilename, &ImageWidth, &ImageHeight, &ImageChannels, STBI_rgb_alpha);

        if (!pPixels) {
            printf("Error loading texture from '%s'\n", pFilename);
            exit(1);
        }

        VkFormat Format = VK_FORMAT_R8G8B8A8_UNORM;
        CreateTextureFromData(pPixels, ImageWidth, ImageHeight, Format, false, Tex);

        stbi_image_free(pPixels);

        printf("Texture from '%s' created\n", pFilename);
    }

    void VulkanCore::CreateImageFromData(VulkanTexture &tex, const void *p_pixels, int image_width,
                                         int image_height, VkFormat format, bool is_cubemap, uint32_t mipLevels) {
        auto Usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                       VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                       VK_IMAGE_USAGE_SAMPLED_BIT);
        VkMemoryPropertyFlagBits PropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        CreateImage(tex, image_width, image_height, format, Usage, PropertyFlags, is_cubemap, mipLevels);

        int LayerCount = is_cubemap ? 6 : 1;
        UpdateTextureImage(tex, image_width, image_height, format, LayerCount, p_pixels, is_cubemap, mipLevels);
    }

    void VulkanCore::CreateTextureDynamically(const char *pFilename, VulkanTexture &Tex, int requiredChannels) {
        int ImageWidth = 0;
        int ImageHeight = 0;
        int ImageChannels = 0;

        stbi_set_flip_vertically_on_load(1);

        stbi_uc *pPixels = stbi_load(pFilename, &ImageWidth, &ImageHeight, &ImageChannels, requiredChannels);

        if (!pPixels) {
            printf("Error loading texture from '%s'\n", pFilename);
            return;
        }

        VkFormat Format;
        if (requiredChannels == 1) {
            Format = VK_FORMAT_R8_UNORM;
        } else {
            Format = VK_FORMAT_R8G8B8A8_UNORM;
        }

        CreateTextureFromData(pPixels, ImageWidth, ImageHeight, Format, false, Tex);

        stbi_image_free(pPixels);

        printf("Texture from '%s' created (Channels: %d)\n", pFilename, requiredChannels);
    }

    void VulkanCore::Create2DTextureFromData(const void *pPixels, int ImageWidth, int ImageHeight, VulkanTexture &Tex) {
        VkFormat Format = VK_FORMAT_R8G8B8A8_SRGB;
        CreateTextureFromData(pPixels, ImageWidth, ImageHeight, Format, false, Tex);
    }

    void VulkanCore::CreateTextureFromData(const void *pPixels, int ImageWidth, int ImageHeight,
                                           VkFormat Format, bool IsCubemap, VulkanTexture &Tex) {
        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(ImageWidth, ImageHeight)))) + 1;

        CreateImageFromData(Tex, pPixels, ImageWidth, ImageHeight, Format, IsCubemap, mipLevels);

        VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        Tex.m_view = CreateImageView(m_device, Tex.m_image, Format, AspectFlags, IsCubemap, mipLevels);

        VkFilter MinFilter = VK_FILTER_LINEAR;
        VkFilter MaxFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        Tex.m_sampler = CreateTextureSampler(m_device, MinFilter, MaxFilter, AddressMode, mipLevels);

        printf("Texture from data created\n");
    }


    void VulkanTexture::Destroy(VkDevice Device) {
        vkDestroySampler(Device, m_sampler, NULL);
        vkDestroyImageView(Device, m_view, NULL);
        vkDestroyImage(Device, m_image, NULL);
        vkFreeMemory(Device, m_mem, NULL);
    }


    void VulkanCore::CreateCubemapTexture(const char *pFilename, VulkanTexture &Tex) {
        int Width, Height;

        const unsigned char *pImg = stbi_load(pFilename, &Width, &Height, NULL, STBI_rgb_alpha);

        if (!pImg) {
            printf("Error loading '%s'\n", pFilename);
            exit(1);
        }

        Bitmap Source(Width, Height, 4, eBitmapFormat_UnsignedByte, (void *) pImg);
        std::vector<Bitmap> Cubemap;
        int FaceSize = ConvertEquirectangularImageToCubemap(Source, Cubemap);

        stbi_image_free((void *) pImg);

        // Hack...
        int NumFaces = 6;
        VkFormat Format = VK_FORMAT_R8G8B8A8_SRGB;
        int BytesPerPixel = GetBytesPerTexFormat(Format);
        size_t SingleFaceNumBytes = FaceSize * FaceSize * BytesPerPixel;
        size_t TotalBytes = NumFaces * SingleFaceNumBytes;
        char *p = (char *) malloc(TotalBytes);

        for (int i = 0; i < NumFaces; i++) {
            memcpy(p + i * SingleFaceNumBytes, Cubemap[i].data_.data(), SingleFaceNumBytes);
        }

        CreateTextureFromData(p, FaceSize, FaceSize, Format, true, Tex);

        free(p);

        printf("Texture from '%s' created\n", pFilename);
    }

    void VulkanCore::CreateImage(VulkanTexture &Tex, u32 ImageWidth, u32 ImageHeight, VkFormat TexFormat,
                                 VkImageUsageFlags UsageFlags, VkMemoryPropertyFlagBits PropertyFlags, bool IsCubemap,
                                 uint32_t mipLevels) {
        VkImageCreateInfo ImageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = IsCubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : (VkImageCreateFlags) 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = TexFormat,
            .extent = VkExtent3D{.width = ImageWidth, .height = ImageHeight, .depth = 1},
            .mipLevels = mipLevels,
            .arrayLayers = IsCubemap ? 6u : 1u,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = UsageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkResult res = vkCreateImage(m_device, &ImageInfo, NULL, &Tex.m_image);
        CHECK_VK_RESULT(res, "vkCreateImage error");

        VkMemoryRequirements MemReqs = {0};
        vkGetImageMemoryRequirements(m_device, Tex.m_image, &MemReqs);
        printf("Image requires %d bytes\n", (int) MemReqs.size);

        u32 MemoryTypeIndex = GetMemoryTypeIndex(MemReqs.memoryTypeBits, PropertyFlags);
        printf("Memory type index %d\n", MemoryTypeIndex);

        VkMemoryAllocateInfo MemAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = MemReqs.size,
            .memoryTypeIndex = MemoryTypeIndex
        };

        res = vkAllocateMemory(m_device, &MemAllocInfo, NULL, &Tex.m_mem);
        CHECK_VK_RESULT(res, "vkAllocateMemory error");

        res = vkBindImageMemory(m_device, Tex.m_image, Tex.m_mem, 0);
        CHECK_VK_RESULT(res, "vkBindBufferMemory error %d\n");
    }

    void VulkanCore::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
                                     uint32_t mipLevels, int layerCount) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(m_physDevices.selected().m_physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            return;
        }

        VkCommandBuffer commandBuffer = CreateAndBeginSingleUseCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = layerCount;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = layerCount;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        EndSingleTimeCommands(commandBuffer);
    }

    void VulkanCore::UpdateTextureImage(VulkanTexture &Tex, u32 ImageWidth, u32 ImageHeight,
                                        VkFormat TexFormat, int LayerCount, const void *pPixels, bool IsCubemap,
                                        uint32_t mipLevels) {
        int BytesPerPixel = GetBytesPerTexFormat(TexFormat);

        VkDeviceSize LayerSize = ImageWidth * ImageHeight * BytesPerPixel;
        VkDeviceSize ImageSize = LayerCount * LayerSize;

        VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        BufferAndMemory StagingTex = CreateBuffer(ImageSize, Usage, Properties);

        StagingTex.Update(m_device, pPixels, ImageSize);

        TransitionImageLayout(Tex.m_image, TexFormat,
                              VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, LayerCount);

        CopyBufferToImage(Tex.m_image, StagingTex.m_buffer, ImageWidth, ImageHeight, LayerSize, LayerCount);

        GenerateMipmaps(Tex.m_image, TexFormat, ImageWidth, ImageHeight, mipLevels, LayerCount);

        StagingTex.Destroy(m_device);
    }

    void VulkanCore::TransitionImageLayout(VkImage &Image, VkFormat Format,
                                           VkImageLayout OldLayout, VkImageLayout NewLayout, int LayerCount) {
        BeginCommandBuffer(m_copyCmdBuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        ImageMemBarrier(m_copyCmdBuf, Image, Format, OldLayout, NewLayout, LayerCount);

        SubmitCopyCommand();
    }

    void VulkanCore::SubmitCopyCommand() {
        vkEndCommandBuffer(m_copyCmdBuf);

        m_queue.SubmitSync(m_copyCmdBuf);

        m_queue.WaitIdle();
    }

    void VulkanCore::CopyBufferToImage(VkImage Dst, VkBuffer Src,
                                       u32 ImageWidth, u32 ImageHeight,
                                       VkDeviceSize LayerSize, int LayerCount) {
        BeginCommandBuffer(m_copyCmdBuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        std::vector<VkBufferImageCopy> BufferImageCopy(LayerCount);

        for (int i = 0; i < LayerCount; i++) {
            VkBufferImageCopy bic = {
                .bufferOffset = i * LayerSize,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = VkImageSubresourceLayers{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = (u32) i,
                    .layerCount = 1
                },
                .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
                .imageExtent = VkExtent3D{.width = ImageWidth, .height = ImageHeight, .depth = 1}
            };

            BufferImageCopy[i] = bic;
        }

        vkCmdCopyBufferToImage(m_copyCmdBuf, Src, Dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               (u32) BufferImageCopy.size(), BufferImageCopy.data());
        SubmitCopyCommand();
    }


    void VulkanCore::CopyBufferToBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size) {
        BeginCommandBuffer(m_copyCmdBuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferCopy BufferCopy = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = Size
        };

        vkCmdCopyBuffer(m_copyCmdBuf, Src, Dst, 1, &BufferCopy);

        SubmitCopyCommand();
    }


    u32 VulkanCore::GetMemoryTypeIndex(u32 MemTypeBitsMask, VkMemoryPropertyFlags ReqMemPropFlags) {
        const VkPhysicalDeviceMemoryProperties &MemProps = m_physDevices.selected().m_memProps;

        for (u32 i = 0; i < MemProps.memoryTypeCount; i++) {
            const VkMemoryType &MemType = MemProps.memoryTypes[i];
            u32 CurBitmask = (1 << i);
            bool IsCurMemTypeSupported = (MemTypeBitsMask & CurBitmask);
            bool HasRequiredMemProps = ((MemType.propertyFlags & ReqMemPropFlags) == ReqMemPropFlags);

            if (IsCurMemTypeSupported && HasRequiredMemProps) {
                return i;
            }
        }

        printf("Cannot find memory type for type %x requested mem props %x\n", MemTypeBitsMask, ReqMemPropFlags);
        exit(1);
    }


    void BufferAndMemory::Destroy(VkDevice Device) {
        if (m_mem) {
            vkFreeMemory(Device, m_mem, nullptr);
        }
        if (m_buffer) {
            vkDestroyBuffer(Device, m_buffer, nullptr);
        }
    }


    void VulkanCore::GetFramebufferSize(int &Width, int &Height) const {
        glfwGetWindowSize(m_pWindow, &Width, &Height);
    }


    std::vector<BufferAndMemory> VulkanCore::CreateUniformBuffers(size_t Size) {
        std::vector<BufferAndMemory> UniformBuffers;

        UniformBuffers.resize(m_images.size());

        for (auto &UniformBuffer: UniformBuffers) {
            UniformBuffer = CreateUniformBuffer(Size);
        }

        return UniformBuffers;
    }

    void VulkanCore::CreateDepthResources() {
        int NumSwapChainImages = static_cast<int>(m_images.size());

        m_depthImages.resize(NumSwapChainImages);

        VkFormat DepthFormat = m_physDevices.selected().m_depthFormat;

        for (int i = 0; i < NumSwapChainImages; i++) {
            VkImageUsageFlagBits Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            VkMemoryPropertyFlagBits PropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            CreateImage(m_depthImages[i], m_windowWidth, m_windowHeight, DepthFormat,
                        Usage, PropertyFlags, false, 1);

            VkImageLayout OldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout NewLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            TransitionImageLayout(m_depthImages[i].m_image, DepthFormat, OldLayout, NewLayout, 1);

            m_depthImages[i].m_view = CreateImageView(m_device, m_depthImages[i].m_image,
                                                      DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, false, 1);
        }
    }

    VkCommandBuffer VulkanCore::CreateAndBeginSingleUseCommand() {
        VkCommandBuffer CmdBuf;

        CreateCommandBuffers(1, &CmdBuf);

        BeginCommandBuffer(CmdBuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        return CmdBuf;
    }


    void VulkanCore::EndSingleTimeCommands(VkCommandBuffer CmdBuf) {
        vkEndCommandBuffer(CmdBuf);

        m_queue.SubmitSync(CmdBuf);

        m_queue.WaitIdle();

        vkFreeCommandBuffers(m_device, m_cmdBufPool, 1, &CmdBuf);
    }


    void BufferAndMemory::Update(VkDevice Device, const void *pData, size_t Size) {
        void *pMem = nullptr;
        VkResult res = vkMapMemory(Device, m_mem, 0, Size, 0, &pMem);
        CHECK_VK_RESULT(res, "vkMapMemory");
        memcpy(pMem, pData, Size);
        vkUnmapMemory(Device, m_mem);
    }
}