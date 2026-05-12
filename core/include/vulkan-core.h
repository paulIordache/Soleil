/*

		Copyright 2024 Etay Meiri

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <vector>

#include <GLFW/glfw3.h>

#include "vulkan-device.h"
#include "vulkan-queue.h"
#include "vulkan_texture.h"


namespace VK {
    class BufferAndMemory {
    public:
        BufferAndMemory() = default;

        VkBuffer m_buffer = NULL;
        VkDeviceMemory m_mem = NULL;
        VkDeviceSize m_allocationSize = 0;

        void Update(VkDevice Device, const void *pData, size_t Size);

        void Destroy(VkDevice Device);
    };

    class VulkanCore {
    public:
        VulkanCore();

        ~VulkanCore();

        void Init(const char *pAppName, GLFWwindow *pWindow, bool DepthEnabled);

        VkRenderPass CreateSimpleRenderPass();

        std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass RenderPass) const;

        void BeginDynamicRendering(VkCommandBuffer CmdBuf, int ImageIndex, VkClearValue *pClearColor,
                                   VkClearValue *pDepthValue);

        void DestroyFramebuffers(std::vector<VkFramebuffer> &Framebuffers);

        VkDevice &GetDevice() { return m_device; }

        int GetNumImages() const { return (int) m_images.size(); }

        const VkImage &GetImage(int Index) const;

        VulkanQueue *GetQueue() { return &m_queue; }

        u32 GetQueueFamily() const { return m_queueFamily; }

        void CreateCommandBuffers(u32 Count, VkCommandBuffer *pCmdBufs);

        void FreeCommandBuffers(u32 Count, const VkCommandBuffer *pCmdBufs);

        BufferAndMemory UploadDataToGPU(const void *pVertices, size_t Size);

        std::vector<BufferAndMemory> CreateUniformBuffers(size_t Size);

        void CreateDepthResources();

        void CreateTexture(const char *filename, VulkanTexture &Tex);

        void CreateImageFromData(VulkanTexture &tex, const void *p_pixels, int image_width, int image_height,
                                 VkFormat format,
                                 bool is_cubemap, uint32_t mipLevels);

        void CreateTextureDynamically(const char *pFilename, VulkanTexture &xTex, int requiredChannels = 4);

        void Create2DTextureFromData(const void *pPixels, int ImageWidth, int ImageHeight, VulkanTexture &Tex);

        void CreateTextureFromData(const void *pPixels, int ImageWidth, int ImageHeight, VkFormat Format,
                                   bool IsCubemap, VulkanTexture &Tex);

        void GetFramebufferSize(int &Width, int &Height) const;

        VkFormat GetSwapChainFormat() const { return m_swapChainSurfaceFormat.format; }

        VkFormat GetDepthFormat() const { return m_physDevices.selected().m_depthFormat; }

        const VkImageView &GetImageView(int Index) const;

        const VkImageView &GetDepthView(int Index) const;

        const VkPhysicalDeviceLimits &GetPhysicalDeviceLimits() const;

        u32 GetInstanceVersion() const;

        GLFWwindow *GetWindow() const { return m_pWindow; }

        VkInstance GetInstance() const { return m_instance; }

        const PhysicalDevice &GetPhysicalDevice() { return m_physDevices.selected(); }

        void CreateCubemapTexture(const char *pFilename, VulkanTexture &Tex);

        VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer);

        BufferAndMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

        void GetVRAMUsage(size_t& usage, size_t& budget) const;

        struct rtExtensions {
            PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
            PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
            PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
            PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
            PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;

            // NEW: RT pipeline + SBT
            PFN_vkCreateRayTracingPipelinesKHR        vkCreateRayTracingPipelinesKHR;
            PFN_vkGetRayTracingShaderGroupHandlesKHR  vkGetRayTracingShaderGroupHandlesKHR;
            PFN_vkCmdTraceRaysKHR                     vkCmdTraceRaysKHR;
        } rtExtensions;

        VkCommandBuffer CreateAndBeginSingleUseCommand();

        void EndSingleTimeCommands(VkCommandBuffer CmdBuf);

        void CreateImage(VulkanTexture &Tex, u32 ImageWidth, u32 ImageHeight, VkFormat TexFormat,
                         VkImageUsageFlags UsageFlags, VkMemoryPropertyFlagBits PropertyFlags, bool IsCubemap, uint32_t mipLevels);

        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight,
                             uint32_t mipLevels,
                             int layerCount);

        void TransitionImageLayout(VkImage &Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout,
                                   int LayerCount);

    	void CmdBeginLabel(VkCommandBuffer cmd, const char *name, float r, float g, float b);

    	void CmdEndLabel(VkCommandBuffer cmd);

private:
        void CreateTextureImageFromData(VulkanTexture &Tex, const void *pPixels, u32 ImageWidth, u32 ImageHeight,
                                        VkFormat TexFormat);

        void CreateInstance(const char *pAppName);

        void CreateDebugCallback();

        void CreateSurface();

        void UpdateInstanceVersion();

        void CreateDevice();

        void CreateSwapChain();

        void CreateCommandBufferPool();

        BufferAndMemory CreateUniformBuffer(size_t Size);

        u32 GetMemoryTypeIndex(u32 memTypeBits, VkMemoryPropertyFlags memPropFlags);

        void CopyBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);


        void UpdateTextureImage(VulkanTexture &Tex, u32 ImageWidth, u32 ImageHeight, VkFormat TexFormat,
                                int LayerCount, const void *pPixels, bool IsCubemap, uint32_t mipLevels);

        void SubmitCopyCommand();

        void CopyBufferToImage(VkImage Dst, VkBuffer Src, u32 ImageWidth, u32 ImageHeight, VkDeviceSize LayerSize,
                               int LayerCount);

        void CopyBufferToBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        GLFWwindow *m_pWindow = NULL;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VulkanPhysicalDevice m_physDevices;
        u32 m_queueFamily = 0;
        VkDevice m_device{};
        VkSurfaceFormatKHR m_swapChainSurfaceFormat{};
        VkSwapchainKHR m_swapChain{};
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
        std::vector<VulkanTexture> m_depthImages;
        VkCommandPool m_cmdBufPool{};
        VulkanQueue m_queue;
        VkCommandBuffer m_copyCmdBuf{};
        int m_windowWidth = 1920;
        int m_windowHeight = 1080;
        bool m_depthEnabled = false;

        struct {
            int Major = 0;
            int Minor = 0;
            int Patch = 0;
        } m_instanceVersion;
    };
}