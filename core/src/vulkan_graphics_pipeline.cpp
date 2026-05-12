#include <cstdio>
#include "types.h"
#include "vulkan-util.h"
#include "vulkan_graphics_pipeline_v2.h"

namespace VK {
    GraphicsPipelineV2::GraphicsPipelineV2(VkDevice Device,
                                           GLFWwindow *pWindow,
                                           VkRenderPass RenderPass,
                                           VkShaderModule vs,
                                           VkShaderModule fs,
                                           int NumImages,
                                           const std::vector<VkFormat> &ColorFormats,
                                           VkFormat DepthFormat)
        : VulkanPipelineBase(Device) {
        m_numImages = NumImages;

        bool IsVB = true;
        bool IsIB = true;
        bool IsUniform = true;
        bool IsTex = true;
        bool IsCubemap = false;
        CreateDescriptorSetLayout(IsVB, IsIB, IsTex, IsUniform, IsCubemap);

        InitCommon(pWindow, RenderPass, vs, fs, ColorFormats, DepthFormat, VK_COMPARE_OP_LESS);
    }

    GraphicsPipelineV2::GraphicsPipelineV2(const PipelineDesc &pd)
        : VulkanPipelineBase(pd.Device) {
        m_numImages = pd.NumImages;

        CreateDescriptorSetLayout(pd.IsVB, pd.IsIB, pd.IsTex2D, pd.IsUniform, pd.IsTexCube);

        std::vector<VkFormat> formats;
        formats.push_back(pd.ColorFormat);

        InitCommon(pd.pWindow, nullptr, pd.vs, pd.fs, formats, pd.DepthFormat, pd.DepthCompareOp);
    }

    GraphicsPipelineV2::~GraphicsPipelineV2() {
        Destroy();
    }

    void GraphicsPipelineV2::Destroy() {
        DestroyBaseResources();
    }

