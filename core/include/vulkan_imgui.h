

#ifndef VULKAN_IMGUI_H
#define VULKAN_IMGUI_H
#include "vulkan-core.h"


namespace VK {
    class ImGUI {
    public:
        ImGUI() = default;
        ~ImGUI() = default;

        void Init(VulkanCore *pvkCore);
        void Destroy();

        VkCommandBuffer PrepareCommandBuffer(int Image);
    private:
        void CreateDescriptorPool();

        void InitImGui();

        VulkanCore *m_pvkCore = nullptr;
        int m_framebufferWidth = 0;
        int m_framebufferHeight = 0;
        std::vector<VkCommandBuffer> m_cmdBuffs;
        VkDescriptorPool m_descriptorPool = nullptr;
    };
}

bool isMouseControlledByImgui();

#endif //VULKAN_IMGUI_H
