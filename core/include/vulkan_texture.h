#pragma once

#include <string>

#include <vulkan/vulkan.h>


namespace VK {
    class VulkanCore;

    class VulkanTexture {
    public:
        VulkanTexture() = default;

        explicit VulkanTexture(VulkanCore *pVulkanCore) { m_pVulkanCore = pVulkanCore; }

        void Init(VulkanCore* pVulkanCore) { m_pVulkanCore = pVulkanCore; }

        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_mem = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;

        void Destroy(VkDevice Device);

        void LoadFromDisk(const std::string &Filename);

        void Load(const std::string &Filename, int requiredChannels = 4);

        void Load(unsigned int BufferSize, void *pImageData);
        void LoadEctCubemap(const std::string& Filename, bool IsRGB);
    private:
        VulkanCore *m_pVulkanCore = nullptr;

        int m_imageWidth = 0;
        int m_imageHeight = 0;
        int m_imageBPP = 0;
    };
}