    void GraphicsPipelineV2::Bind(VkCommandBuffer CmdBuf) {
        vkCmdBindPipeline(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }

    void GraphicsPipelineV2::InitCommon(GLFWwindow *pWindow, VkRenderPass RenderPass,
                                        VkShaderModule vs, VkShaderModule fs,
                                        const std::vector<VkFormat> &ColorFormats,
                                        VkFormat DepthFormat, VkCompareOp DepthCompareOp) {
        VkPipelineShaderStageCreateInfo ShaderStageCreateInfo[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vs,
                .pName = "main",
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fs,
                .pName = "main"
            }
        };

        VkPipelineVertexInputStateCreateInfo VertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        VkPipelineInputAssemblyStateCreateInfo PipelineIACreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        int WindowWidth, WindowHeight;
        glfwGetWindowSize(pWindow, &WindowWidth, &WindowHeight);

        VkViewport VP = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float) WindowWidth,
            .height = (float) WindowHeight,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        VkRect2D Scissor = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width = (u32) WindowWidth,
                .height = (u32) WindowHeight
            }
        };

        VkPipelineViewportStateCreateInfo VPCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &VP,
            .scissorCount = 1,
            .pScissors = &Scissor
        };

        VkPipelineRasterizationStateCreateInfo RastCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f
        };

        VkPipelineMultisampleStateCreateInfo PipelineMSCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f
        };

        VkPipelineDepthStencilStateCreateInfo DepthStencilState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = DepthCompareOp,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };

        std::vector<VkPipelineColorBlendAttachmentState> BlendAttachments;
        BlendAttachments.reserve(ColorFormats.size());

        for (size_t i = 0; i < ColorFormats.size(); i++) {
            VkPipelineColorBlendAttachmentState BlendAttachState = {
                .blendEnable = VK_FALSE,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT
            };
            BlendAttachments.push_back(BlendAttachState);
        }

        VkPipelineColorBlendStateCreateInfo BlendCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = static_cast<u32>(BlendAttachments.size()),
            .pAttachments = BlendAttachments.data()
        };

        VkPipelineRenderingCreateInfo RenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<u32>(ColorFormats.size()),
            .pColorAttachmentFormats = ColorFormats.data(),
            .depthAttachmentFormat = DepthFormat,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        VkPipelineLayoutCreateInfo LayoutInfo = {};

        LayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };

        VkResult res = vkCreatePipelineLayout(m_device, &LayoutInfo, nullptr, &m_pipelineLayout);
        CHECK_VK_RESULT(res, "vkCreatePipelineLayout\n");

        VkGraphicsPipelineCreateInfo PipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = RenderPass ? nullptr : &RenderingInfo,
            .stageCount = std::size(ShaderStageCreateInfo),
            .pStages = &ShaderStageCreateInfo[0],
            .pVertexInputState = &VertexInputInfo,
            .pInputAssemblyState = &PipelineIACreateInfo,
            .pViewportState = &VPCreateInfo,
            .pRasterizationState = &RastCreateInfo,
            .pMultisampleState = &PipelineMSCreateInfo,
            .pDepthStencilState = &DepthStencilState,
            .pColorBlendState = &BlendCreateInfo,
            .layout = m_pipelineLayout,
            .renderPass = RenderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_pipeline);
        CHECK_VK_RESULT(res, "vkCreateGraphicsPipelines\n");

        printf("Graphics pipeline created with %zu color attachments\n", ColorFormats.size());
    }

    void GraphicsPipelineV2::CreateDescriptorPool(int MaxSets) {
        std::vector<VkDescriptorPoolSize> PoolSizes = {
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = static_cast<u32>(MaxSets * 2)
            },
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<u32>(MaxSets)
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = static_cast<u32>(MaxSets * 6)
            },
        };

        const VkDescriptorPoolCreateInfo PoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = 0,
            .maxSets = static_cast<u32>(MaxSets),
            .poolSizeCount = static_cast<u32>(PoolSizes.size()),
            .pPoolSizes = PoolSizes.data()
        };

        VkResult res = vkCreateDescriptorPool(m_device, &PoolInfo, nullptr, &m_descriptorPool);
        CHECK_VK_RESULT(res, "vkCreateDescriptorPool");
        printf("Descriptor pool created\n");
    }

    void GraphicsPipelineV2::CreateDescriptorSetLayout(
        const bool IsVB,
        const bool IsIB,
        const bool IsTex2D,
        const bool IsUniform,
        const bool IsCubemap) {
        std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;

        if (IsVB) {
            VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_VB = {
                .binding = BindingVB,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            };

            LayoutBindings.push_back(VertexShaderLayoutBinding_VB);
        }

        if (IsIB) {
            VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_IB = {
                .binding = BindingIB,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            };

            LayoutBindings.push_back(VertexShaderLayoutBinding_IB);
        }

        if (IsUniform) {
            VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_Uniform = {
                .binding = BindingUniform,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            };

            LayoutBindings.push_back(VertexShaderLayoutBinding_Uniform);
        }

        if (IsTex2D) {
            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Tex = {
                .binding = BindingTexture2D,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Tex);

            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Normal = {
                .binding = BindingNormalTexture,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Normal);

            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Metallic = {
                .binding = BindingMetallicTexture,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Metallic);

            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Roughness = {
                .binding = BindingRoughnessTexture,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Roughness);

            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Opacity = {
                .binding = BindingOpacityTexture,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Opacity);

            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_Emissive = {
                .binding = BindingEmissiveTexture,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };
            LayoutBindings.push_back(FragmentShaderLayoutBinding_Emissive);
        }

        if (IsCubemap) {
            VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding_TexCube = {
                .binding = BindingTextureCube,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            };

            LayoutBindings.push_back(FragmentShaderLayoutBinding_TexCube);
        }

        const VkDescriptorSetLayoutCreateInfo LayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<u32>(LayoutBindings.size()),
            .pBindings = LayoutBindings.data()
        };

        VkResult res = vkCreateDescriptorSetLayout(m_device, &LayoutInfo, nullptr, &m_descriptorSetLayout);
        CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout");
    }

    void GraphicsPipelineV2::AllocateDescriptorSets(int NumSubmeshes,
                                                    std::vector<std::vector<VkDescriptorSet> > &DescriptorSets) {
        CreateDescriptorPool(NumSubmeshes * m_numImages);
        AllocateDescriptorSetsInternal(NumSubmeshes, DescriptorSets);
    }

    void GraphicsPipelineV2::AllocateDescriptorSetsInternal(int NumSubmeshes,
                                                            std::vector<std::vector<VkDescriptorSet> > &
                                                            DescriptorSets) {
        std::vector Layouts(NumSubmeshes, m_descriptorSetLayout);

        VkDescriptorSetAllocateInfo AllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = (u32) Layouts.size(),
            .pSetLayouts = Layouts.data()
        };

        DescriptorSets.resize(m_numImages);

        for (auto &DescriptorSet: DescriptorSets) {
            DescriptorSet.resize(NumSubmeshes);

            VkResult res = vkAllocateDescriptorSets(m_device, &AllocInfo, DescriptorSet.data());
            CHECK_VK_RESULT(res, "vkAllocateDescriptorSets");
        }
    }

    void GraphicsPipelineV2::UpdateDescriptorSets(const ModelDesc &ModelDesc,
                                                  std::vector<std::vector<VkDescriptorSet> > &DescriptorSets) {
        u32 NumSubmeshes = (u32) DescriptorSets[0].size();

        std::vector<VkWriteDescriptorSet> WriteDescriptorSet;
        WriteDescriptorSet.reserve(m_numImages * NumSubmeshes * BindingCount);

        std::vector<VkDescriptorBufferInfo> BufferInfo_VBs(NumSubmeshes);
        std::vector<VkDescriptorBufferInfo> BufferInfo_IBs(NumSubmeshes);
        std::vector<std::vector<VkDescriptorBufferInfo> > BufferInfo_Uniforms(m_numImages);

        std::vector<VkDescriptorImageInfo> AlbedoImageInfo(NumSubmeshes);
        std::vector<VkDescriptorImageInfo> NormalImageInfo(NumSubmeshes);
        std::vector<VkDescriptorImageInfo> MetallicImageInfo(NumSubmeshes);
        std::vector<VkDescriptorImageInfo> RoughnessImageInfo(NumSubmeshes);
        std::vector<VkDescriptorImageInfo> OpacityImageInfo(NumSubmeshes);
        std::vector<VkDescriptorImageInfo> EmissiveImageInfo(NumSubmeshes);

        for (u32 SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
            BufferInfo_VBs[SubmeshIndex].buffer = ModelDesc.m_vb;
            BufferInfo_VBs[SubmeshIndex].offset = ModelDesc.m_ranges[SubmeshIndex].m_vbRange.m_offset;
            BufferInfo_VBs[SubmeshIndex].range = ModelDesc.m_ranges[SubmeshIndex].m_vbRange.m_range;

            BufferInfo_IBs[SubmeshIndex].buffer = ModelDesc.m_ib;
            BufferInfo_IBs[SubmeshIndex].offset = ModelDesc.m_ranges[SubmeshIndex].m_ibRange.m_offset;
            BufferInfo_IBs[SubmeshIndex].range = ModelDesc.m_ranges[SubmeshIndex].m_ibRange.m_range;

            AlbedoImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_sampler;
            AlbedoImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_imageView;
            AlbedoImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            NormalImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_normalSampler;
            NormalImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_normalImageView;
            NormalImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            MetallicImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_metallicSampler;
            MetallicImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_metallicImageView;
            MetallicImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            RoughnessImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_roughnessSampler;
            RoughnessImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_roughnessImageView;
            RoughnessImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            OpacityImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_opacitySampler;
            OpacityImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_opacityImageView;
            OpacityImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            EmissiveImageInfo[SubmeshIndex].sampler = ModelDesc.m_materials[SubmeshIndex].m_emissiveSampler;
            EmissiveImageInfo[SubmeshIndex].imageView = ModelDesc.m_materials[SubmeshIndex].m_emissiveImageView;
            EmissiveImageInfo[SubmeshIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        for (int ImageIndex = 0; ImageIndex < m_numImages; ImageIndex++) {
            BufferInfo_Uniforms[ImageIndex].resize(NumSubmeshes);

            for (u32 SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
                BufferInfo_Uniforms[ImageIndex][SubmeshIndex].buffer = ModelDesc.m_uniforms[ImageIndex];
                BufferInfo_Uniforms[ImageIndex][SubmeshIndex].offset = ModelDesc.m_ranges[SubmeshIndex].m_uniformRange.
                        m_offset;
                BufferInfo_Uniforms[ImageIndex][SubmeshIndex].range = ModelDesc.m_ranges[SubmeshIndex].m_uniformRange.
                        m_range;
            }
        }

        for (int ImageIndex = 0; ImageIndex < m_numImages; ImageIndex++) {
            for (u32 SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
                VkDescriptorSet DstSet = DescriptorSets[ImageIndex][SubmeshIndex];

                VkWriteDescriptorSet wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingVB,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &BufferInfo_VBs[SubmeshIndex]
                };

                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingIB,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &BufferInfo_IBs[SubmeshIndex]
                };

                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingUniform,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &BufferInfo_Uniforms[ImageIndex][SubmeshIndex]
                };

                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingTexture2D,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &AlbedoImageInfo[SubmeshIndex]
                };

                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingNormalTexture,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &NormalImageInfo[SubmeshIndex]
                };
                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingMetallicTexture,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &MetallicImageInfo[SubmeshIndex]
                };
                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingRoughnessTexture,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &RoughnessImageInfo[SubmeshIndex]
                };
                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingOpacityTexture,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &OpacityImageInfo[SubmeshIndex]
                };
                WriteDescriptorSet.push_back(wds);

                wds = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = DstSet,
                    .dstBinding = BindingEmissiveTexture,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &EmissiveImageInfo[SubmeshIndex]
                };
                WriteDescriptorSet.push_back(wds);
            }
        }

        vkUpdateDescriptorSets(m_device, static_cast<u32>(WriteDescriptorSet.size()), WriteDescriptorSet.data(), 0,
                               nullptr);
    }
}
