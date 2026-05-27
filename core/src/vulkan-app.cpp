#include "vulkan-app.h"

#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include <algorithm>

#include "glm-camera.h"
#include "math-3d.h"
#include "vulkan-glfw.h"

#include <glm/gtx/string_cast.hpp>

#include "composite_pipeline.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

namespace VK {
    struct ProfilerScope {
        VulkanCore &core;
        VkCommandBuffer cmd;
        VulkanApp *app;
        std::string name;
        uint32_t frameIndex;
        uint32_t startIndex;
        uint32_t endIndex{};

        ProfilerScope(VK::VulkanCore &c, VkCommandBuffer cb, VK::VulkanApp *a, const char *n, float r, float g, float b,
                      uint32_t fIdx)
            : core(c), cmd(cb), app(a), name(n), frameIndex(fIdx) {
            core.CmdBeginLabel(cmd, n, r, g, b);
            startIndex = app->AllocateQuery(frameIndex);
            vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, app->m_queryPool,
                                frameIndex * app->m_maxQueriesPerFrame + startIndex);
        }

        ~ProfilerScope() {
            endIndex = app->AllocateQuery(frameIndex);
            vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, app->m_queryPool,
                                frameIndex * app->m_maxQueriesPerFrame + endIndex);
            core.CmdEndLabel(cmd);
            app->m_frameQueryRecords[frameIndex].push_back({name, startIndex, endIndex});
        }
    };
}

#define PROFILE_PASS(cmd, name, r, g, b, fIdx) VK::ProfilerScope profile_scope_##__LINE__(m_vkCore, cmd, this, name, r, g, b, fIdx)

namespace VK {
    uint32_t VulkanApp::AllocateQuery(uint32_t frameIndex) {
        return m_queryAllocated[frameIndex]++;
    }

    void SetupImGuiStyle() {
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.85f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.90f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.85f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.02f, 0.02f, 0.02f, 0.95f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.85f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.50f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.90f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.90f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 0.60f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.70f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 0.90f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.40f, 0.40f, 0.40f, 0.50f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);

        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(4.0f, 3.0f);
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 21.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;

