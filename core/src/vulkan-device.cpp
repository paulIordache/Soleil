#include <cassert>

#include "vulkan-device.h"
#include "vulkan-util.h"
#include "windows-macros.h"

namespace VK {
    static void PrintImageUsageFlags(const VkImageUsageFlags &flags) {
        if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
            printf("Image usage transfer src is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
            printf("Image usage transfer dest is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
            printf("Image usage sampled is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            printf("Image usage color attac hment is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            printf("Image usage depth stencil attachment is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
            printf("Image usage transient attachment is supported\n");
        }

        if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
            printf("Image usage input attachment is supported\n");
        }
    }


    static void PrintMemoryProperty(const VkMemoryPropertyFlags propertyFlags) {
        if (propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            printf("DEVICE LOCAL ");
        }

        if (propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            printf("HOST VISIBLE ");
        }

        if (propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            printf("HOST COHERENT ");
        }

        if (propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
            printf("HOST CACHED ");
        }

        if (propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
            printf("LAZILY ALLOCATED ");
        }

        if (propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
            printf("PROTECTED ");
        }
    }

    static VkFormat FindDepthFormat(VkPhysicalDevice device) {
        const std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        const VkFormat depthFormat = FindSupportedFormat(device, candidates, VK_IMAGE_TILING_OPTIMAL,
                                                         VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        return depthFormat;
    }

    bool PhysicalDevice::IsExtensionSupported(const char *pExt) const {
        bool ret = false;

        const std::string RequestExt(pExt);
        for (const VkExtensionProperties &e: m_extensions) {
            if (std::string CurExt(e.extensionName);
                CurExt == RequestExt) {
                ret = true;
                break;
            }
        }

        printf("Extension %s %s is supported\n", pExt, ret ? "IS" : "is NOT");
        return ret;
    }

    void VulkanPhysicalDevice::GetExtensions(int DeviceIndex) {
        PhysicalDevice &Dev = m_devices[DeviceIndex];

        u32 ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(Dev.m_physicalDevice, nullptr, &ExtensionCount, nullptr);

        Dev.m_extensions.resize(ExtensionCount);

        vkEnumerateDeviceExtensionProperties(Dev.m_physicalDevice, nullptr, &ExtensionCount, Dev.m_extensions.data());

        printf("Physical device extensions: \n");
        for (const VkExtensionProperties &e: Dev.m_extensions) {
            printf("    %s\n", e.extensionName);
        }
    }

    void VulkanPhysicalDevice::init(const VkInstance &instance, const VkSurfaceKHR &surface) {
        u32 numDevices = 0;

        VkResult res = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
        CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices error (1)\n");

        printf("Num physical devices %d\n\n", numDevices);

        m_devices.resize(numDevices);

        my_types::vec<VkPhysicalDevice> devices;
        devices.resize(numDevices);

        res = vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
        CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices error (2)\n");

        for (u32 i = 0; i < numDevices; i++) {
            const VkPhysicalDevice physDev = devices[i];
            m_devices[i].m_physicalDevice = physDev;

            vkGetPhysicalDeviceProperties(physDev, &m_devices[i].m_devProps);

            printf("Device name: %s\n", m_devices[i].m_devProps.deviceName);

            // GetDeviceApiVersion(i);

            GetExtensions(i);

            u32 NumQFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physDev, &NumQFamilies, nullptr);
            printf("    Num of family queues: %d\n", NumQFamilies);

            m_devices[i].m_qFamilyProps.resize(NumQFamilies);
            m_devices[i].m_qSupportsPresent.resize(NumQFamilies);

            vkGetPhysicalDeviceQueueFamilyProperties(physDev, &NumQFamilies, m_devices[i].m_qFamilyProps.data());

            for (u32 q = 0; q < NumQFamilies; q++) {
                const VkQueueFamilyProperties &QFamilyProp = m_devices[i].m_qFamilyProps[q];

                printf("    Family %d Num queues: %d ", q, QFamilyProp.queueCount);
                const VkQueueFlags flags = QFamilyProp.queueFlags;
                printf("    GFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
                       flags & VK_QUEUE_GRAPHICS_BIT ? "Yes" : "No",
                       flags & VK_QUEUE_COMPUTE_BIT ? "Yes" : "No",
                       flags & VK_QUEUE_TRANSFER_BIT ? "Yes" : "No",
                       flags & VK_QUEUE_SPARSE_BINDING_BIT ? "Yes" : "No");

                res = vkGetPhysicalDeviceSurfaceSupportKHR(physDev, q, surface, &m_devices[i].m_qSupportsPresent[q]);
                CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error\n");
            }

            u32 NumFormats = 0;
            res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &NumFormats, nullptr);
            CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR (1)\n");
            assert(NumFormats > 0);

            m_devices[i].m_surfaceFormats.resize(NumFormats);

            res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &NumFormats,
                                                       m_devices[i].m_surfaceFormats.data());
            CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR (2)\n");

            for (u32 j = 0; j < NumFormats; j++) {
                const auto &[format, colorSpace] = m_devices[i].m_surfaceFormats[j];
                printf("    Format %x color space %x\n", format, colorSpace);
            }

            res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &m_devices[i].m_surfaceCaps);
            CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR\n");

            PrintImageUsageFlags(m_devices[i].m_surfaceCaps.supportedUsageFlags);

            u32 NumPresentModes = 0;

            res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &NumPresentModes, nullptr);
            CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR (1) error\n");

