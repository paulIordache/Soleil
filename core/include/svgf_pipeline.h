#pragma once
#include "vulkan-core.h"
#include "vulkan_pipeline_base.h"
#include <vector>

namespace VK {
    enum SVGFTemporalBinding {
        BindingTemporalInputDiffuse = 0,
        BindingTemporalInputSpecular = 1,
        BindingTemporalOutputDiffuse = 2,
        BindingTemporalOutputSpecular = 3,
        BindingTemporalMotion = 4,
        BindingTemporalNormal = 5,
        BindingTemporalDepth = 6,
        BindingTemporalPrevNormal = 7,
        BindingTemporalPrevDepth = 8,
        BindingTemporalHistoryDiffuse = 9,
        BindingTemporalHistorySpecular = 10,
        BindingTemporalPrevMomentsDiffuse = 11,
        BindingTemporalPrevMomentsSpecular = 12,
        BindingTemporalMomentsDiffuse = 13,
        BindingTemporalMomentsSpecular = 14,
        TemporalBindingCount
    };

    enum SVGFSpatialBinding {
        BindingSpatialInputDiffuse = 0,
        BindingSpatialInputSpecular = 1,
        BindingSpatialOutputDiffuse = 2,
        BindingSpatialOutputSpecular = 3,
        BindingSpatialNormal = 4,
        BindingSpatialDepth = 5,
        BindingSpatialMomentsDiffuse = 6,
        BindingSpatialMomentsSpecular = 7,
        BindingSpatialOutVarianceDiffuse = 8,
        BindingSpatialOutVarianceSpecular = 9,
        SpatialBindingCount
    };

    class SVGFPipeline : public VulkanPipelineBase {
    public:
        explicit SVGFPipeline(VulkanCore *core);

        ~SVGFPipeline() override;

        void Init(VkShaderModule temporalShader, VkShaderModule spatialShader);

        void Destroy() override;

        void CreateOutputImages(uint32_t width, uint32_t height, uint32_t numImages);

        void UpdateDescriptorSets(const std::vector<VkImageView> &inputDiffuseViews,
                                  const std::vector<VkImageView> &inputSpecularViews,
                                  const std::vector<VkImageView> &albedoViews,
                                  const std::vector<VkImageView> &normalViews,
                                  const std::vector<VkImageView> &motionViews,
                                  const std::vector<VkImageView> &specularMotionViews,
                                  const std::vector<VkImageView> &depthViews, VkSampler commonSampler);

        void DispatchTemporal(VkCommandBuffer cmd, uint32_t w, uint32_t h, uint32_t idx);

        void DispatchSpatial(VkCommandBuffer cmd, uint32_t w, uint32_t h,
                             uint32_t idx, int step, int iter);

        VulkanTexture &GetDenoisedDiffuse(uint32_t i) { return m_denoiseImages[i]; }
        VulkanTexture &GetDenoisedSpecular(uint32_t i) { return m_denoiseImagesSpec[i]; }
        VulkanTexture &GetIntermediateDiffuse(uint32_t i) { return m_intermediateImages[i]; }
        VulkanTexture &GetIntermediateSpecular(uint32_t i) { return m_intermediateImagesSpec[i]; }

        VulkanTexture &GetVarianceDiff(int slot, uint32_t i) { return m_varianceDiff[slot][i]; }
        VulkanTexture &GetVarianceSpec(int slot, uint32_t i) { return m_varianceSpec[slot][i]; }

        std::vector<VulkanTexture> m_denoiseImages;
        std::vector<VulkanTexture> m_denoiseImagesSpec;
        std::vector<VulkanTexture> m_intermediateImages;
        std::vector<VulkanTexture> m_intermediateImagesSpec;
        std::vector<VulkanTexture> m_momentsImages;
        std::vector<VulkanTexture> m_momentsImagesSpec;

    private:
        VkDescriptorSetLayout m_spatialSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_spatialPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipelineSpatial = VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> m_setsTemporal;
        std::vector<VkDescriptorSet> m_setsSpatial;

        std::vector<VulkanTexture> m_varianceDiff[2];
        std::vector<VulkanTexture> m_varianceSpec[2];
    };
}