        style.WindowRounding = 4.0f;
        style.ChildRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;

        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
    }

    void VulkanApp::CollectTimestamps(uint32_t frameIndex) {
        if (m_queryAllocated[frameIndex] == 0) return;

        std::vector<uint64_t> results(m_queryAllocated[frameIndex] * 2);
        VkResult res = vkGetQueryPoolResults(
            m_device, m_queryPool, frameIndex * m_maxQueriesPerFrame, m_queryAllocated[frameIndex],
            results.size() * sizeof(uint64_t), results.data(), 2 * sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        if (res == VK_SUCCESS || res == VK_NOT_READY) {
            StageMetrics sm;
            bool hasData = false;

            for (const auto &record: m_frameQueryRecords[frameIndex]) {
                uint64_t start = results[record.startIndex * 2];
                uint64_t startAvail = results[record.startIndex * 2 + 1];
                uint64_t end = results[record.endIndex * 2];
                uint64_t endAvail = results[record.endIndex * 2 + 1];

                if (startAvail != 0 && endAvail != 0 && end > start) {
                    double ms = (end - start) * m_timestampPeriod * 1e-6;
                    sm.stages[record.name] = ms;
                    hasData = true;
                }
            }

            if (hasData) {
                m_gpuStageHistory.push_back(sm);
            }
        }
    }

    VulkanApp::~VulkanApp() {
        double totalAvgGpuTime = 0.0;
        std::map<std::string, double> stageAverages;

        if (!m_gpuStageHistory.empty()) {
            for (const auto &sm: m_gpuStageHistory) {
                double frameTotalGpu = 0.0;
                for (const auto &pair: sm.stages) {
                    frameTotalGpu += pair.second;
                    stageAverages[pair.first] += pair.second;
                }
                totalAvgGpuTime += frameTotalGpu;
            }

            totalAvgGpuTime /= m_gpuStageHistory.size();
            for (auto &pair: stageAverages) {
                pair.second /= m_gpuStageHistory.size();
            }
        }

        std::ofstream summaryCsv("../../thesis_summary.csv");
        summaryCsv << "Parameter,Value\n";
        summaryCsv << "GPU," << m_vkCore.GetPhysicalDevice().m_devProps.deviceName << "\n";
        summaryCsv << "Resolution," << m_windowWidth << "x" << m_windowHeight << "\n";
        summaryCsv << "Total Scene Vertices," << m_totalVertices << "\n";
        summaryCsv << "Frames Tracked (Hardware)," << m_frameMetricsHistory.size() << "\n";
        summaryCsv << "Frames Tracked (GPU Stages)," << m_gpuStageHistory.size() << "\n\n";

        double avgCpu = 0, avgVram = 0, avgRam = 0;
        if (!m_frameMetricsHistory.empty()) {
            for (const auto &m: m_frameMetricsHistory) {
                avgCpu += m.cpuFrameTimeMs;
                avgVram += m.vramUsageMB;
                avgRam += m.ramUsageMB;
            }
            double count = m_frameMetricsHistory.size();
            summaryCsv << "Avg CPU Frame Time (ms)," << avgCpu / count << "\n";
            summaryCsv << "Avg Total GPU Time (ms)," << totalAvgGpuTime << "\n";
            summaryCsv << "Avg VRAM Usage (MB)," << avgVram / count << "\n";
            summaryCsv << "Avg System RAM Usage (MB)," << avgRam / count << "\n\n";
        }

        summaryCsv << "GPU Stage,Average Time (ms)\n";
        for (const auto &pair: stageAverages) {
            summaryCsv << pair.first << "," << pair.second << "\n";
        }
        summaryCsv.close();

        std::ofstream hwCsv("../../thesis_hardware_metrics.csv");
        hwCsv << "Frame,CPU_Time_ms,VRAM_MB,RAM_MB\n";
        for (const auto &m: m_frameMetricsHistory) {
            hwCsv << m.frameNumber << ","
                    << m.cpuFrameTimeMs << ","
                    << m.vramUsageMB << ","
                    << m.ramUsageMB << "\n";
        }
        hwCsv.close();

        std::ofstream stageCsv("../../thesis_gpu_stages_metrics.csv");

        std::set<std::string> allStages;
        for (const auto &sm: m_gpuStageHistory) {
            for (const auto &pair: sm.stages) {
                allStages.insert(pair.first);
            }
        }

        stageCsv << "Frame";
        for (const auto &s: allStages) {
            stageCsv << ",\"" << s << " (ms)\"";
        }
        stageCsv << "\n";

        for (size_t i = 0; i < m_gpuStageHistory.size(); ++i) {
            const auto &sm = m_gpuStageHistory[i];
            stageCsv << i;

            for (const auto &s: allStages) {
                auto it = sm.stages.find(s);
                if (it != sm.stages.end()) {
                    stageCsv << "," << it->second;
                } else {
                    stageCsv << ",0.0";
                }
            }
            stageCsv << "\n";
        }
        stageCsv.close();

        if (m_queryPool != VK_NULL_HANDLE) {
            vkDestroyQueryPool(m_device, m_queryPool, nullptr);
        }

        m_vkCore.FreeCommandBuffers(static_cast<u32>(m_cmdBuffs.withGUI.size()),
                                    m_cmdBuffs.withGUI.data());
        m_vkCore.FreeCommandBuffers(static_cast<u32>(m_cmdBuffs.withoutGUI.size()),
                                    m_cmdBuffs.withoutGUI.data());
        vkDestroyShaderModule(m_device, m_vs, nullptr);
        vkDestroyShaderModule(m_device, m_fs, nullptr);
        vkDestroyShaderModule(m_device, m_vsComposite, nullptr);
        vkDestroyShaderModule(m_device, m_fsComposite, nullptr);
        vkDestroyShaderModule(m_device, m_compSVGF, nullptr);
        vkDestroyShaderModule(m_device, m_compSVGF_Spatial, nullptr);

        for (auto &gBuffer: m_gBuffers) {
            gBuffer.pos.Destroy(m_device);
            gBuffer.albedo.Destroy(m_device);
            gBuffer.normal.Destroy(m_device);
            gBuffer.depth.Destroy(m_device);
            gBuffer.velocity.Destroy(m_device);
            gBuffer.metallic.Destroy(m_device);
            gBuffer.roughness.Destroy(m_device);
        }
        delete m_pPipeline;
        m_pPipeline = nullptr;
        delete m_pRTPipeline;
        m_pRTPipeline = nullptr;
        delete m_pSVGFPipeline;
        m_pSVGFPipeline = nullptr;
        delete m_pCompositePipeline;
        m_pCompositePipeline = nullptr;

        delete m_pGameCamera;
        m_pGameCamera = nullptr;

        m_blueNoise.Destroy(m_device);

        for (auto *model: m_models) {
            model->Destroy();
            delete model;
        }
        m_pSphereLightModel = nullptr;
        m_models.clear();

        if (m_instanceAddressBuffer.m_buffer) {
            m_instanceAddressBuffer.Destroy(m_device);
        }

        m_skybox.destroy();
        m_imGUIRenderer.Destroy();
    }

    void VulkanApp::createShaders() {
        std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

        m_vs = createShaderModuleFromText(m_device, "../../shaders/g_buffer_norm.vert");
        m_fs = createShaderModuleFromText(m_device, "../../shaders/g_buffer_norm.frag");
        m_rgenModule = createShaderModuleFromText(m_device, "../../shaders/rt_light_oren_nayar.rgen");
        m_rmissModule = createShaderModuleFromText(m_device, "../../shaders/rt_shadow.rmiss");
        m_rchitModule = createShaderModuleFromText(m_device, "../../shaders/rt_shadow.rchit");
        m_reflMissModule = createShaderModuleFromText(m_device, "../../shaders/rt_reflection.rmiss");
        m_reflHitModule = createShaderModuleFromText(m_device, "../../shaders/rt_reflection.rchit");
        m_compSVGF = createShaderModuleFromText(m_device, "../../shaders/svgf.comp");
        m_compSVGF_Spatial = createShaderModuleFromText(m_device, "../../shaders/svgf_spatial.comp");
        m_vsComposite = createShaderModuleFromText(m_device, "../../shaders/composite.vert");
        m_fsComposite = createShaderModuleFromText(m_device, "../../shaders/composite.frag");
    }

    void VulkanApp::CreateSVGFPipeline(int fbWidth, int fbHeight) {
        m_pSVGFPipeline = new SVGFPipeline(&m_vkCore);
        m_pSVGFPipeline->Init(m_compSVGF, m_compSVGF_Spatial);
        m_pSVGFPipeline->CreateOutputImages(fbWidth, fbHeight, m_numImages);
        printf("SVGFPipeline initialized\n");
    }

    void VulkanApp::CreateRayTracingPipeline(int fbWidth, int fbHeight) {
        VkAccelerationStructureKHR tlas = m_models[0]->GetTLAS();

        if (tlas == VK_NULL_HANDLE) {
            printf("ERROR: TLAS is null, cannot init RT pipeline\n");
        } else {
            m_pRTPipeline = new RayTracingPipeline(&m_vkCore);
            m_pRTPipeline->Init(
                tlas,
                static_cast<uint32_t>(fbWidth),
                static_cast<uint32_t>(fbHeight),
                static_cast<uint32_t>(m_numImages),
                m_rgenModule,
                m_rmissModule,
                m_rchitModule,
                m_reflMissModule,
                m_reflHitModule
            );
            printf("RayTracingPipeline initialized\n");
        }
        m_blueNoise.Init(&m_vkCore);
        m_blueNoise.LoadFromDisk("../../content/blue_noise/256_256/HDR_L_0.png");
        m_pRTPipeline->UpdateBlueNoiseDescriptor(m_blueNoise);
    }

    void VulkanApp::init(const char *pAppName) {
        m_pWindow = glfw_vulkan_init(WINDOW_WIDTH, WINDOW_HEIGHT, pAppName);
        m_vkCore.Init(pAppName, m_pWindow, true);
        m_device = m_vkCore.GetDevice();
        m_numImages = m_vkCore.GetNumImages();
        m_pQueue = m_vkCore.GetQueue();

        m_timestampPeriod = m_vkCore.GetPhysicalDevice().m_devProps.limits.timestampPeriod;

        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = m_numImages * m_maxQueriesPerFrame;
        vkCreateQueryPool(m_device, &queryPoolInfo, nullptr, &m_queryPool);

        m_frameQueryRecords.resize(m_numImages);
        m_queryAllocated.resize(m_numImages, 0);

        createShaders();

        m_models.push_back(new VkModel());
        m_models.back()->Init(&m_vkCore);
        m_models.back()->LoadAssimpModel("../../content/HybridRendering/meshes/sponaza.obj");

        m_models.push_back(new VkModel());
        m_models.back()->Init(&m_vkCore);
        m_models.back()->LoadAssimpModel("../../content/coffee/CoffeeCart_01_4k.obj");

        m_models.push_back(new VkModel());
        m_models.back()->Init(&m_vkCore);
        m_models.back()->LoadAssimpModel("../../content/video_camera/video_camera.obj");

        m_models.push_back(new VkModel());
        m_models.back()->Init(&m_vkCore);
        m_models.back()->LoadAssimpModel("../../content/blue_barrel/blue_barrel.obj");

        m_models.push_back(new VkModel());
        m_models.back()->Init(&m_vkCore);
        m_models.back()->LoadAssimpModel("../../content/marble_statue/marble_bust.obj");

        m_totalVertices = 0;
        for (auto *model: m_models) {
            for (const auto &mesh: model->m_Meshes) {
                m_totalVertices += mesh.NumVertices;
            }
        }

        glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
        glm::mat4 Rotate = rotX * rotY * rotZ;
        glm::mat4 Rotate2 = glm::rotate(Rotate, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 Translate = glm::translate(glm::mat4(1.0f), m_position);
        glm::mat4 World = Translate * Rotate2 * Scale;

        for (auto *model: m_models) {
            model->m_worldMatrix = World;
            model->m_prevWorldMatrix = World;
        }

        m_models[0]->BuildTopLevelAS(m_models);

        int fbWidth = 0, fbHeight = 0;
        m_vkCore.GetFramebufferSize(fbWidth, fbHeight);

        CreateRayTracingPipeline(fbWidth, fbHeight);
        CreateSVGFPipeline(fbWidth, fbHeight);
        CreateGeometryPipeline();
        CreateCompositePipeline();

        m_skybox.init(&m_vkCore, "../../content/textures/181_hdrmaps_com_free_10K.jpg");
        CreateGBuffers(fbWidth, fbHeight);
        ManagePipelinesViews();

        if (m_pRTPipeline) {
            m_pRTPipeline->UpdateSkyboxDescriptor(m_skybox.m_cubemapTex.m_view, m_skybox.m_cubemapTex.m_sampler);
        }

        std::vector<VkDescriptorImageInfo> sceneTextures;
        uint32_t currentTextureOffset = 0;

        for (auto *model: m_models) {
            model->SetGlobalTextureOffset(currentTextureOffset);

            for (const auto &mat: model->m_Materials) {
                VkDescriptorImageInfo diffInfo{};
                if (mat.pDiffuseVulkan) {
                    diffInfo.sampler = mat.pDiffuseVulkan->m_sampler;
                    diffInfo.imageView = mat.pDiffuseVulkan->m_view;
                } else {
                    diffInfo.sampler = m_blueNoise.m_sampler;
                    diffInfo.imageView = m_blueNoise.m_view;
                }
                diffInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                sceneTextures.push_back(diffInfo);
            }

            model->RebuildObjDescBuffer();
            currentTextureOffset += model->m_Materials.size();
        }

        if (m_pRTPipeline) {
            m_pRTPipeline->UpdateBindlessTextures(sceneTextures);
        }

        printf("Pipelines Created (Geometry + RT + SVGF + Composite)\n");

        createCommandBuffers();
        DefaultCreateCameraPers();
        RecordCommandBuffers();

        glfw_vulkan_set_callbacks(m_pWindow, this);
        m_imGUIRenderer.Init(&m_vkCore);

        SetupImGuiStyle();
    }

    void VulkanApp::UpdateGUI() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (m_showFPSGraph) {
            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

            ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 360.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.95f);

            if (ImGui::Begin("Hardware Analytics", nullptr, overlayFlags)) {
                float minFps = 9999.0f, maxFps = 0.0f, avgFps = 0.0f;
                float minFt = 9999.0f, maxFt = 0.0f, avgFt = 0.0f;

                for (int i = 0; i < FPS_HISTORY_SIZE; ++i) {
                    float f = m_fpsHistory[i];
                    if (f < minFps && f > 0) minFps = f;
                    if (f > maxFps) maxFps = f;
                    avgFps += f;

                    float t = m_frameTimeHistory[i];
                    if (t < minFt && t > 0) minFt = t;
                    if (t > maxFt) maxFt = t;
                    avgFt += t;
                }
                avgFps /= FPS_HISTORY_SIZE;
                avgFt /= FPS_HISTORY_SIZE;

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[ GPU FRAMERATE ]");
                ImGui::Text("CURRENT: %5.1f FPS  |  AVG: %5.1f FPS", ImGui::GetIO().Framerate, avgFps);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "MIN: %.1f  |  MAX: %.1f",
                                   minFps == 9999.0f ? 0 : minFps, maxFps);
                ImGui::PlotLines("##fps", m_fpsHistory, FPS_HISTORY_SIZE, m_fpsHistoryOffset,
                                 nullptr, minFps > 5.0f ? minFps - 5.0f : 0.0f, maxFps + 5.0f, ImVec2(320, 60));

                ImGui::Separator();

                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.5f, 1.0f), "[ FRAME TIME ]");
                ImGui::Text("CURRENT: %5.2f MS   |  AVG: %5.2f MS", 1000.0f / ImGui::GetIO().Framerate, avgFt);
                ImGui::PlotHistogram("##frametime", m_frameTimeHistory, FPS_HISTORY_SIZE, m_fpsHistoryOffset,
                                     nullptr, 0.0f, maxFt * 1.2f, ImVec2(320, 60));

                ImGui::Separator();

                size_t vramUsage = 0, vramBudget = 0;
                m_vkCore.GetVRAMUsage(vramUsage, vramBudget);
                float vramRatio = vramBudget > 0 ? (float) vramUsage / (float) vramBudget : 0.0f;
                float vramMB = vramUsage / (1024.0f * 1024.0f);
                float vramBudgetMB = vramBudget / (1024.0f * 1024.0f);

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ VRAM ALLOCATION ]");
                ImGui::Text("DEDICATED USAGE: %.1f MB / %.1f MB", vramMB, vramBudgetMB);

                char vramOverlay[32];
                snprintf(vramOverlay, sizeof(vramOverlay), "%.1f %% UTILIZED", vramRatio * 100.0f);
                ImGui::ProgressBar(vramRatio, ImVec2(320, 18), vramOverlay);

