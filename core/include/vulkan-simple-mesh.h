//
// Created by Paul on 10/3/2025.
//

#ifndef VULKAN_SIMPLE_MESH_H
#define VULKAN_SIMPLE_MESH_H

#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "types.h"
#include "vulkan-core.h"

namespace VK {
    struct SimpleMesh {
        BufferAndMemory m_vb{};
        size_t m_vertexBufferSize = 0;
        VulkanTexture *m_pTex = nullptr;

        void destroy(VkDevice device) {
            m_vb.Destroy(device);

            if (m_pTex) {
                m_pTex->Destroy(device);
                delete m_pTex;
            }
        }
    };
}

#endif //VULKAN_SIMPLE_MESH_H
