#ifndef VULKAN_APP_H
#define VULKAN_APP_H
#include <GL/glew.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <fstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

#include "glm-camera.h"
#include "svgf_pipeline.h"
#include "gpu_vulkan_model.h"
#include "vk_ray_tracing_pipeline.h"
#include "vulkan-core.h"
#include "vulkan-glfw.h"
#include "vulkan-shader.h"
#include "vulkan-util.h"
#include "vulkan-wrapper.h"
#include "vulkan-simple-mesh.h"
#include "vulkan_graphics_pipeline_v2.h"
#include "vulkan_imgui.h"
#include "vulkan_skybox.h"

namespace VK {
    struct ProfilerRecord {
        std::string name;
        uint32_t startIndex;
        uint32_t endIndex;
    };

    struct FrameMetrics {
        uint64_t frameNumber;
        double cpuFrameTimeMs;
        size_t vramUsageMB;
        size_t ramUsageMB;
        uint64_t fragInvocations;
        uint64_t compInvocations;
    };

    struct StageMetrics {
        std::map<std::string, double> stages;
    };

    class BloomPipeline;
    class CompositePipeline;

    class VulkanApp : public GLFWCallbacks {
    public:
        VulkanApp(int WindowWidth, int WindowHeight) {
            m_windowWidth = WindowWidth;
            m_windowHeight = WindowHeight;
        }

        ~VulkanApp() override;

        void createShaders();
        void CreateSVGFPipeline(int fbWidth, int fbHeight);
        void CreateRayTracingPipeline(int fbWidth, int fbHeight);
        void createVertexBuffer();
        void createUniformBuffers();
        void init(const char *pAppName);
        void updateUniformBuffers(u32 imageIndex);
        void UpdateGUI();
        void renderScene();
        void RecordCommandBuffers();
        void ManagePipelinesViews();
        void DefaultCreateCameraPers();
        void DefaultCreateCameraPers(float FOV, float zNear, float zFar);
        void Key(GLFWwindow *pWindow, int Key, int Scancode, int Action, int Mods) override;
        void MouseMove(GLFWwindow *pWindow, double x, double y) override;
        void MouseButton(GLFWwindow *pWindow, int Button, int Action, int Mods) override;
        void createMesh();
        void loadTexture();
        void Execute();

        VkQueryPool m_queryPool = VK_NULL_HANDLE;
        float m_timestampPeriod = 1.0f;
        uint32_t m_maxQueriesPerFrame = 200;
        std::vector<std::vector<ProfilerRecord>> m_frameQueryRecords;
        std::vector<uint32_t> m_queryAllocated;

        uint32_t AllocateQuery(uint32_t frameIndex);
        void CollectTimestamps(uint32_t frameIndex);

    private:
        struct GBufferAttachment {
            VulkanTexture pos;
            VulkanTexture albedo;
            VulkanTexture normal;
            VulkanTexture depth;
            VulkanTexture velocity;
            VulkanTexture metallic;
            VulkanTexture roughness;
            VulkanTexture emissive;
            VulkanTexture geomNormal;
        };

        void RecordCommandBuffersInternal(bool WithSecondBarrier, std::vector<VkCommandBuffer> &CmdBufs);
        void createCommandBuffers();
        void BeginRendering(VkCommandBuffer CmdBuf, int ImageIndex);
        void CreateGeometryPipeline();
        void CreateCompositePipeline();
        void CreateViews();
        void UpdatePipelinesDescriptorSets() const;

        GLFWwindow *m_pWindow;
        VulkanCore m_vkCore;
        VkDevice m_device = nullptr;
        VulkanQueue *m_pQueue = nullptr;
        i32 m_numImages = 0;

        struct {
            std::vector<VkCommandBuffer> withGUI;
            std::vector<VkCommandBuffer> withoutGUI;
        } m_cmdBuffs;

        VkShaderModule m_vs;
        VkShaderModule m_fs;
        GraphicsPipelineV2 *m_pPipeline = nullptr;
        VkModel m_model;
        VkModel* m_pSphereLightModel = nullptr;

