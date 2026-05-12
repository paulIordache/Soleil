#ifndef IP_VULKAN_DEVICE_H
#define IP_VULKAN_DEVICE_H
#include <vector>
#include <vulkan/vulkan_core.h>

#include "types.h"

namespace VK {
    struct PhysicalDevice {
        VkPhysicalDevice m_physicalDevice;
        VkPhysicalDeviceProperties m_devProps;
        my_types::vec<VkQueueFamilyProperties> m_qFamilyProps;
        my_types::vec<VkBool32> m_qSupportsPresent;
        my_types::vec<VkSurfaceFormatKHR> m_surfaceFormats;
        VkSurfaceCapabilitiesKHR m_surfaceCaps;
        VkPhysicalDeviceMemoryProperties m_memProps;
        my_types::vec<VkPresentModeKHR> m_presentModes;
        VkPhysicalDeviceFeatures m_features;
        VkFormat m_depthFormat;
        struct {
            int Variant = 0;
            int Major = 0;
            int Minor = 0;
            int Patch = 0;
        } m_apiVersion;
        std::vector<VkExtensionProperties> m_extensions;

        bool IsExtensionSupported(const char *pExt) const;
    };

    class VulkanPhysicalDevice {
    public:
        VulkanPhysicalDevice() = default;
        ~VulkanPhysicalDevice() = default;

        void GetExtensions(int DeviceIndex);

        void init(const VkInstance &instance, const VkSurfaceKHR &surface);

        u32 selectDevice(VkQueueFlags requiredQueueType, bool supportPresent);

        const PhysicalDevice &selected() const;

    private:
        my_types::vec<PhysicalDevice> m_devices;

        i32 m_devIndex = -1;
    };
}

#endif //IP_VULKAN_DEVICE_H
