#include <vulkan/vulkan.h>
#include <GL/glew.h>

#include "vulkan-core.h"
#include "gpu_vulkan_model.h"
#include <glm/gtx/associated_min_max.hpp>
#include <glm/gtx/string_cast.hpp>

#include "vulkan_graphics_pipeline_v2.h"

namespace VK {
#define UNIFORM_BUFFER_SIZE sizeof(VK::GeneralUBO)

    static VkTransformMatrixKHR ToVkTransformMatrixKHR(const glm::mat4 &m) {
        VkTransformMatrixKHR out{};
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                out.matrix[row][col] = m[col][row];
        return out;
    }

    void VkModel::Destroy() {
        // raster buffers
        m_vb.Destroy(m_pVulkanCore->GetDevice());
        m_ib.Destroy(m_pVulkanCore->GetDevice());

        for (auto &m_uniformBuffer: m_uniformBuffers) {
            m_uniformBuffer.Destroy(m_pVulkanCore->GetDevice());
        }

        if (m_objDescBuffer.m_buffer) {
            m_objDescBuffer.Destroy(m_pVulkanCore->GetDevice());
        }

        // BLAS/TLAS (destroy handles, then their backing buffers)
        if (m_blas != VK_NULL_HANDLE) {
            m_pVulkanCore->rtExtensions.vkDestroyAccelerationStructureKHR(
                m_pVulkanCore->GetDevice(), m_blas, nullptr);
            m_blas = VK_NULL_HANDLE;
        }
        if (m_blasStorage.m_buffer) {
            m_blasStorage.Destroy(m_pVulkanCore->GetDevice());
        }

        if (m_tlas != VK_NULL_HANDLE) {
            m_pVulkanCore->rtExtensions.vkDestroyAccelerationStructureKHR(
                m_pVulkanCore->GetDevice(), m_tlas, nullptr);
            m_tlas = VK_NULL_HANDLE;
        }
        if (m_tlasStorage.m_buffer) {
            m_tlasStorage.Destroy(m_pVulkanCore->GetDevice());
        }
    }

    VulkanTexture *VkModel::AllocTexture2DVulkan() {
        assert(m_pVulkanCore);
        return new VulkanTexture(m_pVulkanCore);
    }

    void VkModel::UpdateAlignedMeshesArray() {
        VkDeviceSize Alignment = m_pVulkanCore->GetPhysicalDeviceLimits().minStorageBufferOffsetAlignment;

        size_t NumSubmeshes = m_Meshes.size();
        m_alignedMeshes.resize(NumSubmeshes);

        size_t BaseVertexOffset = 0;
        size_t BaseIndexOffset = 0;

        for (int SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
            m_alignedMeshes[SubmeshIndex].VertexBufferOffset = BaseVertexOffset;
            m_alignedMeshes[SubmeshIndex].VertexBufferRange = m_Meshes[SubmeshIndex].NumVertices * m_vertexSize;

            BaseVertexOffset += m_alignedMeshes[SubmeshIndex].VertexBufferRange;
            BaseVertexOffset = AlignUpToMultiple(BaseVertexOffset, Alignment);

            m_alignedMeshes[SubmeshIndex].IndexBufferOffset = BaseIndexOffset;
            m_alignedMeshes[SubmeshIndex].IndexBufferRange = m_Meshes[SubmeshIndex].NumIndices * sizeof(u32);

            BaseIndexOffset += m_alignedMeshes[SubmeshIndex].IndexBufferRange;
            BaseIndexOffset = AlignUpToMultiple(BaseIndexOffset, Alignment);
        }
    }

