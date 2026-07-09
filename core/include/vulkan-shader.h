
#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#pragma once
#include <vulkan/vulkan_core.h>

namespace VK {

    VkShaderModule createShaderModuleFromBinary(VkDevice &device, const char *pFileName);

    VkShaderModule createShaderModuleFromText(VkDevice &device, const char *pFileName);

}

#endif //VULKAN_SHADER_H
