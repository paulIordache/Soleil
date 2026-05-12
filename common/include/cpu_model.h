#pragma once

#include <map>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "material.h"
#include "glm-camera.h"
#include "gl_basic_mesh_entry.h"

class CoreRenderingSystem;

class CoreModel {
public:
    virtual ~CoreModel() = default;

    CoreModel() = default;

    bool LoadAssimpModel(const std::string &Filename);

    [[nodiscard]] const std::vector<GLMCameraFirstPerson> &GetCameras() const { return m_cameras; }

    std::vector<Material> m_Materials;

    std::vector<BasicMeshEntry> m_Meshes;


protected:
    virtual void AllocBuffers() = 0;

    virtual VK::VulkanTexture *AllocTexture2DVulkan() = 0;

    struct Vertex {
        Vector3f Position;
        Vector2f TexCoords;
        Vector3f Normal;
        Vector3f Tangent;
        Vector3f Bitangent;

        void Print() const {
            Position.Print();
            TexCoords.Print();
            Normal.Print();
            Tangent.Print();
            Bitangent.Print();
        }
    };

    virtual void InitGeometryPost() = 0;


    std::vector<u32> m_Indices;

    CoreRenderingSystem *m_pCoreRenderingSystem = nullptr;

private:
    template<typename VertexType>
    void ReserveSpace(std::vector<VertexType> &Vertices, u32 NumVertices, u32 NumIndices);

    template<typename VertexType>
    VertexType ExtractVertex(const aiMesh *paiMesh, unsigned int index);

    template<typename VertexType>
    void InitSingleMesh(std::vector<VertexType> &Vertices, u32 MeshIndex, const aiMesh *paiMesh);

    template<typename VertexType>
    void InitSingleMeshOpt(std::vector<VertexType> &Vertices, u32 MeshIndex, const aiMesh *paiMesh);

    virtual void PopulateBuffers(std::vector<Vertex> &Vertices) = 0;

    u32 CountValidFaces(const aiMesh &Mesh);

    bool InitGeometry(const aiScene *pScene, const std::string &Filename);

    template<typename VertexType>
    void InitGeometryInternal(std::vector<VertexType> &Vertices, int NumVertices, int NumIndices);

    void CountVerticesAndIndices(const aiScene *pScene, u32 &NumVertices, u32 &NumIndices);

    template<typename VertexType>
    void InitAllMeshes(const aiScene *pScene, std::vector<VertexType> &Vertices);

    template<typename VertexType>
    void OptimizeMesh(int MeshIndex, std::vector<u32> &Indices, std::vector<VertexType> &Vertices, std::vector<VertexType> &AllVertices);

    void CalculateMeshTransformations(const aiScene *pScene);

    void TraverseNodeHierarchy(Matrix4f ParentTransformation, aiNode *pNode);

    bool InitMaterials(const aiScene *pScene, const std::string &Filename);

    void LoadTextures(const std::string &Dir, const aiMaterial *pMaterial, int index);

    void LoadMaterialTexture(const std::string &Dir, const aiMaterial *pMaterial, int MaterialIndex, aiTextureType texType, VK::VulkanTexture *&destTexture, VK::VulkanTexture *&defaultTexture, const std::string &defaultTexPath, bool useChannels);

    void LoadTextureEmbedded(const aiTexture *paiTexture, VK::VulkanTexture *&destTexture);

    void LoadTextureFromFile(const std::string &Dir, const aiString &Path, VK::VulkanTexture *&destTexture, bool useChannels);

    void LoadColors(const aiMaterial *pMaterial, int index);

    const aiScene *m_pScene = nullptr;

    Matrix4f m_GlobalInverseTransform;

    Assimp::Importer m_Importer;

    std::vector<GLMCameraFirstPerson> m_cameras;
    float m_textureScale = 1.0f;

    Vector3f m_minPos = Vector3f(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3f m_maxPos = Vector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    VK::VulkanTexture *s_pDefaultDiffuse = nullptr;
    VK::VulkanTexture *s_pDefaultNormal = nullptr;
    VK::VulkanTexture *s_pDefaultMetallic = nullptr;
    VK::VulkanTexture *s_pDefaultRoughness = nullptr;
    VK::VulkanTexture *s_pDefaultOpacity = nullptr;
    VK::VulkanTexture *s_pDefaultEmissive = nullptr;
};