    void VkModel::RebuildObjDescBuffer() {
        size_t numSubmeshes = m_Meshes.size();
        if (numSubmeshes == 0) return;

        VkDeviceAddress baseVertexAddr = m_pVulkanCore->GetBufferDeviceAddress(m_vb.m_buffer);
        VkDeviceAddress baseIndexAddr  = m_pVulkanCore->GetBufferDeviceAddress(m_ib.m_buffer);

        std::vector<ObjDesc> objDescs(numSubmeshes);
        for (size_t i = 0; i < numSubmeshes; ++i) {
            objDescs[i].vertexAddress    = baseVertexAddr + m_alignedMeshes[i].VertexBufferOffset;
            objDescs[i].indexAddress     = baseIndexAddr  + m_alignedMeshes[i].IndexBufferOffset;
            int matIdx = m_Meshes[i].MaterialIndex;
            objDescs[i].diffuseTexIndex  = m_globalTextureOffset + matIdx;
        }

        m_objDescBuffer.Update(m_pVulkanCore->GetDevice(),
                               objDescs.data(),
                               sizeof(ObjDesc) * numSubmeshes);
    }

    void VkModel::CreateBuffers(std::vector<Vertex> &Vertices) {
        size_t numSubmeshes = m_alignedMeshes.size();
        if (numSubmeshes == 0) {
            printf("Warning: no submeshes found, skipping buffer creation.\n");
            return;
        }

        size_t vertexBufferSize =
                m_alignedMeshes[numSubmeshes - 1].VertexBufferOffset +
                m_alignedMeshes[numSubmeshes - 1].VertexBufferRange;

        size_t indexBufferSize =
                m_alignedMeshes[numSubmeshes - 1].IndexBufferOffset +
                m_alignedMeshes[numSubmeshes - 1].IndexBufferRange;

        char *pAlignedVertices = static_cast<char *>(malloc(vertexBufferSize));
        char *pSrcVertices = reinterpret_cast<char *>(Vertices.data());

        char *pAlignedIndices = static_cast<char *>(malloc(indexBufferSize));
        char *pSrcIndices = reinterpret_cast<char *>(m_Indices.data());

        for (int submeshIndex = 0; submeshIndex < numSubmeshes; submeshIndex++) {
            size_t srcOffset = m_Meshes[submeshIndex].BaseVertex * m_vertexSize;
            char *pSrc = pSrcVertices + srcOffset;
            char *pDst = pAlignedVertices + m_alignedMeshes[submeshIndex].VertexBufferOffset;
            size_t size = m_alignedMeshes[submeshIndex].VertexBufferRange;
            memcpy(pDst, pSrc, size);

            srcOffset = m_Meshes[submeshIndex].BaseIndex * sizeof(u32);
            pSrc = pSrcIndices + srcOffset;
            pDst = pAlignedIndices + m_alignedMeshes[submeshIndex].IndexBufferOffset;
            size = m_alignedMeshes[submeshIndex].IndexBufferRange;
            memcpy(pDst, pSrc, size);
        }

        m_vb = m_pVulkanCore->UploadDataToGPU(pAlignedVertices, vertexBufferSize);
        m_ib = m_pVulkanCore->UploadDataToGPU(pAlignedIndices, indexBufferSize);

        free(pAlignedVertices);
        free(pAlignedIndices);

        printf("Building multi-geometry BLAS for model...\n");

        VkDevice device = m_pVulkanCore->GetDevice();
        auto &rt = m_pVulkanCore->rtExtensions;

        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges;
        geometries.reserve(numSubmeshes);
        ranges.reserve(numSubmeshes);

        VkDeviceAddress baseVertexAddr = m_pVulkanCore->GetBufferDeviceAddress(m_vb.m_buffer);
        VkDeviceAddress baseIndexAddr = m_pVulkanCore->GetBufferDeviceAddress(m_ib.m_buffer);

        for (size_t i = 0; i < numSubmeshes; ++i) {
            const auto &aligned = m_alignedMeshes[i];
            const auto &mesh = m_Meshes[i];

            VkAccelerationStructureGeometryTrianglesDataKHR tri{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR
            };
            tri.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            tri.vertexData.deviceAddress = baseVertexAddr + aligned.VertexBufferOffset;
            tri.vertexStride = sizeof(Vertex);
            tri.maxVertex = mesh.NumVertices - 1; // FIX: highest index, not count
            tri.indexType = VK_INDEX_TYPE_UINT32;
            tri.indexData.deviceAddress = baseIndexAddr + aligned.IndexBufferOffset;
            tri.transformData.deviceAddress = 0;

            VkAccelerationStructureGeometryKHR geom{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
            };
            geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geom.geometry.triangles = tri;

            geometries.push_back(geom);

            VkAccelerationStructureBuildRangeInfoKHR range{};
            range.primitiveCount = mesh.NumIndices / 3;
            range.primitiveOffset = 0;
            range.firstVertex = 0;
            range.transformOffset = 0;
            ranges.push_back(range);
        }

        std::vector<uint32_t> primCounts(numSubmeshes);
        for (size_t i = 0; i < numSubmeshes; ++i)
            primCounts[i] = ranges[i].primitiveCount;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR
        };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
        buildInfo.pGeometries = geometries.data();

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
        };

        rt.vkGetAccelerationStructureBuildSizesKHR(
            device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo,
            primCounts.data(),
            &sizeInfo);

        // --- Allocate BLAS storage buffer (PERSISTENT!!) ---
        m_blasStorage = m_pVulkanCore->CreateBuffer(
            sizeInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureCreateInfoKHR asCreateInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR
        };
        asCreateInfo.buffer = m_blasStorage.m_buffer; // FIX: persistent buffer member
        asCreateInfo.size = sizeInfo.accelerationStructureSize;
        asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        VkResult res = rt.vkCreateAccelerationStructureKHR(device, &asCreateInfo, nullptr, &m_blas);
        if (res != VK_SUCCESS) {
            printf("Failed to create BLAS: %d\n", res);
            return;
        }

        buildInfo.dstAccelerationStructure = m_blas;

        // --- Scratch buffer ---
        BufferAndMemory scratch = m_pVulkanCore->CreateBuffer(
            sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        buildInfo.scratchData.deviceAddress = m_pVulkanCore->GetBufferDeviceAddress(scratch.m_buffer);

        // --- Build ranges pointers ---
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> pRanges(numSubmeshes);
        for (size_t i = 0; i < numSubmeshes; ++i)
            pRanges[i] = &ranges[i];

        // --- Record command buffer ---
        VkCommandBuffer cmd = m_pVulkanCore->CreateAndBeginSingleUseCommand();
        rt.vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRanges.data());
        m_pVulkanCore->EndSingleTimeCommands(cmd);

        // --- Query BLAS GPU address ---
        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR
        };
        addrInfo.accelerationStructure = m_blas;
        m_blasAddress = rt.vkGetAccelerationStructureDeviceAddressKHR(device, &addrInfo);

        printf("BLAS built successfully. GPU Address: 0x%llx\n", m_blasAddress);

        // cleanup
        scratch.Destroy(device);
        // DO NOT destroy m_blasStorage here (keep until Destroy())

        std::vector<ObjDesc> objDescs(numSubmeshes);
        for (size_t i = 0; i < numSubmeshes; ++i) {
            objDescs[i].vertexAddress = baseVertexAddr + m_alignedMeshes[i].VertexBufferOffset;
            objDescs[i].indexAddress = baseIndexAddr + m_alignedMeshes[i].IndexBufferOffset;

            int matIdx = m_Meshes[i].MaterialIndex;

            objDescs[i].diffuseTexIndex  = m_globalTextureOffset + matIdx;
        }

        VkDeviceSize objDescSize = sizeof(ObjDesc) * numSubmeshes;
        m_objDescBuffer = m_pVulkanCore->CreateBuffer(
            objDescSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        m_objDescBuffer.Update(device, objDescs.data(), objDescSize);
        m_objDescAddress = m_pVulkanCore->GetBufferDeviceAddress(m_objDescBuffer.m_buffer);

        // --- Create uniform buffers per mesh ---
        VkDeviceSize uniformAlignment = m_pVulkanCore->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment;
        VkDeviceSize alignedUniformSize = AlignUpToMultiple(UNIFORM_BUFFER_SIZE, uniformAlignment);
        m_uniformBuffers = m_pVulkanCore->CreateUniformBuffers(alignedUniformSize * m_Meshes.size());
    }

    void VkModel::PopulateBuffers(std::vector<Vertex> &Vertices) {
        m_vertexSize = sizeof(Vertex);
        UpdateAlignedMeshesArray();
        CreateBuffers(Vertices); // builds VB/IB + BLAS
    }

    void VkModel::CreateDescriptorSets(GraphicsPipelineV2 &Pipeline) {
        int NumSubmeshes = static_cast<int>(m_Meshes.size());
        Pipeline.AllocateDescriptorSets(NumSubmeshes, m_descriptorSets);

        ModelDesc md;
        UpdateModelDesc(md);
        Pipeline.UpdateDescriptorSets(md, m_descriptorSets);
    }

    // --- TLAS build/update (instances over model BLASes) ---
    void VkModel::BuildOrUpdateTLAS(std::vector<VkModel *> &models, bool doUpdate) {
        VkDevice device = m_pVulkanCore->GetDevice();
        auto &rt = m_pVulkanCore->rtExtensions;

        std::vector<VkAccelerationStructureInstanceKHR> instances;
        instances.reserve(models.size());

        for (uint32_t i = 0; i < models.size(); ++i) {
            VkAccelerationStructureInstanceKHR inst{};
            inst.transform = ToVkTransformMatrixKHR(models[i]->m_worldMatrix);
            inst.instanceCustomIndex = i;
            inst.mask = 0xFF;
            inst.instanceShaderBindingTableRecordOffset = 0;
            inst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            inst.accelerationStructureReference = models[i]->m_blasAddress;

            instances.push_back(inst);
        }

        const VkDeviceSize instBufSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
        BufferAndMemory instanceBuffer = m_pVulkanCore->CreateBuffer(
            instBufSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        instanceBuffer.Update(device, instances.data(), instBufSize);
        VkDeviceAddress instanceAddr = m_pVulkanCore->GetBufferDeviceAddress(instanceBuffer.m_buffer);

        VkAccelerationStructureGeometryInstancesDataKHR instData{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR
        };
        instData.arrayOfPointers = VK_FALSE;
        instData.data.deviceAddress = instanceAddr;

        VkAccelerationStructureGeometryKHR asGeom{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
        };
        asGeom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        asGeom.geometry.instances = instData;

        VkBuildAccelerationStructureFlagsKHR buildFlags =
                VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR
        };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = buildFlags;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &asGeom;
        buildInfo.mode = doUpdate
                             ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                             : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

        uint32_t primCount = static_cast<uint32_t>(instances.size());

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
        };
        rt.vkGetAccelerationStructureBuildSizesKHR(
            device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo,
            &primCount,
            &sizeInfo
        );

        if (!doUpdate) {
            m_tlasStorage = m_pVulkanCore->CreateBuffer(
                sizeInfo.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            VkAccelerationStructureCreateInfoKHR createInfo{
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR
            };
            createInfo.buffer = m_tlasStorage.m_buffer;
            createInfo.size = sizeInfo.accelerationStructureSize;
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

            VkResult res = rt.vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &m_tlas);
            if (res != VK_SUCCESS) {
                printf("Failed to create TLAS: %d\n", res);
                return;
            }
        }

        BufferAndMemory scratch = m_pVulkanCore->CreateBuffer(
            sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        buildInfo.dstAccelerationStructure = m_tlas;
        if (doUpdate) buildInfo.srcAccelerationStructure = m_tlas;
        buildInfo.scratchData.deviceAddress = m_pVulkanCore->GetBufferDeviceAddress(scratch.m_buffer);

        VkAccelerationStructureBuildRangeInfoKHR range{};
        range.primitiveCount = primCount;
        const VkAccelerationStructureBuildRangeInfoKHR *pRange = &range;

        VkCommandBuffer cmd = m_pVulkanCore->CreateAndBeginSingleUseCommand();
        rt.vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRange);
        m_pVulkanCore->EndSingleTimeCommands(cmd);

        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR
        };
        addrInfo.accelerationStructure = m_tlas;
        m_tlasAddress = rt.vkGetAccelerationStructureDeviceAddressKHR(device, &addrInfo);

        scratch.Destroy(device);
        instanceBuffer.Destroy(device);
    }

    void VkModel::BuildTopLevelAS(std::vector<VK::VkModel *> &models) {
        BuildOrUpdateTLAS(models, false);
    }

    void VkModel::UpdateModelDesc(ModelDesc &md) {
        md.m_vb = m_vb.m_buffer;
        md.m_ib = m_ib.m_buffer;

        md.m_uniforms.resize(m_pVulkanCore->GetNumImages());
        for (int ImageIndex = 0; ImageIndex < m_pVulkanCore->GetNumImages(); ImageIndex++) {
            md.m_uniforms[ImageIndex] = m_uniformBuffers[ImageIndex].m_buffer;
        }

        md.m_materials.resize(m_Meshes.size());
        md.m_ranges.resize(m_Meshes.size());

        int NumSubmeshes = static_cast<int>(m_Meshes.size());

        VkDeviceSize uniformAlignment = m_pVulkanCore->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment;
        VkDeviceSize alignedUniformSize = AlignUpToMultiple(UNIFORM_BUFFER_SIZE, uniformAlignment);

        for (int SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
            int MaterialIndex = m_Meshes[SubmeshIndex].MaterialIndex;

            if ((MaterialIndex >= 0) && (m_Materials[MaterialIndex].pDiffuseVulkan)) {
                VulkanTexture *pDiffuse = m_Materials[MaterialIndex].pDiffuseVulkan;
                md.m_materials[SubmeshIndex].m_sampler = pDiffuse->m_sampler;
                md.m_materials[SubmeshIndex].m_imageView = pDiffuse->m_view;
            } else {
                printf("No diffuse texture in material %d\n", MaterialIndex);
                exit(0);
            }

            if (m_Materials[MaterialIndex].pNormalVulkan) {
                VulkanTexture *pNormal = m_Materials[MaterialIndex].pNormalVulkan;
                md.m_materials[SubmeshIndex].m_normalSampler = pNormal->m_sampler;
                md.m_materials[SubmeshIndex].m_normalImageView = pNormal->m_view;
            } else {
                printf("No normal texture in material %d\n", MaterialIndex);
            }

            if (m_Materials[MaterialIndex].pMetallicVulkan) {
                VulkanTexture *pMetallic = m_Materials[MaterialIndex].pMetallicVulkan;
                md.m_materials[SubmeshIndex].m_metallicSampler = pMetallic->m_sampler;
                md.m_materials[SubmeshIndex].m_metallicImageView = pMetallic->m_view;
            } else {
                printf("No metallic texture in material %d\n", MaterialIndex);
            }

            if (m_Materials[MaterialIndex].pRoughnessVulkan) {
                VulkanTexture *pRoughness = m_Materials[MaterialIndex].pRoughnessVulkan;
                md.m_materials[SubmeshIndex].m_roughnessSampler = pRoughness->m_sampler;
                md.m_materials[SubmeshIndex].m_roughnessImageView = pRoughness->m_view;
            } else {
                printf("No roughness texture in material %d\n", MaterialIndex);
            }

            if (m_Materials[MaterialIndex].pOpacityVulkan) {
                VulkanTexture *pOpacity = m_Materials[MaterialIndex].pOpacityVulkan;
                md.m_materials[SubmeshIndex].m_opacitySampler = pOpacity->m_sampler;
                md.m_materials[SubmeshIndex].m_opacityImageView = pOpacity->m_view;
            } else {
                printf("No opacity texture in material %d\n", MaterialIndex);
            }

            if (m_Materials[MaterialIndex].pEmissiveVulkan) {
                VulkanTexture *pEmissive = m_Materials[MaterialIndex].pEmissiveVulkan;
                md.m_materials[SubmeshIndex].m_emissiveSampler = pEmissive->m_sampler;
                md.m_materials[SubmeshIndex].m_emissiveImageView = pEmissive->m_view;
            } else {
                printf("No emissive texture in material %d\n", MaterialIndex);
            }

            size_t offset = m_alignedMeshes[SubmeshIndex].VertexBufferOffset;
            size_t range = m_alignedMeshes[SubmeshIndex].VertexBufferRange;
            md.m_ranges[SubmeshIndex].m_vbRange = {.m_offset = offset, .m_range = range};

            offset = m_alignedMeshes[SubmeshIndex].IndexBufferOffset;
            range = m_alignedMeshes[SubmeshIndex].IndexBufferRange;
            md.m_ranges[SubmeshIndex].m_ibRange = {.m_offset = offset, .m_range = range};

            offset = SubmeshIndex * alignedUniformSize;
            range = UNIFORM_BUFFER_SIZE;
            md.m_ranges[SubmeshIndex].m_uniformRange = {.m_offset = offset, .m_range = range};
        }
    }

    void VkModel::RecordCommandBuffer(VkCommandBuffer CmdBuf, GraphicsPipelineV2 &Pipeline, int ImageIndex) {
        u32 InstanceCount = 1;
        u32 FirstInstance = 0;
        u32 BaseVertex = 0;

        for (u32 SubmeshIndex = 0; SubmeshIndex < m_Meshes.size(); SubmeshIndex++) {
            vkCmdBindDescriptorSets(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    Pipeline.GetPipelineLayout(),
                                    0, 1,
                                    &m_descriptorSets[ImageIndex][SubmeshIndex],
                                    0, nullptr);

            vkCmdDraw(CmdBuf, m_Meshes[SubmeshIndex].NumIndices,
                      InstanceCount, BaseVertex, FirstInstance);
        }
    }

    void VkModel::Update(int ImageIndex, GeneralUBO uboData) {
        VkDeviceSize uniformAlignment = m_pVulkanCore->GetPhysicalDeviceLimits().minUniformBufferOffsetAlignment;
        VkDeviceSize alignedUniformSize = AlignUpToMultiple(UNIFORM_BUFFER_SIZE, uniformAlignment);

        size_t totalSize = alignedUniformSize * m_Meshes.size();
        auto pAlignedData = static_cast<char *>(malloc(totalSize));
        memset(pAlignedData, 0, totalSize);

        for (u32 SubmeshIndex = 0; SubmeshIndex < m_Meshes.size(); SubmeshIndex++) {
            glm::mat4 MeshTransform = glm::make_mat4(m_Meshes[SubmeshIndex].Transformation.data());
            MeshTransform = glm::transpose(MeshTransform);

            glm::mat4 World = glm::mat4(uboData.World) * MeshTransform;
            glm::mat4 WVP = glm::mat4(uboData.WVP) * MeshTransform;
            glm::mat4 prevWVP = glm::mat4(uboData.prevWVP) * MeshTransform;

            GeneralUBO ubo{};
            ubo.WVP = WVP;
            ubo.World = World;
            ubo.prevWVP = prevWVP;

            int matIdx = m_Meshes[SubmeshIndex].MaterialIndex;

            if (matIdx >= 0 && matIdx < m_Materials.size()) {
                const Material &mat = m_Materials[matIdx];
                ubo.Ka_Ni = glm::vec4(mat.AmbientColor.x, mat.AmbientColor.y, mat.AmbientColor.z, mat.Ni);
                ubo.Kd_Ns = glm::vec4(mat.DiffuseColor.x, mat.DiffuseColor.y, mat.DiffuseColor.z, mat.Ns);
                ubo.Ks_d = glm::vec4(mat.SpecularColor.x, mat.SpecularColor.y, mat.SpecularColor.z, mat.Opacity);
                ubo.Ke_Tf = glm::vec4(mat.EmissiveColor.x, mat.EmissiveColor.y, mat.EmissiveColor.z, 0.0f);
                ubo.Tf = glm::vec4(mat.Transmission.x, mat.Transmission.y, mat.Transmission.z, 1.0f);
            } else {
                ubo.Ka_Ni = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                ubo.Kd_Ns = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f);
                ubo.Ks_d = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                ubo.Ke_Tf = glm::vec4(0.0f);
                ubo.Tf = glm::vec4(1.0f);
            }

            char *pDst = pAlignedData + (SubmeshIndex * alignedUniformSize);
            memcpy(pDst, &ubo, sizeof(GeneralUBO));
        }

        m_uniformBuffers[ImageIndex].Update(
            m_pVulkanCore->GetDevice(),
            pAlignedData,
            totalSize
        );

        free(pAlignedData);
    }
}
