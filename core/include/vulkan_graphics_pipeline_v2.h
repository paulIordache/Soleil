#pragma once
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "types.h"
#include "vulkan-core.h"
#include "model_desc.h"
#include "vk_ray_tracing_pipeline.h"
#include "vulkan_pipeline_base.h"

namespace VK {
    enum Binding {
        BindingVB = 0,
        BindingIB = 1,
        BindingUniform = 2,
        BindingTexture2D = 3,
        BindingTextureCube = 4,
        BindingNormalTexture = 6,
        BindingMetallicTexture = 7,
        BindingRoughnessTexture = 8,
        BindingOpacityTexture = 9,
        BindingEmissiveTexture = 10,
        BindingCount
    };

    struct PipelineDesc {
        VkDevice Device = nullptr;
        GLFWwindow *pWindow = nullptr;
        VkShaderModule vs = nullptr;
        VkShaderModule fs = nullptr;
        int NumImages = 0;
        VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
        VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
        bool IsVB = false;
        bool IsIB = false;
        bool IsUniform = false;
        bool IsTex2D = false;
        bool IsTexCube = false;
    };

    class GraphicsPipelineV2 : public VulkanPipelineBase {
    public:

        GraphicsPipelineV2(VkDevice Device, GLFWwindow *pWindow, VkRenderPass RenderPass, VkShaderModule vs,
                           VkShaderModule fs, int NumImages, const std::vector<VkFormat> &ColorFormats,
                           VkFormat DepthFormat);

        GraphicsPipelineV2(const PipelineDesc &pd);

        ~GraphicsPipelineV2() override;

        void Destroy() override;

        void Bind(VkCommandBuffer CmdBuf);

        void AllocateDescriptorSets(int NumSubmeshes, std::vector<std::vector<VkDescriptorSet> > &DescriptorSets);

        void UpdateDescriptorSets(const ModelDesc &ModelDesc,
                                  std::vector<std::vector<VkDescriptorSet> > &DescriptorSets);

    private:
        void InitCommon(GLFWwindow *pWindow, VkRenderPass RenderPass, VkShaderModule vs, VkShaderModule fs,
                        const std::vector<VkFormat> &ColorFormats, VkFormat
                        DepthFormat, VkCompareOp DepthCompareOp);

        void AllocateDescriptorSetsInternal(int NumSubmeshes,
                                            std::vector<std::vector<VkDescriptorSet> > &DescriptorSets);

        void CreateDescriptorPool(int MaxSets);

        void CreateDescriptorSetLayout(bool IsVB, bool IsIB, bool IsTex2D, bool IsUniform, bool IsCubemap);


        int m_numImages = 0;
    };
}
