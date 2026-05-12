#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

#include "vulkan-core.h"
#include "vulkan_texture.h"
#include "vulkan-util.h"
#include "vulkan_pipeline_base.h"

namespace VK {
    enum RTBinding {
        RTBindingTLAS = 0,
        RTBindingIndirectDiffuse = 1,
        RTBindingParams = 2,
        RTBindingGPos = 3,
        RTBindingGNormal = 4,
        RTBindingGAlbedo = 5,
        RTBindingSkybox = 6,
        RTBindingBlueNoise = 7,
        RTBindingIndirectSpecular = 8,
        RTBindingGMetallic = 9,
        RTBindingGRoughness = 10,
        RTBindingSceneTextures = 11,
        RTBindingLightBuffer = 12,
        RTBindingCount
    };

    struct Light {
        glm::vec4 posAndType;
        glm::vec4 colorAndStrength;
        glm::vec4 radiusData;
    };

    struct ObjDesc {
        uint64_t vertexAddress;
        uint64_t indexAddress;
        uint64_t diffuseTexIndex;
    };

    struct RayTracingUBO {
        alignas(16) glm::mat4 invViewProj;
        alignas(16) glm::vec3 cameraPos;
        alignas(16) glm::vec3 lightDir;
        alignas(16) glm::vec3 lightColor;
        uint32_t frameIndex;
        uint64_t objDescAddress;
        alignas(16) glm::mat4 prevViewProj;
    };

    class RayTracingPipeline : public VulkanPipelineBase {
    public:
        RayTracingPipeline(VulkanCore *core);

        ~RayTracingPipeline() override;

        void Init(VkAccelerationStructureKHR tlas, uint32_t width, uint32_t height, uint32_t numImages,
                  VkShaderModule rgenModule, VkShaderModule rmissModule, VkShaderModule rchitModule,
                  VkShaderModule reflMissModule, VkShaderModule reflHitModule);

        void Destroy() override;

        std::vector<VulkanTexture> m_specularImages;

        void RecordTraceRays(VkCommandBuffer cmdBuf, uint32_t imageIndex);

        void UpdateTLAS(VkAccelerationStructureKHR tlas);

        void UpdateDescriptorSets(const std::vector<VkImageView> &posViews, const std::vector<VkImageView> &normalViews,
                                  const std::vector<VkImageView> &albedoViews,
                                  const std::vector<VkImageView> &metallicViews,
                                  const std::vector<VkImageView> &roughnessViews) const;

        void UpdateBlueNoiseDescriptor(VulkanTexture &blueNoiseTexture);

        VkImage GetDiffuseRawImage(uint32_t imageIndex) const;

        VkImageView GetDiffuseRawImageView(uint32_t imageIndex) const;

        VkSampler GetDiffuseRawSampler() const;

        VkImageView GetSpecularImageView(uint32_t imageIndex) const { return m_specularImages[imageIndex].m_view; }
        VkImage GetSpecularImage(uint32_t imageIndex) const { return m_specularImages[imageIndex].m_image; }

        [[nodiscard]] VkImageView GetSpecularMotionImageView(uint32_t imageIndex) const {
            return m_specularMotionImages[imageIndex].m_view;
        }

        [[nodiscard]] VkImage GetSpecularMotionImage(uint32_t imageIndex) const {
            return m_specularMotionImages[imageIndex].m_image;
        }

        void UpdateUBO(uint32_t imageIndex, const RayTracingUBO &data);

        void UpdateBindlessTextures(const std::vector<VkDescriptorImageInfo> &textureInfos);

        void UpdateSkyboxDescriptor(VkImageView skyboxView, VkSampler skyboxSampler);

        void UpdateLights(uint32_t imageIndex, const std::vector<Light> &lights);

    private:
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_numImages = 0;

        VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;

        std::vector<VulkanTexture> m_diffuseImages;
        std::vector<VulkanTexture> m_specularMotionImages;

        std::vector<BufferAndMemory> m_rtUniformBuffers;

        std::vector<VkDescriptorSet> m_descSets;

        BufferAndMemory m_sbtBuffer;
        uint32_t m_currentLightCount = 0;
        std::vector<BufferAndMemory> m_lightBuffers;

        VkStridedDeviceAddressRegionKHR m_sbtRaygenRegion{};
        VkStridedDeviceAddressRegionKHR m_sbtMissRegion{};
        VkStridedDeviceAddressRegionKHR m_sbtHitRegion{};
        VkStridedDeviceAddressRegionKHR m_sbtCallableRegion{};

        VkShaderModule m_rgenModule = VK_NULL_HANDLE;
        VkShaderModule m_rmissModule = VK_NULL_HANDLE;
        VkShaderModule m_rchitModule = VK_NULL_HANDLE;
        VkShaderModule m_reflMissModule = VK_NULL_HANDLE;
        VkShaderModule m_reflHitModule = VK_NULL_HANDLE;

        void createRawImages();

        void destroyShadowImages();

        void createRawImage(VulkanTexture &tex);

        uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

        void createDescriptorSetLayout();

        void createPipeline();

        void createSBT();

        void createUniformBuffers();

        void createDescriptorPoolAndSets();
    };
}
