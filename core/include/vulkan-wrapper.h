#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "types.h"

namespace VK {
    void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags);

    VkSemaphore CreateSemaphore(VkDevice Device);

    void ImageMemBarrier(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format,
                         VkImageLayout OldLayout, VkImageLayout NewLayout, int LayerCount);

    VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format,
                            VkImageAspectFlags AspectFlags, bool IsCubemap, uint32_t mipLevels);

    VkSampler CreateTextureSampler(VkDevice Device, VkFilter MinFilter, VkFilter MaxFilter,
                               VkSamplerAddressMode AddressMode, uint32_t mipLevels);
}
