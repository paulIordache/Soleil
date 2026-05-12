#include "vulkan_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vulkan-core.h"

namespace VK {
    void VulkanTexture::LoadFromDisk(const std::string &Filename) {
        assert(m_pVulkanCore);
        m_pVulkanCore->CreateTexture(Filename.c_str(), *this);
    }

    void VulkanTexture::Load(const std::string &Filename, int requiredChannels) {
        assert(m_pVulkanCore);
        m_pVulkanCore->CreateTextureDynamically(Filename.c_str(), *this, requiredChannels);
    }

    void VulkanTexture::Load(unsigned int BufferSize, void *pData) {
        assert(m_pVulkanCore);
        int Width = 0;
        int Height = 0;
        int BPP = 0;
        void *pImageData = stbi_load_from_memory(static_cast<const stbi_uc *>(pData),
                                                 BufferSize, &Width, &Height, &BPP, 0);
        m_pVulkanCore->Create2DTextureFromData(pImageData, Width, Height, *this);
    }

    void VulkanTexture::LoadEctCubemap(const std::string &Filename, bool IsRGB) {
        assert(m_pVulkanCore);
        m_pVulkanCore->CreateCubemapTexture(Filename.c_str(), *this);
    }
}
