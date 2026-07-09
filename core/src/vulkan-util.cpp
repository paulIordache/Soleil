#include <vulkan/vulkan.h>

#include "vulkan-util.h"
#include "windows-macros.h"


const char *GetDebugSeverityStr(const VkDebugUtilsMessageSeverityFlagBitsEXT Severity) {
    switch (Severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return "Verbose";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            return "Info";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            return "Warning";

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            return "Error";

        default:
            VK_ENGINE_ERROR("Invalid severity code %d\n", Severity);
            exit(1);
    }
}

string GetDirFromFilename(const string& Filename)
{
    // Extract the directory part from the file name
    string::size_type SlashIndex;

#ifdef _WIN64
    SlashIndex = Filename.find_last_of("\\");

    if (SlashIndex == -1) {
        SlashIndex = Filename.find_last_of("/");
    }
#else
    SlashIndex = Filename.find_last_of("/");
#endif

    string Dir;

    if (SlashIndex == string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = Filename.substr(0, SlashIndex);
    }

    return Dir;
}


const char *GetDebugType(const VkDebugUtilsMessageTypeFlagsEXT type) {
    switch (type) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return "General";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            return "Validation";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            return "Performance";

        case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
            return "Device address binding";

        default:
            VK_ENGINE_ERROR("Invalid type code %d\n", type);
            exit(1);
    }
}


int GetBytesPerTexFormat(VkFormat Format) {
    switch (Format) {
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_UNORM:
            return 1;
        case VK_FORMAT_R16_SFLOAT:
            return 2;
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
            return 4;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 4 * sizeof(uint16_t);
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 3 * sizeof(float);
        case VK_FORMAT_R8G8B8_SRGB:
            return 3;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * sizeof(float);
        default:
            printf("Unknown format %d\n", Format);
            exit(1);
    }

    return 0;
}


bool HasStencilComponent(VkFormat Format) {
    return ((Format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
            (Format == VK_FORMAT_D24_UNORM_S8_UINT));
}


VkFormat FindSupportedFormat(VkPhysicalDevice Device, const std::vector<VkFormat> &Candidates,
                             VkImageTiling Tiling, VkFormatFeatureFlags Features) {
    for (const auto Format: Candidates) {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(Device, Format, &Props);

        if ((Tiling == VK_IMAGE_TILING_LINEAR) &&
            (Props.linearTilingFeatures & Features) == Features) {
            return Format;
        }
        if (Tiling == VK_IMAGE_TILING_OPTIMAL &&
            (Props.optimalTilingFeatures & Features) == Features) {
            return Format;
        }
    }

    printf("Failed to find supported format!\n");
    exit(1);
}