            assert(NumPresentModes != 0);

            m_devices[i].m_presentModes.resize(NumPresentModes);

            res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &NumPresentModes,
                                                            m_devices[i].m_presentModes.data());
            CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR (2) error\n");

            printf("Number of presentation modes %d\n", NumPresentModes);

            vkGetPhysicalDeviceMemoryProperties(physDev, &m_devices[i].m_memProps);

            printf("Num memory types %d\n", m_devices[i].m_memProps.memoryTypeCount);
            for (u32 j = 0; j < m_devices[i].m_memProps.memoryTypeCount; j++) {
                printf("%d: flags %x heap %d ", j,
                       m_devices[i].m_memProps.memoryTypes[j].propertyFlags,
                       m_devices[i].m_memProps.memoryTypes[j].heapIndex);

                PrintMemoryProperty(m_devices[i].m_memProps.memoryTypes[j].propertyFlags);

                printf("\n");
            }
            printf("Num heap types %d\n", m_devices[i].m_memProps.memoryHeapCount);
            printf("\n");

            vkGetPhysicalDeviceFeatures(m_devices[i].m_physicalDevice, &m_devices[i].m_features);

            m_devices[i].m_depthFormat = FindDepthFormat(physDev);
        }
    }

    u32 VulkanPhysicalDevice::selectDevice(const VkQueueFlags requiredQueueType, const bool supportPresent) {
        for (i32 i = 0; i < m_devices.size(); i++) {
            for (i32 j = 0; j < m_devices[i].m_qFamilyProps.size(); j++) {
                if (const VkQueueFamilyProperties &QFamilyProp = m_devices[i].m_qFamilyProps[j];
                    QFamilyProp.queueFlags & requiredQueueType && static_cast<bool>(m_devices[i].m_qSupportsPresent[j])
                    == supportPresent) {
                    m_devIndex = i;
                    const i32 queueFamily = j;
                    printf("Using GFX device %d and queue family %d\n", m_devIndex, queueFamily);
                    return queueFamily;
                }
            }
        }

        VK_ENGINE_ERROR("Required queue type %x and supports present %d not found\n", requiredQueueType,
                        supportPresent);

        return 0;
    }

    const PhysicalDevice &VulkanPhysicalDevice::selected() const {
        if (m_devices.size() < 0) {
            VK_ENGINE_ERROR("A physical device has not been selected\n");
        }

        return m_devices[m_devIndex];
    }
}