        std::vector<VkModel *> m_models;
        BufferAndMemory m_instanceAddressBuffer;
        Skybox m_skybox;
        SimpleMesh m_mesh;
        std::vector<BufferAndMemory> m_uniformBuffers;
        GLMCameraFirstPerson *m_pGameCamera = nullptr;
        ImGUI m_imGUIRenderer;
        bool m_showImgui = true;
        int m_windowWidth = 0;
        int m_windowHeight = 0;

        glm::vec3 m_dirLightDir = glm::vec3(0.489f, -1.0f, 0.032f);
        glm::vec3 m_dirLightColor = glm::vec3(1.0f, 0.98f, 0.85f);
        float m_dirLightStrength = 1.5f;
        float m_dirLightAngle = 0.001f;

        glm::vec3 m_sphLightPos = glm::vec3(13.058f, -11.05f, 3.12f);
        glm::vec3 m_sphLightColor = glm::vec3(1.0f, 0.84f, 0.67f);
        float m_sphLightStrength = 50.0f;
        float m_sphLightRadius = 0.5f;
        float m_sphLightAreaRadius = 5.0f;

        glm::vec3 m_clearColor = glm::vec3(0.0f);
        glm::vec3 m_position = glm::vec3(0.0f);
        glm::vec3 m_rotation = glm::vec3(0.0f);
        float m_scale = 1.0f;

        RayTracingPipeline *m_pRTPipeline = nullptr;
        glm::mat4 prev_wvp_matrix_ = glm::mat4(1.0f);
        bool m_denoiser = true;
        VkShaderModule m_rgenModule = VK_NULL_HANDLE;
        VkShaderModule m_rmissModule = VK_NULL_HANDLE;
        VkShaderModule m_rchitModule = VK_NULL_HANDLE;
        VkShaderModule m_reflMissModule = VK_NULL_HANDLE;
        VkShaderModule m_reflHitModule = VK_NULL_HANDLE;

        std::vector<GBufferAttachment> m_gBuffers;
        VulkanTexture m_blueNoise;

        CompositePipeline *m_pCompositePipeline = nullptr;
        VkShaderModule m_vsComposite = VK_NULL_HANDLE;
        VkShaderModule m_fsComposite = VK_NULL_HANDLE;
        VkShaderModule m_compSVGF = VK_NULL_HANDLE;
        VkShaderModule m_compSVGF_Spatial = VK_NULL_HANDLE;

        SVGFPipeline *m_pSVGFPipeline = nullptr;
        uint32_t m_frameCounter = 0;

        std::vector<VkImageView> m_posViews;
        std::vector<VkImageView> m_albedoViews;
        std::vector<VkImageView> m_normalViews;
        std::vector<VkImageView> m_rtDiffuseViews;
        std::vector<VkImageView> m_rtSpecularViews;
        std::vector<VkImageView> m_denoiseDiffuseViews;
        std::vector<VkImageView> m_denoiseSpecularViews;
        std::vector<VkImageView> m_motionViews;
        std::vector<VkImageView> m_specularMotionViews;
        std::vector<VkImageView> m_depthViews;
        std::vector<VkImageView> m_roughnessViews;
        std::vector<VkImageView> m_metallicViews;
        std::vector<VkImageView> m_rtSpecularMotionViews;
        std::vector<VkImageView> m_geomNormalViews;

        // --- Thesis Metrics Tracking ---
        VkQueryPool m_pipelineStatsPool = VK_NULL_HANDLE;
        std::vector<FrameMetrics> m_frameMetricsHistory;
        std::vector<StageMetrics> m_gpuStageHistory;
        uint64_t m_absoluteFrameCount = 0;
        uint64_t m_fragInvocations = 0;
        uint64_t m_compInvocations = 0;
        uint64_t m_totalVertices = 0;

        void CreateGBuffers(int width, int height);
    };
}
#endif