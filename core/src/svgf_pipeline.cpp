#include "svgf_pipeline.h"
#include <stdio.h>
#include "vulkan-wrapper.h"

namespace VK {
    SVGFPipeline::SVGFPipeline(VulkanCore *core) : VulkanPipelineBase(core->GetDevice(), core) {
    }

    SVGFPipeline::~SVGFPipeline() { Destroy(); }

    void SVGFPipeline::Destroy() {
        for (auto &tex: m_denoiseImages) tex.Destroy(m_device);
        for (auto &tex: m_denoiseImagesSpec) tex.Destroy(m_device);
        for (auto &tex: m_intermediateImages) tex.Destroy(m_device);
        for (auto &tex: m_intermediateImagesSpec) tex.Destroy(m_device);
        for (auto &tex: m_momentsImages) tex.Destroy(m_device);
        for (auto &tex: m_momentsImagesSpec) tex.Destroy(m_device);
        for (auto &tex: m_varianceDiff[0]) tex.Destroy(m_device);
        for (auto &tex: m_varianceDiff[1]) tex.Destroy(m_device);
        for (auto &tex: m_varianceSpec[0]) tex.Destroy(m_device);
        for (auto &tex: m_varianceSpec[1]) tex.Destroy(m_device);

        m_denoiseImages.clear();
        m_denoiseImagesSpec.clear();
        m_intermediateImages.clear();
        m_intermediateImagesSpec.clear();
        m_momentsImages.clear();
        m_momentsImagesSpec.clear();
        m_varianceDiff[0].clear();
        m_varianceDiff[1].clear();
        m_varianceSpec[0].clear();
        m_varianceSpec[1].clear();

        if (m_spatialSetLayout) vkDestroyDescriptorSetLayout(m_device, m_spatialSetLayout, nullptr);
        if (m_spatialPipelineLayout) vkDestroyPipelineLayout(m_device, m_spatialPipelineLayout, nullptr);
        if (m_pipelineSpatial) vkDestroyPipeline(m_device, m_pipelineSpatial, nullptr);

        m_spatialSetLayout = VK_NULL_HANDLE;
        m_spatialPipelineLayout = VK_NULL_HANDLE;
        m_pipelineSpatial = VK_NULL_HANDLE;

        DestroyBaseResources();
    }

