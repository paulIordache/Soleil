#include "composite_pipeline.h"
#include "vulkan-core.h"
#include "vulkan-util.h"
#include <stdio.h>
#include <glm/detail/_noise.hpp>

namespace VK {
    CompositePipeline::CompositePipeline(VulkanCore *core, VkRenderPass renderPass, VkFormat colorFormat)
        : VulkanPipelineBase(core->GetDevice(), core), m_gBufferSampler(nullptr) {
    }

    CompositePipeline::~CompositePipeline() {
        Destroy();
    }

    void CompositePipeline::Destroy() {
        DestroyBaseResources();
    }

    void CompositePipeline::Init(VkShaderModule vs, VkShaderModule fs) {
        std::vector<VkDescriptorSetLayoutBinding> bindings(CompBindingCount);

        bindings[CompBindingDenoisedLight] = {
            .binding = CompBindingDenoisedLight,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        bindings[CompBindingAlbedo] = {
            .binding = CompBindingAlbedo,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        bindings[CompBindingDenoisedSpecular] = {
            .binding = CompBindingDenoisedSpecular,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        bindings[CompBindingMetallic] = {
            .binding = CompBindingMetallic,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        bindings[CompBindingGNormal] = {
            .binding = CompBindingGNormal,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        bindings[CompBindingGPos] = {
            .binding = CompBindingGPos,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        VkResult res = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
        CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout (Composite)");

        VkPushConstantRange pushConstantRange = {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(CompositePushConstants)
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange
        };

        res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        CHECK_VK_RESULT(res, "vkCreatePipelineLayout (Composite)");

        VkFormat swapChainFormat = m_core->GetSwapChainFormat();
        VkFormat depthFormat = m_core->GetDepthFormat();

        VkPipelineRenderingCreateInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainFormat,
            .depthAttachmentFormat = depthFormat
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1
        };

        VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT
        };

        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
        };

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vs,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fs,
                .pName = "main"
            }
        };

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates
        };

        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &renderingInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = m_pipelineLayout
        };

        res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
        CHECK_VK_RESULT(res, "Create Composite Pipeline");
    }

    void CompositePipeline::UpdateDescriptorSets(
        const std::vector<VkImageView> &denoisedDiffuseViews,
        const std::vector<VkImageView> &denoisedSpecularViews,
        const std::vector<VkImageView> &albedoViews,
        const std::vector<VkImageView> &specularMaterialViews,
        const std::vector<VkImageView> &normalViews,
        const std::vector<VkImageView> &posViews,
        VkSampler sampler) {
        auto numImages = static_cast<uint32_t>(denoisedDiffuseViews.size());

        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        }

        VkDescriptorPoolSize poolSize = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = numImages * CompBindingCount
        };

        const VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = numImages,
            .poolSizeCount = 1,
            .pPoolSizes = &poolSize
        };

        VkResult res = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
        CHECK_VK_RESULT(res, "vkCreateDescriptorPool (Composite)");

        std::vector<VkDescriptorSetLayout> layouts(numImages, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = numImages,
            .pSetLayouts = layouts.data()
        };

        m_descriptorSets.resize(numImages);
        res = vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data());
        CHECK_VK_RESULT(res, "vkAllocateDescriptorSets (Composite)");

        std::vector<VkWriteDescriptorSet> writes;
        writes.reserve(numImages * CompBindingCount);

        std::vector<VkDescriptorImageInfo> lightInfos(numImages);
        std::vector<VkDescriptorImageInfo> albedoInfos(numImages);
        std::vector<VkDescriptorImageInfo> specInfos(numImages);
        std::vector<VkDescriptorImageInfo> metalInfos(numImages);
        std::vector<VkDescriptorImageInfo> normalInfos(numImages);
        std::vector<VkDescriptorImageInfo> posInfos(numImages);

        for (size_t i = 0; i < numImages; i++) {
            lightInfos[i] = {sampler, denoisedDiffuseViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            albedoInfos[i] = {sampler, albedoViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            specInfos[i] = {sampler, denoisedSpecularViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            metalInfos[i] = {sampler, specularMaterialViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            normalInfos[i] = {sampler, normalViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            posInfos[i] = {sampler, posViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingDenoisedLight,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &lightInfos[i]
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingAlbedo,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &albedoInfos[i]
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingDenoisedSpecular,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &specInfos[i]
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingMetallic,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &metalInfos[i]
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingGNormal,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &normalInfos[i]
            });

            writes.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = CompBindingGPos,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &posInfos[i]
            });
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    void CompositePipeline::RecordCommandBuffer(
        VkCommandBuffer cmd,
        const int imageIndex,
        const uint32_t width,
        const uint32_t height,
        const glm::vec3 &cameraPos,
        const glm::vec3 &lightDir) const {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                &m_descriptorSets[imageIndex], 0, nullptr);

        const VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = static_cast<float>(width), .height = static_cast<float>(height),
            .minDepth = 0.0f, .maxDepth = 1.0f
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        const VkRect2D scissor = {
            .offset = {0, 0},
            .extent = {width, height}
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        CompositePushConstants pc{};
        pc.cameraPos = cameraPos;
        pc.lightDir = lightDir;

        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants), &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
}
