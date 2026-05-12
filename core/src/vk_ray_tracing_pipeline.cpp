#include "vk_ray_tracing_pipeline.h"
#include <array>
#include <cstring>
#include <cassert>
#include "vulkan-shader.h"

namespace VK {
    RayTracingPipeline::RayTracingPipeline(VulkanCore *core)
        : VulkanPipelineBase(core ? core->GetDevice() : VK_NULL_HANDLE, core) {
        assert(m_core);
    }

    RayTracingPipeline::~RayTracingPipeline() {
        Destroy();
    }

    void RayTracingPipeline::Init(VkAccelerationStructureKHR tlas, uint32_t width, uint32_t height, uint32_t numImages,
                                  VkShaderModule rgenModule, VkShaderModule rmissModule, VkShaderModule rchitModule,
                                  VkShaderModule reflMissModule, VkShaderModule reflHitModule) {
        m_tlas = tlas;
        m_width = width;
        m_height = height;
        m_numImages = numImages;

        m_rgenModule = rgenModule;
        m_rmissModule = rmissModule;
        m_rchitModule = rchitModule;
        m_reflMissModule = reflMissModule;
        m_reflHitModule = reflHitModule;

        createRawImages();
        createDescriptorSetLayout();
        createPipeline();
        createSBT();
        createUniformBuffers();
        createDescriptorPoolAndSets();
    }

    void RayTracingPipeline::Destroy() {
        if (!m_device) return;

        for (auto &ubo: m_rtUniformBuffers) ubo.Destroy(m_device);
        m_rtUniformBuffers.clear();

        if (m_sbtBuffer.m_buffer) m_sbtBuffer.Destroy(m_device);

        for (auto &buf: m_lightBuffers) buf.Destroy(m_device);
        m_lightBuffers.clear();

        if (m_rgenModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_rgenModule, nullptr);
            m_rgenModule = VK_NULL_HANDLE;
        }

