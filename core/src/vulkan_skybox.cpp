#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vulkan_skybox.h"

#include "vulkan-shader.h"

namespace VK {
#define UNIFORM_BUFFER_SIZE sizeof(glm::mat4)


    void Skybox::init(VulkanCore *pVulkanCore, const char *pFilename) {
        m_pVulkanCore = pVulkanCore;
        m_numImages = pVulkanCore->GetNumImages();

        m_uniformBuffers = pVulkanCore->CreateUniformBuffers(UNIFORM_BUFFER_SIZE);

        m_cubemapTex.Init(pVulkanCore);
        m_cubemapTex.LoadEctCubemap(pFilename, false);

        m_vs = createShaderModuleFromText(pVulkanCore->GetDevice(), "../../shaders/skybox.vert");
        m_fs = createShaderModuleFromText(pVulkanCore->GetDevice(), "../../shaders/skybox.frag");

        PipelineDesc pd;
        pd.Device = pVulkanCore->GetDevice();
        pd.pWindow = pVulkanCore->GetWindow();
        pd.vs = m_vs;
        pd.fs = m_fs;
        pd.NumImages = m_numImages;
        pd.ColorFormat = m_pVulkanCore->GetSwapChainFormat();
        pd.DepthFormat = m_pVulkanCore->GetDepthFormat();
        pd.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        pd.IsTexCube = true;
        pd.IsUniform = true;

        m_pPipeline = new GraphicsPipelineV2(pd);

        createDescriptorSets();
    }


    void Skybox::destroy() {
        for (auto &m_uniformBuffer: m_uniformBuffers) {
            m_uniformBuffer.Destroy(m_pVulkanCore->GetDevice());
        }

        vkDestroyShaderModule(m_pVulkanCore->GetDevice(), m_vs, NULL);
        vkDestroyShaderModule(m_pVulkanCore->GetDevice(), m_fs, NULL);

        m_cubemapTex.Destroy(m_pVulkanCore->GetDevice());

        delete m_pPipeline;
    }


    void Skybox::createDescriptorSets() {
        int NumSubmeshes = 1;
        m_pPipeline->AllocateDescriptorSets(NumSubmeshes, m_descriptorSets);

        int NumBindings = 2;

        std::vector<VkWriteDescriptorSet> WriteDescriptorSet(m_numImages * NumBindings);

        VkDescriptorImageInfo ImageInfo = {
            .sampler = m_cubemapTex.m_sampler,
            .imageView = m_cubemapTex.m_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        std::vector<VkDescriptorBufferInfo> BufferInfo_Uniforms(m_numImages);

        int WdsIndex = 0;

        for (int ImageIndex = 0; ImageIndex < m_numImages; ImageIndex++) {
            BufferInfo_Uniforms[ImageIndex].buffer = m_uniformBuffers[ImageIndex].m_buffer;
            BufferInfo_Uniforms[ImageIndex].offset = 0;
            BufferInfo_Uniforms[ImageIndex].range = VK_WHOLE_SIZE;
        }

        for (int ImageIndex = 0; ImageIndex < m_numImages; ImageIndex++) {
            VkDescriptorSet DstSet = m_descriptorSets[ImageIndex][0];

            VkWriteDescriptorSet wds = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DstSet,
                .dstBinding = BindingUniform,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &BufferInfo_Uniforms[ImageIndex]
            };

            assert(WdsIndex < WriteDescriptorSet.size());
            WriteDescriptorSet[WdsIndex++] = wds;

            wds = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = DstSet,
                .dstBinding = BindingTextureCube,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &ImageInfo
            };

            assert(WdsIndex < WriteDescriptorSet.size());
            WriteDescriptorSet[WdsIndex++] = wds;
        }

        vkUpdateDescriptorSets(m_pVulkanCore->GetDevice(),
                               (u32) WriteDescriptorSet.size(), WriteDescriptorSet.data(), 0, NULL);
    }


    void Skybox::recordCommandBuffer(VkCommandBuffer CmdBuf, int ImageIndex) {
        m_pPipeline->Bind(CmdBuf);

        vkCmdBindDescriptorSets(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pPipeline->GetPipelineLayout(),
                                0, // firstSet
                                1, // descriptorSetCount
                                m_descriptorSets[ImageIndex].data(),
                                0, // dynamicOffsetCount
                                NULL); // pDynamicOffsets

        int NumVertices = 36;
        int InstanceCount = 1;
        int BaseVertex = 0;
        int FirstInstance = 0;

        vkCmdDraw(CmdBuf, NumVertices, InstanceCount, BaseVertex, FirstInstance);
    }


    void Skybox::update(int ImageIndex, const glm::mat4 &Transformation) {
        m_uniformBuffers[ImageIndex].Update(m_pVulkanCore->GetDevice(),
                                            glm::value_ptr(Transformation),
                                            sizeof(Transformation));
    }
};
