#ifndef GLSLANG_UTIL_H
#define GLSLANG_UTIL_H
#include "types.h"
#include "vulkan-shader.h"

namespace VK {
    struct Shader {
        my_types::vec<u32> SPIRV;
        VkShaderModule shaderModule = VK_NULL_HANDLE;

        void init(glslang_program_t *program) {
            const size_t program_size = glslang_program_SPIRV_get_size(program);
            SPIRV.resize(program_size);
            glslang_program_SPIRV_get(program, SPIRV.data());
        }
    };

    void printShaderSource(const char *text);

    bool compileShader(VkDevice &Device, glslang_stage_t Stage, const char *pShaderCode,
                       Shader &ShaderModule);

    glslang_stage_t ShaderStageFromFilename(const char *pFilename);
}


#endif //GLSLANG_UTIL_H