        if (m_rmissModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_rmissModule, nullptr);
            m_rmissModule = VK_NULL_HANDLE;
        }

        if (m_rchitModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_rchitModule, nullptr);
            m_rchitModule = VK_NULL_HANDLE;
        }

        if (m_reflMissModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_reflMissModule, nullptr);
            m_reflMissModule = VK_NULL_HANDLE;
        }

        if (m_reflHitModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_reflHitModule, nullptr);
            m_reflHitModule = VK_NULL_HANDLE;
        }

        destroyShadowImages();
        DestroyBaseResources();
    }

    VkImage RayTracingPipeline::GetDiffuseRawImage(uint32_t imageIndex) const {
        assert(imageIndex < m_diffuseImages.size());
        return m_diffuseImages[imageIndex].m_image;
    }

    VkImageView RayTracingPipeline::GetDiffuseRawImageView(uint32_t imageIndex) const {
        assert(imageIndex < m_diffuseImages.size());
        return m_diffuseImages[imageIndex].m_view;
    }

    VkSampler RayTracingPipeline::GetDiffuseRawSampler() const {
        assert(!m_diffuseImages.empty());
        return m_diffuseImages[0].m_sampler;
    }

    void RayTracingPipeline::createRawImages() {
        m_diffuseImages.resize(m_numImages);
        m_specularImages.resize(m_numImages);
        m_specularMotionImages.resize(m_numImages);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            m_diffuseImages[i].Init(m_core);
            createRawImage(m_diffuseImages[i]);

            m_specularImages[i].Init(m_core);
            createRawImage(m_specularImages[i]);

            m_specularMotionImages[i].Init(m_core);
            createRawImage(m_specularMotionImages[i]);
        }
    }

    void RayTracingPipeline::destroyShadowImages() {
        for (auto &tex: m_diffuseImages) tex.Destroy(m_device);
        for (auto &tex: m_specularImages) tex.Destroy(m_device);
        for (auto &tex: m_specularMotionImages) tex.Destroy(m_device);

        m_specularImages.clear();
        m_diffuseImages.clear();
        m_specularMotionImages.clear();
    }

    void RayTracingPipeline::createRawImage(VulkanTexture &tex) {
        VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;

        VkImageCreateInfo imgInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {m_width, m_height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        VkResult res = vkCreateImage(m_device, &imgInfo, nullptr, &tex.m_image);
        CHECK_VK_RESULT(res, "vkCreateImage (shadow RT)");

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(m_device, tex.m_image, &memReq);

        VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReq.size,
            .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        res = vkAllocateMemory(m_device, &allocInfo, nullptr, &tex.m_mem);
        CHECK_VK_RESULT(res, "vkAllocateMemory (shadow RT)");

        res = vkBindImageMemory(m_device, tex.m_image, tex.m_mem, 0);
        CHECK_VK_RESULT(res, "vkBindImageMemory (shadow RT)");

        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = tex.m_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        res = vkCreateImageView(m_device, &viewInfo, nullptr, &tex.m_view);
        CHECK_VK_RESULT(res, "vkCreateImageView (shadow RT)");

        VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .maxAnisotropy = 1.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };

        res = vkCreateSampler(m_device, &samplerInfo, nullptr, &tex.m_sampler);
        CHECK_VK_RESULT(res, "vkCreateSampler (shadow RT)");
    }

    uint32_t RayTracingPipeline::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const {
        VkPhysicalDevice phys = m_core->GetPhysicalDevice().m_physicalDevice;
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(phys, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        printf("Failed to find suitable memory type for RT image\n");
        exit(1);
    }

    void RayTracingPipeline::createDescriptorSetLayout() {
        std::vector<VkDescriptorSetLayoutBinding> bindings(RTBindingCount);

        bindings[RTBindingTLAS] = {
            RTBindingTLAS, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr
        };
        bindings[RTBindingIndirectDiffuse] = {
            RTBindingIndirectDiffuse, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingParams] = {
            RTBindingParams, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr
        };
        bindings[RTBindingGPos] = {
            RTBindingGPos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingGNormal] = {
            RTBindingGNormal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingGAlbedo] = {
            RTBindingGAlbedo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingSkybox] = {
            RTBindingSkybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, nullptr
        };
        bindings[RTBindingBlueNoise] = {
            RTBindingBlueNoise, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingIndirectSpecular] = {
            RTBindingIndirectSpecular, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingGMetallic] = {
            RTBindingGMetallic, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingGRoughness] = {
            RTBindingGRoughness, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr
        };
        bindings[RTBindingSceneTextures] = {
            RTBindingSceneTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr
        };
        bindings[RTBindingLightBuffer] = {
            RTBindingLightBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr
        };

        std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size(), 0);
        bindingFlags[RTBindingSceneTextures] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
            .pBindingFlags = bindingFlags.data()
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &flagsInfo,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        VkResult res = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
        CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout (RT)");
    }

    void RayTracingPipeline::createPipeline() {
        if (!m_rgenModule || !m_rmissModule || !m_rchitModule) {
            printf("ERROR: Failed to create one or more RT shader modules\n");
        }

        std::array<VkPipelineShaderStageCreateInfo, 5> stages = {
            {
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                    .module = m_rgenModule,
                    .pName = "main"
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
                    .module = m_rmissModule,
                    .pName = "main"
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
                    .module = m_reflMissModule,
                    .pName = "main"
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                    .module = m_rchitModule,
                    .pName = "main"
                },
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                    .module = m_reflHitModule,
                    .pName = "main"
                }
            }
        };

        std::array<VkRayTracingShaderGroupCreateInfoKHR, 5> groups = {
            {
                {
                    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                    .generalShader = 0,
                    .closestHitShader = VK_SHADER_UNUSED_KHR,
                    .anyHitShader = VK_SHADER_UNUSED_KHR,
                    .intersectionShader = VK_SHADER_UNUSED_KHR
                },
                {
                    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                    .generalShader = 1,
                    .closestHitShader = VK_SHADER_UNUSED_KHR,
                    .anyHitShader = VK_SHADER_UNUSED_KHR,
                    .intersectionShader = VK_SHADER_UNUSED_KHR
                },
                {
                    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                    .generalShader = 2,
                    .closestHitShader = VK_SHADER_UNUSED_KHR,
                    .anyHitShader = VK_SHADER_UNUSED_KHR,
                    .intersectionShader = VK_SHADER_UNUSED_KHR
                },
                {
                    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                    .generalShader = VK_SHADER_UNUSED_KHR,
                    .closestHitShader = 3,
                    .anyHitShader = VK_SHADER_UNUSED_KHR,
                    .intersectionShader = VK_SHADER_UNUSED_KHR
                },
                {
                    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                    .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                    .generalShader = VK_SHADER_UNUSED_KHR,
                    .closestHitShader = 4,
                    .anyHitShader = VK_SHADER_UNUSED_KHR,
                    .intersectionShader = VK_SHADER_UNUSED_KHR
                }
            }
        };

        VkPipelineLayoutCreateInfo plInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };

        VkResult res = vkCreatePipelineLayout(m_device, &plInfo, nullptr, &m_pipelineLayout);
        CHECK_VK_RESULT(res, "vkCreatePipelineLayout (RT)");

        VkRayTracingPipelineCreateInfoKHR rtInfo = {
            .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
            .stageCount = static_cast<uint32_t>(stages.size()),
            .pStages = stages.data(),
            .groupCount = static_cast<uint32_t>(groups.size()),
            .pGroups = groups.data(),
            .maxPipelineRayRecursionDepth = 2,
            .layout = m_pipelineLayout
        };

        res = m_core->rtExtensions.vkCreateRayTracingPipelinesKHR(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtInfo,
                                                                  nullptr, &m_pipeline);
        CHECK_VK_RESULT(res, "vkCreateRayTracingPipelinesKHR");
    }

    void RayTracingPipeline::createSBT() {
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };
        VkPhysicalDeviceProperties2 props2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &rtProps
        };

        vkGetPhysicalDeviceProperties2(m_core->GetPhysicalDevice().m_physicalDevice, &props2);

        const uint32_t handleSize = rtProps.shaderGroupHandleSize;
        const uint32_t handleAlignment = rtProps.shaderGroupHandleAlignment;
        const uint32_t baseAlignment = rtProps.shaderGroupBaseAlignment;
        const uint32_t handleSizeAligned = AlignUpToMultiple(handleSize, handleAlignment);
        constexpr uint32_t groupCount = 5;

        uint32_t rgenSize = AlignUpToMultiple(handleSizeAligned, baseAlignment);
        uint32_t missSize = AlignUpToMultiple(2 * handleSizeAligned, baseAlignment);
        uint32_t hitSize = AlignUpToMultiple(2 * handleSizeAligned, baseAlignment);
        uint32_t sbtSize = rgenSize + missSize + hitSize;

        std::vector<uint8_t> shaderHandleStorage(groupCount * handleSize);

        VkResult res = m_core->rtExtensions.vkGetRayTracingShaderGroupHandlesKHR(
            m_device, m_pipeline, 0, groupCount, shaderHandleStorage.size(), shaderHandleStorage.data());
        CHECK_VK_RESULT(res, "vkGetRayTracingShaderGroupHandlesKHR");

        m_sbtBuffer = m_core->CreateBuffer(
            sbtSize,
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        uint8_t *dst = nullptr;
        res = vkMapMemory(m_device, m_sbtBuffer.m_mem, 0, sbtSize, 0, reinterpret_cast<void **>(&dst));
        CHECK_VK_RESULT(res, "vkMapMemory (SBT)");

        memset(dst, 0, sbtSize);

        memcpy(dst, shaderHandleStorage.data() + 0 * handleSize, handleSize);
        memcpy(dst + rgenSize, shaderHandleStorage.data() + 1 * handleSize, handleSize);
        memcpy(dst + rgenSize + handleSizeAligned, shaderHandleStorage.data() + 2 * handleSize, handleSize);
        memcpy(dst + rgenSize + missSize, shaderHandleStorage.data() + 3 * handleSize, handleSize);
        memcpy(dst + rgenSize + missSize + handleSizeAligned, shaderHandleStorage.data() + 4 * handleSize, handleSize);

        vkUnmapMemory(m_device, m_sbtBuffer.m_mem);

        VkDeviceAddress rawAddr = m_core->GetBufferDeviceAddress(m_sbtBuffer.m_buffer);

        m_sbtRaygenRegion.deviceAddress = rawAddr;
        m_sbtRaygenRegion.stride = rgenSize;
        m_sbtRaygenRegion.size = rgenSize;

        m_sbtMissRegion.deviceAddress = rawAddr + rgenSize;
        m_sbtMissRegion.stride = handleSizeAligned;
        m_sbtMissRegion.size = missSize;

        m_sbtHitRegion.deviceAddress = rawAddr + rgenSize + missSize;
        m_sbtHitRegion.stride = handleSizeAligned;
        m_sbtHitRegion.size = hitSize;

        m_sbtCallableRegion.deviceAddress = 0;
        m_sbtCallableRegion.stride = 0;
        m_sbtCallableRegion.size = 0;
    }

    void RayTracingPipeline::createUniformBuffers() {
        m_rtUniformBuffers = m_core->CreateUniformBuffers(sizeof(RayTracingUBO));
        m_lightBuffers.resize(m_numImages);
        size_t maxLightBufferSize = 16 + (sizeof(Light) * 1024);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            m_lightBuffers[i] = m_core->CreateBuffer(
                maxLightBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
    }

    void RayTracingPipeline::createDescriptorPoolAndSets() {
        std::array<VkDescriptorPoolSize, 5> poolSizes = {
            {
                {
                    .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                    .descriptorCount = m_numImages
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = m_numImages * 2
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = m_numImages
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = m_numImages * 7 + m_numImages * 1024
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = m_numImages
                }
            }
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = m_numImages,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };

        VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
        CHECK_VK_RESULT(res, "vkCreateDescriptorPool (RT)");

        std::vector layouts(m_numImages, m_descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = m_numImages,
            .pSetLayouts = layouts.data()
        };

        m_descSets.resize(m_numImages);
        res = vkAllocateDescriptorSets(m_device, &allocInfo, m_descSets.data());
        CHECK_VK_RESULT(res, "vkAllocateDescriptorSets (RT)");

        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> asInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> imageInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> specOutInfos(m_numImages);
        std::vector<VkDescriptorBufferInfo> uboInfos(m_numImages);
        std::vector<VkDescriptorBufferInfo> lightBufInfos(m_numImages);
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(m_numImages * 5);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            asInfos[i] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                .accelerationStructureCount = 1,
                .pAccelerationStructures = &m_tlas
            };

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = &asInfos[i],
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingTLAS,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
            });

            imageInfos[i] = {VK_NULL_HANDLE, m_diffuseImages[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingIndirectDiffuse,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfos[i]
            });

            specOutInfos[i] = {VK_NULL_HANDLE, m_specularImages[i].m_view, VK_IMAGE_LAYOUT_GENERAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingIndirectSpecular,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &specOutInfos[i]
            });

            uboInfos[i] = {m_rtUniformBuffers[i].m_buffer, 0, sizeof(RayTracingUBO)};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingParams,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &uboInfos[i]
            });

            lightBufInfos[i] = {m_lightBuffers[i].m_buffer, 0, VK_WHOLE_SIZE};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingLightBuffer,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &lightBufInfos[i]
            });
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void RayTracingPipeline::UpdateLights(uint32_t imageIndex, const std::vector<Light> &lights) {
        auto count = static_cast<uint32_t>(lights.size());
        if (count == 0) return;
        if (count > 1024) count = 1024;

        size_t dataSize = 16 + (sizeof(Light) * count);
        void *data;
        vkMapMemory(m_device, m_lightBuffers[imageIndex].m_mem, 0, dataSize, 0, &data);
        memcpy(data, &count, sizeof(uint32_t));
        memcpy(static_cast<uint8_t *>(data) + 16, lights.data(), sizeof(Light) * count);
        vkUnmapMemory(m_device, m_lightBuffers[imageIndex].m_mem);
    }

    void RayTracingPipeline::UpdateDescriptorSets(const std::vector<VkImageView> &posViews,
                                                  const std::vector<VkImageView> &normalViews,
                                                  const std::vector<VkImageView> &albedoViews,
                                                  const std::vector<VkImageView> &metallicViews,
                                                  const std::vector<VkImageView> &roughnessViews) const {
        VkSampler sampler = GetDiffuseRawSampler();
        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(m_numImages * 5);

        std::vector<VkDescriptorImageInfo> posInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> normInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> albedoInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> metallicInfos(m_numImages);
        std::vector<VkDescriptorImageInfo> roughnessInfos(m_numImages);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            posInfos[i] = {sampler, posViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_descSets[i], .dstBinding = RTBindingGPos,
                .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &posInfos[i]
            });

            normInfos[i] = {sampler, normalViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_descSets[i],
                .dstBinding = RTBindingGNormal, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &normInfos[i]
            });

            albedoInfos[i] = {sampler, albedoViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_descSets[i],
                .dstBinding = RTBindingGAlbedo, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &albedoInfos[i]
            });

            metallicInfos[i] = {sampler, metallicViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_descSets[i],
                .dstBinding = RTBindingGMetallic, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &metallicInfos[i]
            });

            roughnessInfos[i] = {sampler, roughnessViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = m_descSets[i],
                .dstBinding = RTBindingGRoughness, .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &roughnessInfos[i]
            });
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void RayTracingPipeline::UpdateBlueNoiseDescriptor(VulkanTexture &blueNoiseTexture) {
        if (blueNoiseTexture.m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, blueNoiseTexture.m_sampler, nullptr);
        }

        VkSamplerCreateInfo samplerInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .maxAnisotropy = 1.0f
        };
        vkCreateSampler(m_device, &samplerInfo, nullptr, &blueNoiseTexture.m_sampler);

        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorImageInfo> infos(m_numImages);
        writes.reserve(m_numImages);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            infos[i] = {blueNoiseTexture.m_sampler, blueNoiseTexture.m_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingBlueNoise,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &infos[i]
            });
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void RayTracingPipeline::RecordTraceRays(VkCommandBuffer cmdBuf, uint32_t imageIndex) {
        assert(imageIndex < m_descSets.size());
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipelineLayout, 0, 1,
                                &m_descSets[imageIndex], 0, nullptr);
        m_core->rtExtensions.vkCmdTraceRaysKHR(cmdBuf, &m_sbtRaygenRegion, &m_sbtMissRegion, &m_sbtHitRegion,
                                               &m_sbtCallableRegion, m_width, m_height, 1);
    }

    void RayTracingPipeline::UpdateTLAS(VkAccelerationStructureKHR tlas) {
        if (tlas == m_tlas) return;
        m_tlas = tlas;

        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> asInfos(m_numImages);
        writes.reserve(m_numImages);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            asInfos[i] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                .accelerationStructureCount = 1,
                .pAccelerationStructures = &m_tlas
            };
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = &asInfos[i],
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingTLAS,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
            });
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void RayTracingPipeline::UpdateUBO(const uint32_t imageIndex, const RayTracingUBO &data) {
        assert(imageIndex < m_rtUniformBuffers.size());
        m_rtUniformBuffers[imageIndex].Update(m_device, &data, sizeof(RayTracingUBO));
    }

    void RayTracingPipeline::UpdateBindlessTextures(const std::vector<VkDescriptorImageInfo> &textureInfos) {
        if (textureInfos.empty()) return;

        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(m_numImages);
        uint32_t count = static_cast<uint32_t>(textureInfos.size());
        count = count > 1024 ? 1024 : count;

        for (uint32_t i = 0; i < m_numImages; ++i) {
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingSceneTextures,
                .dstArrayElement = 0,
                .descriptorCount = count,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = textureInfos.data()
            });
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void RayTracingPipeline::UpdateSkyboxDescriptor(VkImageView skyboxView, VkSampler skyboxSampler) {
        if (skyboxView == VK_NULL_HANDLE || skyboxSampler == VK_NULL_HANDLE) return;

        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorImageInfo> infos(m_numImages);
        writes.reserve(m_numImages);

        for (uint32_t i = 0; i < m_numImages; ++i) {
            infos[i] = {skyboxSampler, skyboxView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descSets[i],
                .dstBinding = RTBindingSkybox,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &infos[i]
            });
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}
