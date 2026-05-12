//
// Created by Paul on 9/29/2025.
//

#include "vulkan-shader.h"

#include <cassert>
#include <windows.h>
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Include/glslang_c_shader_types.h"

#include "file-util.h"
#include "glslang-util.h"

#include "types.h"
#include "vulkan-util.h"

namespace VK {
    VkShaderModule createShaderModuleFromText(VkDevice &Device, const char *pFilename) {
        std::string Source;

        if (!ReadFile(pFilename, Source)) {
            assert(0);
        }

        Shader ShaderModule;

        glslang_stage_t ShaderStage = ShaderStageFromFilename(pFilename);

        VkShaderModule ret = nullptr;

        glslang_initialize_process();

        if (compileShader(Device, ShaderStage, Source.c_str(), ShaderModule)) {
            printf("Created shader from text file '%s'\n", pFilename);
            ret = ShaderModule.shaderModule;
            std::string BinaryFilename = std::string(pFilename) + ".spv";
            WriteBinaryFile(BinaryFilename.c_str(), ShaderModule.SPIRV.data(),
                            static_cast<int>(ShaderModule.SPIRV.size()) * sizeof(uint32_t));
        }

        glslang_finalize_process();

        return ret;
    }
}
