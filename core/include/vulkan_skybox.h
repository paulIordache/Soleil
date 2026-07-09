

#ifndef VULKAN_SKYBOX_H
#define VULKAN_SKYBOX_H
#include <vulkan/vulkan_core.h>

#include "vulkan-core.h"
#include "vulkan_graphics_pipeline_v2.h"

namespace VK {
    class Skybox {
    public:
        Skybox() = default;

        ~Skybox() = default;

        void init(VulkanCore *pVulkanCore, const char *fileName);

        void destroy();

        void recordCommandBuffer(VkCommandBuffer cmdBuf, int imageIndex);

        void update(int ImageIndex, const glm::mat4& Transformation);

        VulkanTexture m_cubemapTex;

    private:
        void createDescriptorSets();

        VulkanCore *m_pVulkanCore = nullptr;
        int m_numImages = 0;
        std::vector<BufferAndMemory> m_uniformBuffers;
        std::vector<std::vector<VkDescriptorSet>> m_descriptorSets;
        VkShaderModule m_vs = VK_NULL_HANDLE;
        VkShaderModule m_fs = VK_NULL_HANDLE;
        GraphicsPipelineV2 *m_pPipeline = nullptr;
    };
}


#endif //VULKAN_SKYBOX_H
