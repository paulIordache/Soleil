

#ifndef VULKAN_ENGINE_MODEL_DESC_H
#define VULKAN_ENGINE_MODEL_DESC_H

#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace VK {
    struct RangeDesc {
        VkDeviceSize m_offset = 0;
        VkDeviceSize m_range = 0;
    };

    struct SubmeshRanges {
        RangeDesc m_vbRange;
        RangeDesc m_ibRange;
        RangeDesc m_uniformRange;
    };

    struct TextureInfo {
        VkSampler m_sampler = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;

        VkSampler m_normalSampler = VK_NULL_HANDLE;
        VkImageView m_normalImageView = VK_NULL_HANDLE;

        VkSampler m_metallicSampler = VK_NULL_HANDLE;
        VkImageView m_metallicImageView = VK_NULL_HANDLE;

        VkSampler m_roughnessSampler = VK_NULL_HANDLE;
        VkImageView m_roughnessImageView = VK_NULL_HANDLE;

        VkSampler m_opacitySampler = VK_NULL_HANDLE;
        VkImageView m_opacityImageView = VK_NULL_HANDLE;

        VkSampler m_emissiveSampler = VK_NULL_HANDLE;
        VkImageView m_emissiveImageView = VK_NULL_HANDLE;
    };

    struct ModelDesc {
        VkBuffer m_vb;
        VkBuffer m_ib;
        std::vector<VkBuffer> m_uniforms;
        std::vector<TextureInfo> m_materials;
        std::vector<SubmeshRanges> m_ranges;
    };
}



#endif //VULKAN_ENGINE_MODEL_DESC_H