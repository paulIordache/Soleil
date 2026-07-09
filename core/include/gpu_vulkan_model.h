#pragma once

#include "vulkan_texture.h"
#include "vulkan_graphics_pipeline_v2.h"
#include "cpu_model.h"
#include "model_desc.h"
#include "types.h"

namespace VK {
    class BufferAndMemory;

    struct GeneralUBO {
        alignas(16) glm::mat4 WVP;
        alignas(16) glm::mat4 World;
        alignas(16) glm::mat4 prevWVP;
        alignas(16) glm::vec4 Ka_Ni;
        alignas(16) glm::vec4 Kd_Ns;
        alignas(16) glm::vec4 Ks_d;
        alignas(16) glm::vec4 Ke_Tf;
        alignas(16) glm::vec4 Tf;
    };


    class VulkanCore;

    class VkModel final : public CoreModel {
    public:
        VkModel() = default;

        void Destroy();

        void Init(VulkanCore *pVulkanCore) { m_pVulkanCore = pVulkanCore; }

        void CreateDescriptorSets(GraphicsPipelineV2 &Pipeline);

        void RecordCommandBuffer(VkCommandBuffer CmdBuf, GraphicsPipelineV2 &pPipeline, int ImageIndex);

        void Update(int ImageIndex, GeneralUBO uboData);

        [[nodiscard]] const BufferAndMemory *GetVB() const { return &m_vb; }

        [[nodiscard]] const BufferAndMemory *GetIB() const { return &m_ib; }

        void BuildTopLevelAS(std::vector<VkModel *> &models);

        glm::mat4 m_worldMatrix = glm::mat4(1.0f);

        glm::mat4 m_prevWorldMatrix = glm::mat4(1.0f);

        void BuildOrUpdateTLAS(std::vector<VK::VkModel *> &models, bool doUpdate);

        [[nodiscard]] VkAccelerationStructureKHR GetTLAS() const { return m_tlas; }

        [[nodiscard]] VkDeviceAddress GetVertexBufferAddress() const {
            if (!m_vb.m_buffer) return 0;
            return m_pVulkanCore->GetBufferDeviceAddress(m_vb.m_buffer);
        }

        [[nodiscard]] VkDeviceAddress GetIndexBufferAddress() const {
            if (!m_ib.m_buffer) return 0;
            return m_pVulkanCore->GetBufferDeviceAddress(m_ib.m_buffer);
        }

        [[nodiscard]] VkDeviceAddress GetObjDescAddress() const { return m_objDescAddress; }

        void SetGlobalTextureOffset(uint32_t offset) { m_globalTextureOffset = offset; }

        void RebuildObjDescBuffer();

    protected:
        virtual void AllocBuffers() {
            /* Nothing to do here */
        }

        VulkanTexture *AllocTexture2DVulkan();

        virtual void InitGeometryPost() {
            /* Nothing to do here */
        }

        void UpdateAlignedMeshesArray();


        void CreateBuffers(std::vector<Vertex> &Vertices);

        virtual void PopulateBuffers(vector<Vertex> &Vertices);

    private:
        void UpdateModelDesc(ModelDesc &md);

        uint32_t m_globalTextureOffset = 0;

        VulkanCore *m_pVulkanCore = nullptr;

        BufferAndMemory m_vb;
        BufferAndMemory m_ib;
        std::vector<BufferAndMemory> m_uniformBuffers;
        std::vector<std::vector<VkDescriptorSet> > m_descriptorSets;
        size_t m_vertexSize = 0;

        std::vector<VulkanMeshEntry> m_alignedMeshes;

        VkAccelerationStructureKHR m_blas = VK_NULL_HANDLE;
        VkDeviceAddress m_blasAddress = 0;
        BufferAndMemory m_blasStorage; // persistent BLAS backing

        VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
        VkDeviceAddress m_tlasAddress = 0;
        BufferAndMemory m_tlasStorage; // persistent backing buffer

        BufferAndMemory m_objDescBuffer;
        VkDeviceAddress m_objDescAddress = 0;
    };
}
