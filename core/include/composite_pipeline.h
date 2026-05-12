#pragma once

#include "types.h"
#include <vector>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include "vulkan-core.h"
#include "vulkan_pipeline_base.h"

namespace VK {
    class VulkanCore;

    enum CompositeBinding {
        CompBindingDenoisedLight = 0,
        CompBindingAlbedo = 1,
        CompBindingDenoisedSpecular = 2,
        CompBindingMetallic = 3,
        CompBindingGNormal = 4,
        CompBindingGPos = 5,
        CompBindingCount
    };

    struct CompositePushConstants {
        alignas(16) glm::vec3 cameraPos;
        alignas(16) glm::vec3 lightDir;
    };

    struct CompositeUBO {
        glm::vec3 lightDir;
        float padding1;
        glm::vec3 lightColor;
        float padding2;
        glm::vec3 viewPos;
        float padding3;
    };

    class CompositePipeline : public VulkanPipelineBase {
    public:
        CompositePipeline(VulkanCore *core, VkRenderPass renderPass, VkFormat colorFormat);

        ~CompositePipeline() override;

        void Init(VkShaderModule vs, VkShaderModule fs);

        void Destroy() override;

        void UpdateDescriptorSets(
            const std::vector<VkImageView> &denoisedDiffuseViews,
            const std::vector<VkImageView> &denoisedSpecularViews,
            const std::vector<VkImageView> &albedoViews,
            const std::vector<VkImageView> &specularMaterialViews,
            const std::vector<VkImageView> &normalViews,
            const std::vector<VkImageView> &posViews,
            VkSampler sampler
        );

        void UpdateUBO(int imageIndex, const CompositeUBO &uboData);

        void RecordCommandBuffer(struct ::VkCommandBuffer_T *cmd, int imageIndex, unsigned width, unsigned height,
                                 const glm::vec3 &cameraPos, const glm::vec3 &lightDir) const;

    private:
        VkSampler m_gBufferSampler = nullptr;

        std::vector<VkDescriptorSet> m_descriptorSets;
        std::vector<BufferAndMemory> m_ubos;
    };
}