#ifdef _WIN32
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc))) {
                    float ramMB = pmc.WorkingSetSize / (1024.0f * 1024.0f);
                    float totalRamMB = 16384.0f;
                    float ramRatio = std::min(ramMB / totalRamMB, 1.0f);

                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ SYSTEM RAM ]");
                    ImGui::Text("PROCESS ALLOCATION: %.1f MB", ramMB);

                    char ramOverlay[32];
                    snprintf(ramOverlay, sizeof(ramOverlay), "%.1f MB", ramMB);
                    ImGui::ProgressBar(ramRatio, ImVec2(320, 18), ramOverlay);
                }
#endif
            }
            ImGui::End();
        }

        if (m_showImgui) {
            ImGui::Begin("Lighting & Scene Controls");

            if (ImGui::CollapsingHeader("Global Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Show Advanced Analytics Overlay", &m_showFPSGraph);
                ImGui::Separator();
                ImGui::Text("Total Scene Vertices: %llu", m_totalVertices);
            }

            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SliderFloat3("Dir", &m_dirLightDir.x, -1.0f, 1.0f)) {
                    if (glm::length(m_dirLightDir) < 0.0001f) {
                        m_dirLightDir = glm::vec3(0.0f, -1.0f, 0.0f);
                    }
                }
                if (ImGui::Button("Normalize Vector")) {
                    m_dirLightDir = glm::normalize(m_dirLightDir);
                }
                ImGui::ColorEdit3("Dir Color", &m_dirLightColor.x);
                ImGui::SliderFloat("Dir Strength", &m_dirLightStrength, 0.0f, 10.0f);
                ImGui::SliderFloat("Sun Angle (Radius)", &m_dirLightAngle, 0.001f, 0.1f);
            }

            if (ImGui::CollapsingHeader("Spherical Area Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Enable Light", &m_sphLightEnabled);
                ImGui::SliderFloat3("Position", &m_sphLightPos.x, -100.0f, 100.0f);
                ImGui::ColorEdit3("Color", &m_sphLightColor.x);
                ImGui::SliderFloat("Strength", &m_sphLightStrength, 0.0f, 2000.0f);
                ImGui::SliderFloat("Radius", &m_sphLightRadius, 0.01f, 50.0f);
            }

            ImGui::End();
        }

        ImGui::Render();
    }

    void VulkanApp::updateUniformBuffers(u32 imageIndex) {
        static auto prevWorldMatrix = glm::mat4(1.0f);
        bool worldMatrixChanged = false;
        m_frameCounter++;

        glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1, 0, 0));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0, 1, 0));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
        glm::mat4 Rotate = rotX * rotY * rotZ;
        glm::mat4 Rotate2 = glm::rotate(Rotate, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 Translate = glm::translate(glm::mat4(1.0f), m_position);

        glm::mat4 World = Translate * Rotate2 * Scale;
        glm::mat4 VP = m_pGameCamera->GetVPMatrix();

        if (World != prevWorldMatrix) {
            worldMatrixChanged = true;
            prevWorldMatrix = World;
        }

        static glm::mat4 prevVPMatrix = m_pGameCamera->GetProjMatrixGLM() * m_pGameCamera->GetViewMatrix();
        glm::mat4 currentVPMatrix = m_pGameCamera->GetProjMatrixGLM() * m_pGameCamera->GetViewMatrix();

        std::vector<uint64_t> modelAddresses;
        for (auto *model: m_models) {
            if (model == m_pSphereLightModel) {
                glm::mat4 lightTranslate = glm::translate(glm::mat4(1.0f), m_sphLightPos);
                glm::mat4 lightScale = glm::scale(glm::mat4(1.0f), glm::vec3(m_sphLightRadius));
                model->m_worldMatrix = lightTranslate * lightScale;
            } else {
                model->m_worldMatrix = World;
            }

            GeneralUBO ubo = {};
            ubo.World = model->m_worldMatrix;
            ubo.WVP = VP * model->m_worldMatrix;
            ubo.prevWVP = prevVPMatrix * model->m_prevWorldMatrix;

            model->Update(imageIndex, ubo);
            model->m_prevWorldMatrix = model->m_worldMatrix;
            modelAddresses.push_back(model->GetObjDescAddress());
        }

        std::vector<Light> sceneLights;

        Light dirLight{};
        dirLight.posAndType = glm::vec4(glm::normalize(m_dirLightDir), 0.0f);
        dirLight.colorAndStrength = glm::vec4(m_dirLightColor, m_dirLightStrength);
        dirLight.radiusData = glm::vec4(m_dirLightAngle, 0.0f, 0.0f, 0.0f);
        sceneLights.push_back(dirLight);

        if (m_sphLightEnabled) {
            Light sphLight{};
            sphLight.posAndType = glm::vec4(m_sphLightPos, 1.0f);
            sphLight.colorAndStrength = glm::vec4(m_sphLightColor, m_sphLightStrength);
            sphLight.radiusData = glm::vec4(m_sphLightRadius, 0.0f, 0.0f, 0.0f);
            sceneLights.push_back(sphLight);
        }

        if (m_pRTPipeline) {
            m_pRTPipeline->UpdateLights(imageIndex, sceneLights);
        }

        if (m_pRTPipeline) {
            size_t addressBufferSize = modelAddresses.size() * sizeof(uint64_t);
            if (!m_instanceAddressBuffer.m_buffer) {
                m_instanceAddressBuffer = m_vkCore.CreateBuffer(
                    addressBufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                );
            }
            m_instanceAddressBuffer.Update(m_device, modelAddresses.data(), addressBufferSize);

            RayTracingUBO rtData{};
            rtData.invViewProj = glm::inverse(currentVPMatrix);
            rtData.cameraPos = m_pGameCamera->GetPosition();
            rtData.lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
            rtData.frameIndex = m_frameCounter;
            rtData.lightColor = glm::vec3(1.0, 1.0f, 1.0f);

            rtData.objDescAddress = m_vkCore.GetBufferDeviceAddress(m_instanceAddressBuffer.m_buffer);
            rtData.prevViewProj = prevVPMatrix;

            m_pRTPipeline->UpdateUBO(imageIndex, rtData);
        }

        prevVPMatrix = currentVPMatrix;

        if (worldMatrixChanged) {
            m_models[0]->BuildOrUpdateTLAS(m_models, true);

            if (m_pRTPipeline) {
                VkAccelerationStructureKHR tlas = m_models[0]->GetTLAS();
                m_pRTPipeline->UpdateTLAS(tlas);
            }
        }

        const glm::mat4 VPNoTranslate = m_pGameCamera->GetVPMatrixNoTranslate();
        m_skybox.update(imageIndex, VPNoTranslate);
    }

    void VulkanApp::renderScene() {
        u32 ImageIndex = m_pQueue->AcquireNextImage();
        CollectTimestamps(ImageIndex);
        updateUniformBuffers(ImageIndex);

        if (m_showImgui) {
            VkCommandBuffer ImGUICmdBuf = m_imGUIRenderer.PrepareCommandBuffer(ImageIndex);
            VkCommandBuffer CmdBufs[] = {m_cmdBuffs.withGUI[ImageIndex], ImGUICmdBuf};
            m_pQueue->SubmitAsync(&CmdBufs[0], 2);
        } else {
            m_pQueue->SubmitAsync(m_cmdBuffs.withoutGUI[ImageIndex]);
        }
        m_pQueue->Present(ImageIndex);
    }

    void VulkanApp::RecordCommandBuffers() {
        for (auto *model: m_models) {
            model->CreateDescriptorSets(*m_pPipeline);
        }
        for (int i = 0; i < m_numImages; i++) {
            m_frameQueryRecords[i].clear();
            m_queryAllocated[i] = 0;
        }
        RecordCommandBuffersInternal(true, m_cmdBuffs.withoutGUI);
        RecordCommandBuffersInternal(false, m_cmdBuffs.withGUI);
    }

    void VulkanApp::RecordCommandBuffersInternal(bool WithSecondBarrier, std::vector<VkCommandBuffer> &CmdBufs) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(m_pWindow, &fbWidth, &fbHeight);

        for (uint32_t i = 0; i < CmdBufs.size(); i++) {
            VkCommandBuffer &CmdBuf = CmdBufs[i];

            BeginCommandBuffer(CmdBuf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT); {
                vkCmdResetQueryPool(CmdBuf, m_queryPool, i * m_maxQueriesPerFrame, m_maxQueriesPerFrame);

                std::vector<VkImageMemoryBarrier> barriers;
                VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.srcAccessMask = 0;
                b.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                b.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                b.image = m_gBuffers[i].pos.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].albedo.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].normal.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].depth.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].velocity.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].metallic.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].roughness.m_image;
                barriers.push_back(b);

                vkCmdPipelineBarrier(CmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     0, 0, nullptr, 0, nullptr, (uint32_t) barriers.size(), barriers.data());
            }

            VkRenderingAttachmentInfo gBufferAttach[7];
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
            VkClearValue clearDepth = {{{0.0f, 0.0f, 0.0f, 0.0f}}};

            for (int k = 0; k < 7; ++k) {
                gBufferAttach[k] = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
                if (k == 0) gBufferAttach[k].imageView = m_gBuffers[i].pos.m_view;
                else if (k == 1) gBufferAttach[k].imageView = m_gBuffers[i].albedo.m_view;
                else if (k == 2) gBufferAttach[k].imageView = m_gBuffers[i].normal.m_view;
                else if (k == 3) gBufferAttach[k].imageView = m_gBuffers[i].depth.m_view;
                else if (k == 4) gBufferAttach[k].imageView = m_gBuffers[i].velocity.m_view;
                else if (k == 5) gBufferAttach[k].imageView = m_gBuffers[i].metallic.m_view;
                else if (k == 6) gBufferAttach[k].imageView = m_gBuffers[i].roughness.m_view;
                gBufferAttach[k].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                gBufferAttach[k].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                gBufferAttach[k].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                gBufferAttach[k].clearValue = (k == 3) ? clearDepth : clearColor;
            }

            VkRenderingAttachmentInfo depthAttach = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            depthAttach.imageView = m_vkCore.GetDepthView(i);
            depthAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttach.clearValue.depthStencil = {1.0f, 0};

            VkRenderingInfo geomPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {{0, 0}, {static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight)}},
                .layerCount = 1,
                .colorAttachmentCount = 7,
                .pColorAttachments = gBufferAttach,
                .pDepthAttachment = &depthAttach
            }; {
                PROFILE_PASS(CmdBuf, "G-Buffer", 0.8f, 0.2f, 0.2f, i);
                vkCmdBeginRendering(CmdBuf, &geomPassInfo);
                m_pPipeline->Bind(CmdBuf);
                for (auto *model: m_models) {
                    model->RecordCommandBuffer(CmdBuf, *m_pPipeline, i);
                }
                vkCmdEndRendering(CmdBuf);
            } {
                std::vector<VkImageMemoryBarrier> barriers;
                VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                b.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                b.image = m_gBuffers[i].pos.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].albedo.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].normal.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].depth.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].velocity.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].metallic.m_image;
                barriers.push_back(b);
                b.image = m_gBuffers[i].roughness.m_image;
                barriers.push_back(b);

                if (m_pRTPipeline) {
                    VkImageMemoryBarrier rtB = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                    rtB.srcAccessMask = 0;
                    rtB.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    rtB.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    rtB.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    rtB.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                    rtB.image = m_pRTPipeline->GetDiffuseRawImage(i);
                    barriers.push_back(rtB);

                    rtB.image = m_pRTPipeline->GetSpecularImage(i);
                    barriers.push_back(rtB);
                }

                vkCmdPipelineBarrier(CmdBuf,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(barriers.size()),
                                     barriers.data());
            }

            if (m_pRTPipeline) {
                PROFILE_PASS(CmdBuf, "RT", 0.8f, 0.8f, 0.2f, i);
                m_pRTPipeline->RecordTraceRays(CmdBuf, i);
            }

            if (m_pRTPipeline && m_pSVGFPipeline && m_denoiser) {
                std::vector<VkImageMemoryBarrier> bTemp;

                VkImageMemoryBarrier bRT = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                bRT.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                bRT.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                bRT.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                bRT.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                bRT.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                bRT.image = m_pRTPipeline->GetDiffuseRawImage(i);
                bTemp.push_back(bRT);

                bRT.image = m_pRTPipeline->GetSpecularImage(i);
                bTemp.push_back(bRT);

                VkImageMemoryBarrier bInter = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                bInter.srcAccessMask = 0;
                bInter.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                bInter.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                bInter.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                bInter.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                bInter.image = m_pSVGFPipeline->GetIntermediateDiffuse(i).m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->GetIntermediateSpecular(i).m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->m_momentsImages[i].m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->m_momentsImagesSpec[i].m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->GetVarianceDiff(0, i).m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->GetVarianceDiff(1, i).m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->GetVarianceSpec(0, i).m_image;
                bTemp.push_back(bInter);
                bInter.image = m_pSVGFPipeline->GetVarianceSpec(1, i).m_image;
                bTemp.push_back(bInter);

                vkCmdPipelineBarrier(
                    CmdBuf, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                    static_cast<uint32_t>(bTemp.size()), bTemp.data()); {
                    PROFILE_PASS(CmdBuf, "SVGF_Temp", 0.2f, 0.8f, 0.2f, i);
                    m_pSVGFPipeline->DispatchTemporal(CmdBuf, fbWidth, fbHeight, i);
                }

                int step = 1;
                for (int iter = 0; iter < 5; iter++) {
                    bool isPing = (iter % 2 == 0);
                    VulkanTexture &diffIn = isPing
                                                ? m_pSVGFPipeline->GetIntermediateDiffuse(i)
                                                : m_pSVGFPipeline->GetDenoisedDiffuse(i);
                    VulkanTexture &specIn = isPing
                                                ? m_pSVGFPipeline->GetIntermediateSpecular(i)
                                                : m_pSVGFPipeline->GetDenoisedSpecular(i);
                    VulkanTexture &diffOut = isPing
                                                 ? m_pSVGFPipeline->GetDenoisedDiffuse(i)
                                                 : m_pSVGFPipeline->GetIntermediateDiffuse(i);
                    VulkanTexture &specOut = isPing
                                                 ? m_pSVGFPipeline->GetDenoisedSpecular(i)
                                                 : m_pSVGFPipeline->GetIntermediateSpecular(i);

                    int varWriteSlot = iter % 2;
                    VulkanTexture &varDiffOut = m_pSVGFPipeline->GetVarianceDiff(varWriteSlot, i);
                    VulkanTexture &varSpecOut = m_pSVGFPipeline->GetVarianceSpec(varWriteSlot, i);

                    std::vector<VkImageMemoryBarrier> bSpat;
                    VkImageSubresourceRange sr = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}; {
                        VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                        b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        b.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        b.subresourceRange = sr;
                        b.image = diffIn.m_image;
                        bSpat.push_back(b);
                        b.image = specIn.m_image;
                        bSpat.push_back(b);
                    } {
                        VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                        b.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        b.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                        b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        b.subresourceRange = sr;
                        b.image = diffOut.m_image;
                        bSpat.push_back(b);
                        b.image = specOut.m_image;
                        bSpat.push_back(b);
                    }
                    if (iter >= 1) {
                        int varReadSlot = (iter - 1) % 2;
                        VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                        b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        b.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                        b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                        b.subresourceRange = sr;
                        b.image = m_pSVGFPipeline->GetVarianceDiff(varReadSlot, i).m_image;
                        bSpat.push_back(b);
                        b.image = m_pSVGFPipeline->GetVarianceSpec(varReadSlot, i).m_image;
                        bSpat.push_back(b);
                    }

                    vkCmdPipelineBarrier(CmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                         0, 0, nullptr, 0, nullptr,
                                         static_cast<uint32_t>(bSpat.size()), bSpat.data()); {
                        char passName[32];
                        snprintf(passName, sizeof(passName), "SVGF_Spatial_%d", step);
                        PROFILE_PASS(CmdBuf, passName, 0.2f, 0.5f, 0.8f, i);
                        m_pSVGFPipeline->DispatchSpatial(CmdBuf, fbWidth, fbHeight, i, step, iter);
                    }

                    step *= 2;
                }

                std::vector<VkImageMemoryBarrier> bComp;
                VkImageMemoryBarrier bEnd = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                bEnd.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                bEnd.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                bEnd.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                bEnd.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                bEnd.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                bEnd.image = m_pSVGFPipeline->GetDenoisedDiffuse(i).m_image;
                bComp.push_back(bEnd);
                bEnd.image = m_pSVGFPipeline->GetDenoisedSpecular(i).m_image;
                bComp.push_back(bEnd);

                vkCmdPipelineBarrier(CmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     0, 0, nullptr, 0, nullptr, (uint32_t) bComp.size(), bComp.data());
            } else if (m_pRTPipeline) {
                std::vector<VkImageMemoryBarrier> barriers;
                VkImageMemoryBarrier b = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                b.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

                b.image = m_pRTPipeline->GetDiffuseRawImage(i);
                barriers.push_back(b);
                b.image = m_pRTPipeline->GetSpecularImage(i);
                barriers.push_back(b);

                if (!barriers.empty()) {
                    vkCmdPipelineBarrier(CmdBuf,
                                         VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                         0, 0, nullptr, 0, nullptr, (uint32_t) barriers.size(), barriers.data());
                }
            }

            ImageMemBarrier(CmdBuf, m_vkCore.GetImage(i), m_vkCore.GetSwapChainFormat(),
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

            VkRenderingAttachmentInfo swapchainAttach = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_vkCore.GetImageView(i),
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {{{0.1f, 0.1f, 0.1f, 1.0f}}}
            };

            VkRenderingAttachmentInfo depthAttachComp = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = m_vkCore.GetDepthView(i),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            };

            VkRenderingInfo compPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {
                    {0, 0},
                    {static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight)}
                },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &swapchainAttach,
                .pDepthAttachment = &depthAttachComp
            }; {
                PROFILE_PASS(CmdBuf, "Comp_Sky", 0.6f, 0.2f, 0.8f, i);
                vkCmdBeginRendering(CmdBuf, &compPassInfo);

                glm::vec3 mainLightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));

                m_pCompositePipeline->RecordCommandBuffer(CmdBuf, i, static_cast<uint32_t>(fbWidth),
                                                          static_cast<uint32_t>(fbHeight),
                                                          m_pGameCamera->GetPosition(),
                                                          mainLightDir);

                m_skybox.recordCommandBuffer(CmdBuf, i);

                vkCmdEndRendering(CmdBuf);
            }

            if (WithSecondBarrier) {
                ImageMemBarrier(CmdBuf, m_vkCore.GetImage(i), m_vkCore.GetSwapChainFormat(),
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
            }

            VkResult res = vkEndCommandBuffer(CmdBuf);
            CHECK_VK_RESULT(res, "vkEndCommandBuffer");
        }
    }

    void VulkanApp::createCommandBuffers() {
        m_cmdBuffs.withGUI.resize(m_numImages);
        m_vkCore.CreateCommandBuffers(m_numImages, m_cmdBuffs.withGUI.data());

        m_cmdBuffs.withoutGUI.resize(m_numImages);
        m_vkCore.CreateCommandBuffers(m_numImages, m_cmdBuffs.withoutGUI.data());

        printf("Created command buffers\n");
    }

    void VulkanApp::BeginRendering(VkCommandBuffer CmdBuf, int ImageIndex) {
        VkClearValue ClearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkClearValue DepthValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}};
        m_vkCore.BeginDynamicRendering(CmdBuf, ImageIndex, &ClearColor, &DepthValue);
    }

    void VulkanApp::CreateGeometryPipeline() {
        const std::vector<VkFormat> GBufferFormats = {
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R32_SFLOAT,
            VK_FORMAT_R16G16_SFLOAT,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_R8_UNORM,
        };
        VkFormat DepthFormat = m_vkCore.GetDepthFormat();

        m_pPipeline = new GraphicsPipelineV2(m_device, m_pWindow, nullptr, m_vs, m_fs,
                                             m_numImages, GBufferFormats, DepthFormat);
    }

    void VulkanApp::CreateCompositePipeline() {
        VkFormat swapChainFormat = m_vkCore.GetSwapChainFormat();
        m_pCompositePipeline = new CompositePipeline(&m_vkCore, nullptr, swapChainFormat);
        m_pCompositePipeline->Init(m_vsComposite, m_fsComposite);
    }

    void VulkanApp::CreateViews() {
        for (int i = 0; i < m_numImages; i++) {
            m_posViews[i] = m_gBuffers[i].pos.m_view;
            m_albedoViews[i] = m_gBuffers[i].albedo.m_view;
            m_normalViews[i] = m_gBuffers[i].normal.m_view;
            m_motionViews[i] = m_gBuffers[i].velocity.m_view;
            m_depthViews[i] = m_gBuffers[i].depth.m_view;
            m_metallicViews[i] = m_gBuffers[i].metallic.m_view;
            m_roughnessViews[i] = m_gBuffers[i].roughness.m_view;

            if (m_pRTPipeline) {
                m_rtDiffuseViews[i] =
                        m_pRTPipeline->GetDiffuseRawImageView(i);
                m_rtSpecularViews[i] =
                        m_pRTPipeline->GetSpecularImageView(i);
            }
            if (m_pSVGFPipeline) {
                m_denoiseDiffuseViews[i] =
                        m_pSVGFPipeline->GetDenoisedDiffuse(i).m_view;
                m_denoiseSpecularViews[i] =
                        m_pSVGFPipeline->GetDenoisedSpecular(i).m_view;
            }
        }
    }

    void VulkanApp::UpdatePipelinesDescriptorSets() const {
        if (m_pRTPipeline) {
            m_pRTPipeline->UpdateDescriptorSets(
                m_posViews,
                m_normalViews,
                m_albedoViews,
                m_metallicViews,
                m_roughnessViews);
        }

        if (m_pRTPipeline && m_pSVGFPipeline) {
            m_pSVGFPipeline->UpdateDescriptorSets(
                m_rtDiffuseViews,
                m_rtSpecularViews,
                m_albedoViews,
                m_normalViews,
                m_motionViews,
                m_rtSpecularMotionViews,
                m_depthViews, m_pRTPipeline->GetDiffuseRawSampler()
            );
        }

        if (m_pSVGFPipeline && m_denoiser == true) {
            m_pCompositePipeline->UpdateDescriptorSets(
                m_denoiseDiffuseViews, m_denoiseSpecularViews, m_albedoViews,
                m_metallicViews, m_normalViews, m_posViews,
                m_pSVGFPipeline->GetDenoisedDiffuse(0).m_sampler
            );
        } else if (m_pRTPipeline) {
            m_pCompositePipeline->UpdateDescriptorSets(
                m_rtDiffuseViews, m_rtSpecularViews, m_albedoViews,
                m_metallicViews, m_normalViews, m_posViews,
                m_pRTPipeline->GetDiffuseRawSampler()
            );
        }
    }

    void VulkanApp::ManagePipelinesViews() {
        m_posViews.resize(m_numImages);
        m_albedoViews.resize(m_numImages);
        m_normalViews.resize(m_numImages);
        m_rtDiffuseViews.resize(m_numImages);
        m_rtSpecularViews.resize(m_numImages);
        m_denoiseDiffuseViews.resize(m_numImages);
        m_denoiseSpecularViews.resize(m_numImages);
        m_motionViews.resize(m_numImages);
        m_depthViews.resize(m_numImages);
        m_metallicViews.resize(m_numImages);
        m_roughnessViews.resize(m_numImages);
        m_rtSpecularMotionViews.resize(m_numImages);

        CreateViews();
        UpdatePipelinesDescriptorSets();
    }

    void VulkanApp::CreateGBuffers(int width, int height) {
        m_gBuffers.resize(m_numImages);

        VkFormat posFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkFormat albedoFormat = VK_FORMAT_R8G8B8A8_SRGB;
        VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkFormat depthFormat = VK_FORMAT_R32_SFLOAT;
        VkFormat velocityFormat = VK_FORMAT_R16G16_SFLOAT;
        VkFormat metallicFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkFormat roughnessFormat = VK_FORMAT_R8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        for (int i = 0; i < m_numImages; ++i) {
            m_vkCore.CreateImage(m_gBuffers[i].pos, width, height, posFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].albedo, width, height, albedoFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].normal, width, height, normalFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].depth, width, height, depthFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].velocity, width, height, velocityFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].metallic, width, height, metallicFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);
            m_vkCore.CreateImage(m_gBuffers[i].roughness, width, height, roughnessFormat, usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false, 1);

            m_gBuffers[i].pos.m_view = CreateImageView(m_device, m_gBuffers[i].pos.m_image, posFormat,
                                                       VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].albedo.m_view = CreateImageView(m_device, m_gBuffers[i].albedo.m_image, albedoFormat,
                                                          VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].normal.m_view = CreateImageView(m_device, m_gBuffers[i].normal.m_image, normalFormat,
                                                          VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].depth.m_view = CreateImageView(m_device, m_gBuffers[i].depth.m_image, depthFormat,
                                                         VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].velocity.m_view = CreateImageView(m_device, m_gBuffers[i].velocity.m_image,
                                                            velocityFormat, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].metallic.m_view = CreateImageView(m_device, m_gBuffers[i].metallic.m_image,
                                                            metallicFormat, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
            m_gBuffers[i].roughness.m_view = CreateImageView(m_device, m_gBuffers[i].roughness.m_image,
                                                             roughnessFormat, VK_IMAGE_ASPECT_COLOR_BIT, false, 1);
        }
        printf("G-Buffers Created for %d frames\n", m_numImages);
    }

    void VulkanApp::DefaultCreateCameraPers() {
        float FOV = 90.0f;
        float zNear = 0.1f;
        float zFar = 1000.0f;
        printf("[Camera] DefaultCreateCameraPers called with FOV=%.2f, zNear=%.2f, zFar=%.2f\n", FOV, zNear, zFar);
        DefaultCreateCameraPers(FOV, zNear, zFar);
    }

    void VulkanApp::DefaultCreateCameraPers(float FOV, float zNear, float zFar) {
        if (m_windowWidth == 0 || (m_windowHeight == 0)) {
            printf("Invalid window dims\n");
            exit(1);
        }
        if (m_pGameCamera) {
            printf("Camera already initialized\n");
            exit(1);
        }
        PersProjInfo persProjInfo = {FOV, static_cast<float>(m_windowWidth), (float) m_windowHeight, zNear, zFar};
        m_pGameCamera = new GLMCameraFirstPerson(glm::vec3(24.55, -11.13, -1.13), glm::vec3(0.91, 0.33, 0.26),
                                                 glm::vec3(0, 1, 0),
                                                 persProjInfo);
    }

    void VulkanApp::Key(GLFWwindow *pWindow, int Key, int Scancode, int Action, int Mods) {
        bool Handled = true;
        bool Press = Action != GLFW_RELEASE;
        switch (Key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
                break;
            case GLFW_KEY_T:
                if (Press) {
                    m_denoiser = !m_denoiser;
                    vkDeviceWaitIdle(m_device);
                    UpdatePipelinesDescriptorSets();
                    RecordCommandBuffers();
                }
                break;
            case GLFW_KEY_SPACE:
                if (Press) {
                    m_showImgui = !m_showImgui;
                }
                break;
            case GLFW_KEY_P:
                if (Press && m_pGameCamera) {
                    glm::vec3 pos = m_pGameCamera->GetPosition();
                    glm::vec3 dir = m_pGameCamera->GetTarget();

                    bool fileExists = std::filesystem::exists("camera_positions.csv");
                    std::ofstream camFile("camera_positions.csv", std::ios::app);

                    if (!fileExists) {
                        camFile << "PosX,PosY,PosZ,TargetX,TargetY,TargetZ\n";
                    }

                    camFile << pos.x << "," << pos.y << "," << pos.z << ","
                            << dir.x << "," << dir.y << "," << dir.z << "\n";

                    camFile.close();
                    std::cout << "Saved camera position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
                }
                break;
            default: Handled = false;
        }
        if (!Handled) Handled = GLFWCameraHandler(m_pGameCamera->m_movement, Key, Action, Mods);
    }

    void VulkanApp::MouseMove(GLFWwindow *pWindow, double x, double y) {
        m_pGameCamera->SetMousePos((float) x, (float) y);
    }

    void VulkanApp::MouseButton(GLFWwindow *pWindow, int Button, int Action, int Mods) {
        if (!isMouseControlledByImgui())
            m_pGameCamera->HandleMouseButton(Button, Action, Mods);
    }

    void VulkanApp::createMesh() {
        m_model.Init(&m_vkCore);
        m_model.LoadAssimpModel("../../content/HybridRendering/meshes/sponananazass.obj");
    }

    void VulkanApp::Execute() {
        auto CurTime = static_cast<float>(glfwGetTime());
        int frames = 0;
        float fpsTime = 0.0f;
        while (!glfwWindowShouldClose(m_pWindow)) {
            auto Time = static_cast<float>(glfwGetTime());
            float dt = Time - CurTime;

            if (dt > 0.0f) {
                m_fpsHistory[m_fpsHistoryOffset] = 1.0f / dt;
                m_frameTimeHistory[m_fpsHistoryOffset] = dt * 1000.0f;
                m_fpsHistoryOffset = (m_fpsHistoryOffset + 1) % FPS_HISTORY_SIZE;
            }

            m_pGameCamera->Update(dt);
            UpdateGUI();
            renderScene();

            m_absoluteFrameCount++;

            FrameMetrics metrics{};
            metrics.frameNumber = m_absoluteFrameCount;
            metrics.cpuFrameTimeMs = dt * 1000.0;

            size_t vramUsage = 0, vramBudget = 0;
            m_vkCore.GetVRAMUsage(vramUsage, vramBudget);
            metrics.vramUsageMB = vramUsage / (1024 * 1024);

#ifdef _WIN32
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc),
                                     sizeof(pmc))) {
                metrics.ramUsageMB = pmc.WorkingSetSize / (1024 * 1024);
            }
#endif

            m_frameMetricsHistory.push_back(metrics);

            CurTime = Time;
            glfwPollEvents();
            frames++;
            fpsTime += dt;
            if (fpsTime >= 1.0f) {
                char title[256];

                glm::vec3 pos = m_pGameCamera->GetPosition();
                glm::vec3 dir = m_pGameCamera->GetTarget();

                snprintf(title, sizeof(title), "Vulkan App - FPS: %d | Pos: %.2f %.2f %.2f | Dir: %.2f %.2f %.2f",
                         frames,
                         pos.x, pos.y, pos.z,
                         dir.x, dir.y, dir.z);

                glfwSetWindowTitle(m_pWindow, title);
                frames = 0;
                fpsTime = 0.0f;
            }
        }
        vkDeviceWaitIdle(m_device);
    }
}
