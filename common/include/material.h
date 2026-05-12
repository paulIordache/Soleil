#ifndef MATERIAL_H
#define MATERIAL_H
#include "math-3d.h"
#include "vulkan_texture.h"

class Material {
public:
    std::string m_name;

    Vector3f AmbientColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f DiffuseColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f SpecularColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f EmissiveColor = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f Transmission = Vector3f(1.0f, 1.0f, 1.0f);

    float Ns = 0.0f;
    float Ni = 1.0f;
    float Opacity = 1.0f;
    float m_alphaTest = 0.0f;


    VK::VulkanTexture *pDiffuseVulkan = nullptr;
    VK::VulkanTexture *pNormalVulkan = nullptr;
    VK::VulkanTexture *pSpecularExponentVulkan = nullptr;
    VK::VulkanTexture *pEmissiveVulkan = nullptr;
    VK::VulkanTexture *pRoughnessVulkan = nullptr;
    VK::VulkanTexture *pMetallicVulkan = nullptr;
    VK::VulkanTexture *pAmbientVulkan = nullptr;
    VK::VulkanTexture *pOpacityVulkan = nullptr;

    ~Material() {
        // if (pDiffuseVulkan) {
        //     delete pDiffuseVulkan;
        // }
        //
        // if (pSpecularExponentVulkan) {
        //     delete pSpecularExponentVulkan;
        // }
        //
        // if (pNormalVulkan) {
        //     delete pNormalVulkan;
        // }
        //
        // if (pAmbientVulkan) {
        //     delete pAmbientVulkan;
        // }
        //
        // if (pEmissiveVulkan) {
        //     delete pEmissiveVulkan;
        // }
        //
        // if (pRoughnessVulkan) {
        //     delete pRoughnessVulkan;
        // }
        //
        // if (pOpacityVulkan) {
        //     delete pOpacityVulkan;
        // }
        //
        // if (pMetallicVulkan) {
        //     delete pMetallicVulkan;
        // }
    }
};


#endif
