#pragma once

#include <vulkan/vulkan.h>
#include "vulkan-core.h"

namespace VK {
    class VulkanPipelineBase {
    protected:
        VulkanCore *m_core = nullptr;
        VkDevice m_device = VK_NULL_HANDLE;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

        void DestroyBaseResources() {
            if (!m_device) return;

            if (m_descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
                m_descriptorPool = VK_NULL_HANDLE;
            }
            if (m_descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
                m_descriptorSetLayout = VK_NULL_HANDLE;
            }
            if (m_pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
                m_pipelineLayout = VK_NULL_HANDLE;
            }
            if (m_pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_device, m_pipeline, nullptr);
                m_pipeline = VK_NULL_HANDLE;
            }
        }

    public:
        VulkanPipelineBase(VkDevice device, VulkanCore *core = nullptr)
            : m_core(core), m_device(device) {
        }

        virtual ~VulkanPipelineBase() = default;

        virtual void Destroy() = 0;

        VkPipeline GetPipeline() const { return m_pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
    };
}