    void SVGFPipeline::CreateOutputImages(uint32_t width, uint32_t height, uint32_t numImages) {
        for (auto &tex: m_denoiseImages) tex.Destroy(m_device);
        for (auto &tex: m_denoiseImagesSpec) tex.Destroy(m_device);
        for (auto &tex: m_intermediateImages) tex.Destroy(m_device);
        for (auto &tex: m_intermediateImagesSpec) tex.Destroy(m_device);
        for (auto &tex: m_momentsImages) tex.Destroy(m_device);
        for (auto &tex: m_momentsImagesSpec) tex.Destroy(m_device);

        m_denoiseImages.resize(numImages);
        m_denoiseImagesSpec.resize(numImages);
        m_intermediateImages.resize(numImages);
        m_intermediateImagesSpec.resize(numImages);
        m_momentsImages.resize(numImages);
        m_momentsImagesSpec.resize(numImages);

        VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        auto createImg = [&](VulkanTexture &tex, VkFormat fmt) {
            tex.Init(m_core);
            m_core->CreateImage(tex, width, height, fmt, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            tex.m_view = CreateImageView(m_device, tex.m_image, fmt, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);

            VkSamplerCreateInfo samplerInfo = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            };
            vkCreateSampler(m_device, &samplerInfo, nullptr, &tex.m_sampler);

            m_core->TransitionImageLayout(tex.m_image, fmt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
        };

        m_varianceDiff[0].resize(numImages);
        m_varianceDiff[1].resize(numImages);
        m_varianceSpec[0].resize(numImages);
        m_varianceSpec[1].resize(numImages);

        for (uint32_t i = 0; i < numImages; ++i) {
            createImg(m_denoiseImages[i], VK_FORMAT_R16G16B16A16_SFLOAT);
            createImg(m_intermediateImages[i], VK_FORMAT_R16G16B16A16_SFLOAT);
            createImg(m_momentsImages[i], VK_FORMAT_R16G16_SFLOAT);
            createImg(m_denoiseImagesSpec[i], VK_FORMAT_R16G16B16A16_SFLOAT);
            createImg(m_intermediateImagesSpec[i], VK_FORMAT_R16G16B16A16_SFLOAT);
            createImg(m_momentsImagesSpec[i], VK_FORMAT_R16G16_SFLOAT);

            createImg(m_varianceDiff[0][i], VK_FORMAT_R16G16_SFLOAT);
            createImg(m_varianceDiff[1][i], VK_FORMAT_R16G16_SFLOAT);
            createImg(m_varianceSpec[0][i], VK_FORMAT_R16G16_SFLOAT);
            createImg(m_varianceSpec[1][i], VK_FORMAT_R16G16_SFLOAT);
        }
    }

    void SVGFPipeline::Init(VkShaderModule temporalShader, VkShaderModule spatialShader) {
        std::vector<VkDescriptorSetLayoutBinding> tBindings(TemporalBindingCount);

        tBindings[BindingTemporalInputDiffuse] = {
            BindingTemporalInputDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        tBindings[BindingTemporalInputSpecular] = {
            BindingTemporalInputSpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        tBindings[BindingTemporalOutputDiffuse] = {
            BindingTemporalOutputDiffuse, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalOutputSpecular] = {
            BindingTemporalOutputSpecular, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalMotion] = {
            BindingTemporalMotion, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalNormal] = {
            BindingTemporalNormal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalDepth] = {
            BindingTemporalDepth, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalPrevNormal] = {
            BindingTemporalPrevNormal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        tBindings[BindingTemporalPrevDepth] = {
            BindingTemporalPrevDepth, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalHistoryDiffuse] = {
            BindingTemporalHistoryDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        tBindings[BindingTemporalHistorySpecular] = {
            BindingTemporalHistorySpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        tBindings[BindingTemporalPrevMomentsDiffuse] = {
            BindingTemporalPrevMomentsDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalPrevMomentsSpecular] = {
            BindingTemporalPrevMomentsSpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalMomentsDiffuse] = {
            BindingTemporalMomentsDiffuse, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        tBindings[BindingTemporalMomentsSpecular] = {
            BindingTemporalMomentsSpecular, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = TemporalBindingCount,
            .pBindings = tBindings.data()
        };
        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);

        VkPushConstantRange pushConstant = {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(int) * 2
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstant
        };
        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

        VkComputePipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = temporalShader,
                .pName = "main"
            },
            .layout = m_pipelineLayout
        };
        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipeline);

        std::vector<VkDescriptorSetLayoutBinding> sBindings(SpatialBindingCount);
        sBindings[BindingSpatialInputDiffuse] = {
            BindingSpatialInputDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        sBindings[BindingSpatialInputSpecular] = {
            BindingSpatialInputSpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        sBindings[BindingSpatialOutputDiffuse] = {
            BindingSpatialOutputDiffuse, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        sBindings[BindingSpatialOutputSpecular] = {
            BindingSpatialOutputSpecular, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        sBindings[BindingSpatialNormal] = {
            BindingSpatialNormal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        sBindings[BindingSpatialDepth] = {
            BindingSpatialDepth, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        sBindings[BindingSpatialMomentsDiffuse] = {
            BindingSpatialMomentsDiffuse, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        sBindings[BindingSpatialMomentsSpecular] = {
            BindingSpatialMomentsSpecular, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
            nullptr
        };
        sBindings[BindingSpatialOutVarianceDiffuse] = {
            BindingSpatialOutVarianceDiffuse, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };
        sBindings[BindingSpatialOutVarianceSpecular] = {
            BindingSpatialOutVarianceSpecular, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr
        };

        VkDescriptorSetLayoutCreateInfo spatialLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = SpatialBindingCount,
            .pBindings = sBindings.data()
        };
        vkCreateDescriptorSetLayout(m_device, &spatialLayoutInfo, nullptr, &m_spatialSetLayout);

        VkPipelineLayoutCreateInfo spPlInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_spatialSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstant
        };
        vkCreatePipelineLayout(m_device, &spPlInfo, nullptr, &m_spatialPipelineLayout);

        info.layout = m_spatialPipelineLayout;
        info.stage.module = spatialShader;
        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipelineSpatial);
    }

    void SVGFPipeline::UpdateDescriptorSets(const std::vector<VkImageView> &inputDiffuseViews,
                                            const std::vector<VkImageView> &inputSpecularViews,
                                            const std::vector<VkImageView> &albedoViews,
                                            const std::vector<VkImageView> &normalViews,
                                            const std::vector<VkImageView> &motionViews,
                                            const std::vector<VkImageView> &specularMotionViews,
                                            const std::vector<VkImageView> &depthViews,
                                            VkSampler commonSampler) {
        auto num = static_cast<uint32_t>(inputDiffuseViews.size());
        if (m_descriptorPool) vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

        uint32_t numSpatialSets = num * 5;
        VkDescriptorPoolSize sizes[] = {
            {
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                num * TemporalBindingCount + numSpatialSets * SpatialBindingCount
            },
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, num * TemporalBindingCount + numSpatialSets * SpatialBindingCount},
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = num + numSpatialSets,
            .poolSizeCount = 2,
            .pPoolSizes = sizes
        };
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

        std::vector<VkDescriptorSetLayout> tLayouts(num, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo tAlloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = num,
            .pSetLayouts = tLayouts.data()
        };
        m_setsTemporal.resize(num);
        vkAllocateDescriptorSets(m_device, &tAlloc, m_setsTemporal.data());

        std::vector<VkDescriptorSetLayout> sLayouts(numSpatialSets, m_spatialSetLayout);
        VkDescriptorSetAllocateInfo sAlloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = numSpatialSets,
            .pSetLayouts = sLayouts.data()
        };
        m_setsSpatial.resize(numSpatialSets);
        vkAllocateDescriptorSets(m_device, &sAlloc, m_setsSpatial.data());

        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(num * TemporalBindingCount + numSpatialSets * SpatialBindingCount);

        std::vector<VkDescriptorImageInfo> tInfos(num * TemporalBindingCount);
        std::vector<VkDescriptorImageInfo> sInfos(numSpatialSets * SpatialBindingCount);

        uint32_t tInfoIdx = 0;
        uint32_t sInfoIdx = 0;

        for (uint32_t i = 0; i < num; ++i) {
            uint32_t prev = (i == 0) ? num - 1 : i - 1;

            tInfos[tInfoIdx++] = {commonSampler, inputDiffuseViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalInputDiffuse, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, inputSpecularViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalInputSpecular, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {VK_NULL_HANDLE, m_intermediateImages[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalOutputDiffuse, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {VK_NULL_HANDLE, m_intermediateImagesSpec[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalOutputSpecular, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, motionViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalMotion, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, normalViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalNormal, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, depthViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalDepth, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, normalViews[prev], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalPrevNormal, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {commonSampler, depthViews[prev], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalPrevDepth, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {
                m_denoiseImages[prev].m_sampler, m_denoiseImages[prev].m_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalHistoryDiffuse, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {
                m_denoiseImagesSpec[prev].m_sampler, m_denoiseImagesSpec[prev].m_view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalHistorySpecular, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {
                m_momentsImages[prev].m_sampler, m_momentsImages[prev].m_view, VK_IMAGE_LAYOUT_GENERAL
            };
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalPrevMomentsDiffuse, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {
                m_momentsImagesSpec[prev].m_sampler, m_momentsImagesSpec[prev].m_view, VK_IMAGE_LAYOUT_GENERAL
            };
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalPrevMomentsSpecular, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {VK_NULL_HANDLE, m_momentsImages[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalMomentsDiffuse, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            tInfos[tInfoIdx++] = {VK_NULL_HANDLE, m_momentsImagesSpec[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_setsTemporal[i],
                .dstBinding = BindingTemporalMomentsSpecular, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &tInfos[tInfoIdx - 1]
            });

            for (int iter = 0; iter < 5; iter++) {
                bool ping = (iter % 2 == 0);
                VkImageView inDiffView = ping ? m_intermediateImages[i].m_view : m_denoiseImages[i].m_view;
                VkImageView inSpecView = ping ? m_intermediateImagesSpec[i].m_view : m_denoiseImagesSpec[i].m_view;
                VkImageView outDiffView = ping ? m_denoiseImages[i].m_view : m_intermediateImages[i].m_view;
                VkImageView outSpecView = ping ? m_denoiseImagesSpec[i].m_view : m_intermediateImagesSpec[i].m_view;

                int varReadSlot = (iter == 0) ? -1 : (iter - 1) % 2;
                int varWriteSlot = iter % 2;

                VkDescriptorSet ds = m_setsSpatial[i * 5 + iter];

                sInfos[sInfoIdx++] = {commonSampler, inDiffView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialInputDiffuse, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {commonSampler, inSpecView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialInputSpecular, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {VK_NULL_HANDLE, outDiffView, VK_IMAGE_LAYOUT_GENERAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialOutputDiffuse, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {VK_NULL_HANDLE, outSpecView, VK_IMAGE_LAYOUT_GENERAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialOutputSpecular, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {commonSampler, normalViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = BindingSpatialNormal,
                    .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {commonSampler, depthViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = BindingSpatialDepth,
                    .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                if (iter == 0) {
                    sInfos[sInfoIdx++] = {commonSampler, m_momentsImages[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
                    writes.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                        .dstBinding = BindingSpatialMomentsDiffuse, .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                    });
                    sInfos[sInfoIdx++] = {commonSampler, m_momentsImagesSpec[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
                    writes.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                        .dstBinding = BindingSpatialMomentsSpecular, .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                    });
                } else {
                    sInfos[sInfoIdx++] = {
                        commonSampler, m_varianceDiff[varReadSlot][i].m_view, VK_IMAGE_LAYOUT_GENERAL
                    };
                    writes.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                        .dstBinding = BindingSpatialMomentsDiffuse, .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                    });
                    sInfos[sInfoIdx++] = {
                        commonSampler, m_varianceSpec[varReadSlot][i].m_view, VK_IMAGE_LAYOUT_GENERAL
                    };
                    writes.push_back({
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                        .dstBinding = BindingSpatialMomentsSpecular, .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &sInfos[sInfoIdx - 1]
                    });
                }

                sInfos[sInfoIdx++] = {VK_NULL_HANDLE, m_varianceDiff[varWriteSlot][i].m_view, VK_IMAGE_LAYOUT_GENERAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialOutVarianceDiffuse, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &sInfos[sInfoIdx - 1]
                });

                sInfos[sInfoIdx++] = {VK_NULL_HANDLE, m_varianceSpec[varWriteSlot][i].m_view, VK_IMAGE_LAYOUT_GENERAL};
                writes.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds,
                    .dstBinding = BindingSpatialOutVarianceSpecular, .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &sInfos[sInfoIdx - 1]
                });
            }
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void SVGFPipeline::DispatchTemporal(VkCommandBuffer cmd, uint32_t w, uint32_t h, uint32_t idx) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_setsTemporal[idx], 0,
                                nullptr);
        vkCmdDispatch(cmd, (w + 15) / 16, (h + 15) / 16, 1);
    }

    void SVGFPipeline::DispatchSpatial(VkCommandBuffer cmd, uint32_t w, uint32_t h, uint32_t idx, int step, int iter) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineSpatial);
        VkDescriptorSet set = m_setsSpatial[idx * 5 + iter];
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_spatialPipelineLayout, 0, 1, &set, 0, nullptr);
        struct PushData {
            int step;
            int iter;
        } pcData = {step, iter};
        vkCmdPushConstants(cmd, m_spatialPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &pcData);
        vkCmdDispatch(cmd, (w + 15) / 16, (h + 15) / 16, 1);
    }
}
