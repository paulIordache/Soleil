#include <cstdio>

#include "vulkan-shader.h"
#include "vulkan-util.h"
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"
#include "glslang-util.h"

namespace VK {
    void printShaderSource(const char *text) {
        int line = 1;

        printf("\n(%3i) ", line);

        while (text && *text++) {
            if (*text == '\n') {
                printf("\n(%3i) ", ++line);
            } else if (*text == '\r') {
                // nothing to do
            } else {
                printf("%c", *text);
            }
        }

        printf("\n");
    }

    bool compileShader(VkDevice &Device, glslang_stage_t Stage, const char *pShaderCode,
                              Shader &ShaderModule) {
        glslang_input_t input = {
            .language = GLSLANG_SOURCE_GLSL,
            .stage = Stage,
            .client = GLSLANG_CLIENT_VULKAN,
            .client_version = GLSLANG_TARGET_VULKAN_1_2,
            .target_language = GLSLANG_TARGET_SPV,
            .target_language_version = GLSLANG_TARGET_SPV_1_4,
            .code = pShaderCode,
            .default_version = 100,
            .default_profile = GLSLANG_NO_PROFILE,
            .force_default_version_and_profile = false,
            .forward_compatible = false,
            .messages = GLSLANG_MSG_DEFAULT_BIT,
            .resource = glslang_default_resource()
        };

        glslang_shader_t *shader = glslang_shader_create(&input);

        if (!glslang_shader_preprocess(shader, &input)) {
            fprintf(stderr, "GLSL preprocessing failed\n");
            fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
            fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
            printShaderSource(input.code);
            return 0;
        }

        if (!glslang_shader_parse(shader, &input)) {
            fprintf(stderr, "GLSL parsing failed\n");
            fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
            fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
            printShaderSource(glslang_shader_get_preprocessed_code(shader));
            return 0;
        }

        glslang_program_t *program = glslang_program_create();
        glslang_program_add_shader(program, shader);

        if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
            fprintf(stderr, "GLSL linking failed\n");
            fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
            fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
            return 0;
        }

        glslang_program_SPIRV_generate(program, Stage);

        ShaderModule.init(program);

        const char *spirv_messages = glslang_program_SPIRV_get_messages(program);

        if (spirv_messages) {
            fprintf(stderr, "SPIR-V message: '%s'", spirv_messages);
        }

        VkShaderModuleCreateInfo shaderCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = ShaderModule.SPIRV.size() * sizeof(uint32_t),
            .pCode = static_cast<const uint32_t *>(ShaderModule.SPIRV.data())
        };

        VkResult res = vkCreateShaderModule(Device, &shaderCreateInfo, NULL, &ShaderModule.shaderModule);
        CHECK_VK_RESULT(res, "vkCreateShaderModule\n");

        glslang_program_delete(program);
        glslang_shader_delete(shader);

        bool ret = ShaderModule.SPIRV.size() > 0;

        return ret;
    }


    glslang_stage_t ShaderStageFromFilename(const char *pFilename) {
        std::string s(pFilename);

        if (s.ends_with(".vert")) {
            return GLSLANG_STAGE_VERTEX;
        }

        if (s.ends_with(".frag")) {
            return GLSLANG_STAGE_FRAGMENT;
        }

        if (s.ends_with(".geom")) {
            return GLSLANG_STAGE_GEOMETRY;
        }

        if (s.ends_with(".comp")) {
            return GLSLANG_STAGE_COMPUTE;
        }

        if (s.ends_with(".tesc")) {
            return GLSLANG_STAGE_TESSCONTROL;
        }

        if (s.ends_with(".tese")) {
            return GLSLANG_STAGE_TESSEVALUATION;
        }

        if (s.ends_with(".rgen")) {
            return GLSLANG_STAGE_RAYGEN;
        }

        if (s.ends_with(".rchit")) {
            return GLSLANG_STAGE_CLOSESTHIT;
        }

        if (s.ends_with(".rmiss")) {
            return GLSLANG_STAGE_MISS;
        }

        printf("Unknown shader stage in '%s'\n", pFilename);
        exit(1);

        return GLSLANG_STAGE_VERTEX;
    }
}
