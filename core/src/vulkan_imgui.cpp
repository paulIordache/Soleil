
#include "../include/vulkan_imgui.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vulkan-util.h"
#include "../include/vulkan-wrapper.h"

static void CheckVKResult(const VkResult err) {
    if (err == 0) return;

    fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", err);

    if (err < 0) {
        abort();
    }
}

bool IsMouseControlledByImgui() {
    const ImGuiIO &io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

namespace VK {
    void ImGUI::Init(VulkanCore *pvkCore) {
        m_pvkCore = pvkCore;
        m_pvkCore->GetFramebufferSize(m_framebufferWidth, m_framebufferHeight);
        CreateDescriptorPool();
        InitImGui();
    }

    void ImGUI::CreateDescriptorPool() {
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo poolCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1000 * IM_ARRAYSIZE(poolSizes),
            .poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes)),
            .pPoolSizes = poolSizes,
        };

        const VkResult res =
                vkCreateDescriptorPool(m_pvkCore->GetDevice(), &poolCreateInfo, nullptr, &m_descriptorPool);
        CHECK_VK_RESULT(res, "vkCreateDescriptorPool failed");
    }

    void ImGUI::InitImGui() {
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
        io.DisplaySize.x = static_cast<float>(m_framebufferWidth);
        io.DisplaySize.y = static_cast<float>(m_framebufferHeight);

        ImGui::GetStyle().FontScaleMain = 1.5f;

        ImGui::StyleColorsDark();

        constexpr bool InstallGLFWCallbacks = true;
        ImGui_ImplGlfw_InitForVulkan(m_pvkCore->GetWindow(), InstallGLFWCallbacks);

        VkFormat ColorFormat = m_pvkCore->GetSwapChainFormat();

        ImGui_ImplVulkan_InitInfo InitInfo = {
            .ApiVersion = m_pvkCore->GetInstanceVersion(),
            .Instance = m_pvkCore->GetInstance(),
            .PhysicalDevice = m_pvkCore->GetPhysicalDevice().m_physicalDevice,
            .Device = m_pvkCore->GetDevice(),
            .QueueFamily = m_pvkCore->GetQueueFamily(),
            .Queue = m_pvkCore->GetQueue()->GetHandle(),
            .DescriptorPool = m_descriptorPool,
            .RenderPass = nullptr, // assume dynamic rendering
            .MinImageCount = m_pvkCore->GetPhysicalDevice().m_surfaceCaps.minImageCount,
            .ImageCount = static_cast<u32>(m_pvkCore->GetNumImages()),
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .PipelineCache = nullptr,
            .Subpass = 0,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &ColorFormat,
                .depthAttachmentFormat = m_pvkCore->GetDepthFormat(),
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
            },
            .Allocator = nullptr,
            .CheckVkResultFn = CheckVKResult
        };

        ImGui_ImplVulkan_Init(&InitInfo);

        m_cmdBuffs.resize(m_pvkCore->GetNumImages());
        m_pvkCore->CreateCommandBuffers(m_pvkCore->GetNumImages(), m_cmdBuffs.data());
    }

    void ImGUI::Destroy() {
        m_pvkCore->FreeCommandBuffers(static_cast<u32>(m_cmdBuffs.size()), m_cmdBuffs.data());
        vkDestroyDescriptorPool(m_pvkCore->GetDevice(), m_descriptorPool, nullptr);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    VkCommandBuffer ImGUI::PrepareCommandBuffer(int Image) {
        BeginCommandBuffer(m_cmdBuffs[Image], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        m_pvkCore->BeginDynamicRendering(m_cmdBuffs[Image], Image, nullptr, nullptr);

        ImDrawData *pDrawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(pDrawData, m_cmdBuffs[Image]);

        vkCmdEndRendering(m_cmdBuffs[Image]);

        ImageMemBarrier(m_cmdBuffs[Image],
                        m_pvkCore->GetImage(Image),
                        m_pvkCore->GetSwapChainFormat(),
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

        vkEndCommandBuffer(m_cmdBuffs[Image]);
        return m_cmdBuffs[Image];
    }
}

bool isMouseControlledByImgui() {
    const ImGuiIO &io = ImGui::GetIO();
    const bool ret = io.WantCaptureMouse;
    return ret;
}
