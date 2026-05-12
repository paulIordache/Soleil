#include "cpu_model.h"
#include "meshoptimizer.h"

using namespace std;

static bool UseMeshOptimizer = false;

static void traverse(int depth, aiNode *pNode);

bool CoreModel::LoadAssimpModel(const string &Filename) {
    bool Ret = false;
    m_pScene = m_Importer.ReadFile(Filename.c_str(), ASSIMP_LOAD_FLAGS);

    if (m_pScene) {
        printf("--- START Node Hierarchy ---\n");
        traverse(0, m_pScene->mRootNode);
        printf("--- END Node Hierarchy ---\n");
        m_GlobalInverseTransform = m_pScene->mRootNode->mTransformation;
        m_GlobalInverseTransform = m_GlobalInverseTransform.Inverse();
        Ret = InitGeometry(m_pScene, Filename);
    } else {
        printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
    }

    return Ret;
}

bool CoreModel::InitGeometry(const aiScene *pScene, const string &Filename) {
    printf("\n*** Initializing geometry ***\n");
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    CountVerticesAndIndices(pScene, NumVertices, NumIndices);
    std::vector<Vertex> Vertices;
    InitGeometryInternal<Vertex>(Vertices, NumVertices, NumIndices);
    PopulateBuffers(Vertices);
    InitMaterials(pScene, Filename);
    CalculateMeshTransformations(pScene);

    return true;
}

template<typename VertexType>
void CoreModel::InitGeometryInternal(std::vector<VertexType> &Vertices, int NumVertices, int NumIndices) {
    ReserveSpace<VertexType>(Vertices, NumVertices, NumIndices);
    InitAllMeshes<VertexType>(m_pScene, Vertices);

    printf("Min pos: ");
    m_minPos.Print();
    printf("Max pos: ");
    m_maxPos.Print();
}

void CoreModel::CountVerticesAndIndices(const aiScene *pScene, unsigned int &NumVertices, unsigned int &NumIndices) {
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].ValidFaces = CountValidFaces(*pScene->mMeshes[i]);
        m_Meshes[i].NumIndices = m_Meshes[i].ValidFaces * 3;
        m_Meshes[i].NumVertices = pScene->mMeshes[i]->mNumVertices;
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices += m_Meshes[i].NumIndices;
    }
}

u32 CoreModel::CountValidFaces(const aiMesh &Mesh) {
    u32 NumValidFaces = 0;
    for (u32 i = 0; i < Mesh.mNumFaces; i++) {
        if (Mesh.mFaces[i].mNumIndices == 3) {
            NumValidFaces++;
        }
    }
    return NumValidFaces;
}

template<typename VertexType>
void CoreModel::ReserveSpace(std::vector<VertexType> &Vertices, unsigned int NumVertices, unsigned int NumIndices) {
    Vertices.reserve(NumVertices);
    m_Indices.reserve(NumIndices);
}

template<typename VertexType>
void CoreModel::InitAllMeshes(const aiScene *pScene, std::vector<VertexType> &Vertices) {
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        const aiMesh *paiMesh = pScene->mMeshes[i];
        if (UseMeshOptimizer) {
            InitSingleMeshOpt<VertexType>(Vertices, i, paiMesh);
        } else {
            InitSingleMesh<VertexType>(Vertices, i, paiMesh);
        }
    }
}

void CoreModel::CalculateMeshTransformations(const aiScene *pScene) {
    printf("----------------------------------------\n");
    printf("Calculating mesh transformations\n");
    Matrix4f Transformation;
    Transformation.InitIdentity();
    TraverseNodeHierarchy(Transformation, pScene->mRootNode);
}

void CoreModel::TraverseNodeHierarchy(Matrix4f ParentTransformation, aiNode *pNode) {
    printf("Traversing node '%s'\n", pNode->mName.C_Str());
    Matrix4f NodeTransformation(pNode->mTransformation);
    Matrix4f CombinedTransformation = ParentTransformation * NodeTransformation;

    printf("Combined transformation:\n");
    CombinedTransformation.Print();

    if (pNode->mNumMeshes > 0) {
        printf("Num meshes: %d - ", pNode->mNumMeshes);
        for (int i = 0; i < (int) pNode->mNumMeshes; i++) {
            int MeshIndex = pNode->mMeshes[i];
            printf("%d ", MeshIndex);
            m_Meshes[MeshIndex].Transformation = CombinedTransformation;
        }
        printf("\n");
    } else {
        printf("No meshes\n");
    }

    for (u32 i = 0; i < pNode->mNumChildren; i++) {
        TraverseNodeHierarchy(CombinedTransformation, pNode->mChildren[i]);
    }
}

template<typename VertexType>
VertexType CoreModel::ExtractVertex(const aiMesh *paiMesh, unsigned int index) {
    VertexType v;
    const aiVector3D &Pos = paiMesh->mVertices[index];
    v.Position = Vector3f(Pos.x, Pos.y, Pos.z);

    m_minPos.x = std::min(m_minPos.x, v.Position.x);
    m_minPos.y = std::min(m_minPos.y, v.Position.y);
    m_minPos.z = std::min(m_minPos.z, v.Position.z);

    m_maxPos.x = std::max(m_maxPos.x, v.Position.x);
    m_maxPos.y = std::max(m_maxPos.y, v.Position.y);
    m_maxPos.z = std::max(m_maxPos.z, v.Position.z);

    if (paiMesh->mNormals) {
        v.Normal = Vector3f(paiMesh->mNormals[index].x, paiMesh->mNormals[index].y, paiMesh->mNormals[index].z);
    } else {
        v.Normal = Vector3f(0.0f, 1.0f, 0.0f);
    }

    if (paiMesh->HasTextureCoords(0)) {
        v.TexCoords = Vector2f(paiMesh->mTextureCoords[0][index].x, paiMesh->mTextureCoords[0][index].y);
        v.Tangent = Vector3f(paiMesh->mTangents[index].x, paiMesh->mTangents[index].y, paiMesh->mTangents[index].z);
        v.Bitangent = Vector3f(paiMesh->mBitangents[index].x, paiMesh->mBitangents[index].y, paiMesh->mBitangents[index].z);
    } else {
        v.TexCoords = Vector2f(0.0f);
        v.Tangent = Vector3f(0.0f);
        v.Bitangent = Vector3f(0.0f);
    }

    return v;
}

template<typename VertexType>
void CoreModel::InitSingleMesh(vector<VertexType> &Vertices, u32 MeshIndex, const aiMesh *paiMesh) {
    printf("Mesh %d %s\n", MeshIndex, paiMesh->mName.C_Str());

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        Vertices.push_back(ExtractVertex<VertexType>(paiMesh, i));
    }

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace &Face = paiMesh->mFaces[i];
        if (Face.mNumIndices != 3) {
            printf("Warning! face %d has %d indices\n", i, Face.mNumIndices);
            continue;
        }
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }
}

template<typename VertexType>
void CoreModel::InitSingleMeshOpt(vector<VertexType> &AllVertices, u32 MeshIndex, const aiMesh *paiMesh) {
    std::vector<VertexType> Vertices(paiMesh->mNumVertices);

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        Vertices[i] = ExtractVertex<VertexType>(paiMesh, i);
    }

    m_Meshes[MeshIndex].BaseVertex = (u32) AllVertices.size();
    m_Meshes[MeshIndex].BaseIndex = (u32) m_Indices.size();

    int NumIndices = paiMesh->mNumFaces * 3;
    std::vector<u32> Indices(NumIndices);

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace &Face = paiMesh->mFaces[i];
        if (Face.mNumIndices != 3) {
            printf("Warning! face %d has %d indices\n", i, Face.mNumIndices);
            continue;
        }
        Indices[i * 3 + 0] = Face.mIndices[0];
        Indices[i * 3 + 1] = Face.mIndices[1];
        Indices[i * 3 + 2] = Face.mIndices[2];
    }

    OptimizeMesh(MeshIndex, Indices, Vertices, AllVertices);
}

template<typename VertexType>
void CoreModel::OptimizeMesh(int MeshIndex, std::vector<u32> &Indices, std::vector<VertexType> &Vertices, std::vector<VertexType> &AllVertices) {
    size_t NumIndices = Indices.size();
    size_t NumVertices = Vertices.size();

    std::vector<unsigned int> remap(NumIndices);
    size_t OptVertexCount = meshopt_generateVertexRemap(remap.data(), Indices.data(), NumIndices, Vertices.data(), NumVertices, sizeof(VertexType));

    std::vector<u32> OptIndices(NumIndices);
    std::vector<VertexType> OptVertices(OptVertexCount);

    meshopt_remapIndexBuffer(OptIndices.data(), Indices.data(), NumIndices, remap.data());
    meshopt_remapVertexBuffer(OptVertices.data(), Vertices.data(), NumVertices, sizeof(VertexType), remap.data());
    meshopt_optimizeVertexCache(OptIndices.data(), OptIndices.data(), NumIndices, OptVertexCount);
    meshopt_optimizeOverdraw(OptIndices.data(), OptIndices.data(), NumIndices, &(OptVertices[0].Position.x), OptVertexCount, sizeof(VertexType), 1.05f);
    meshopt_optimizeVertexFetch(OptVertices.data(), OptIndices.data(), NumIndices, OptVertices.data(), OptVertexCount, sizeof(VertexType));

    float Threshold = 1.0f;
    size_t TargetIndexCount = (size_t) (NumIndices * Threshold);
    float TargetError = 0.0f;
    std::vector<unsigned int> SimplifiedIndices(OptIndices.size());
    size_t OptIndexCount = meshopt_simplify(SimplifiedIndices.data(), OptIndices.data(), NumIndices, &OptVertices[0].Position.x, OptVertexCount, sizeof(VertexType), TargetIndexCount, TargetError);

    static int num_indices = 0;
    num_indices += (int) NumIndices;
    static int opt_indices = 0;
    opt_indices += (int) OptIndexCount;

    printf("Num indices %d\n", num_indices);
    printf("Optimized number of indices %d\n", opt_indices);

    SimplifiedIndices.resize(OptIndexCount);
    m_Indices.insert(m_Indices.end(), SimplifiedIndices.begin(), SimplifiedIndices.end());
    AllVertices.insert(AllVertices.end(), OptVertices.begin(), OptVertices.end());
    m_Meshes[MeshIndex].NumIndices = (u32) OptIndexCount;
}

bool CoreModel::InitMaterials(const aiScene *pScene, const string &Filename) {
    const string Dir = GetDirFromFilename(Filename);
    const bool Ret = true;

    printf("Num materials: %d\n", pScene->mNumMaterials);

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial *pMaterial = pScene->mMaterials[i];
        printf("Loading material %d: '%s'\n", i, pMaterial->GetName().C_Str());
        LoadTextures(Dir, pMaterial, i);
        LoadColors(pMaterial, i);
    }

    return Ret;
}

static std::string TextureTypeToString(aiTextureType type) {
    static std::map<aiTextureType, std::string> textureTypeNames = {
        {aiTextureType_DIFFUSE, "Diffuse"},
        {aiTextureType_SPECULAR, "Specular"},
        {aiTextureType_AMBIENT, "Ambient"},
        {aiTextureType_EMISSIVE, "Emissive"},
        {aiTextureType_HEIGHT, "Height"},
        {aiTextureType_NORMALS, "Normals"},
        {aiTextureType_SHININESS, "Shininess"},
        {aiTextureType_OPACITY, "Opacity"},
        {aiTextureType_DISPLACEMENT, "Displacement"},
        {aiTextureType_LIGHTMAP, "Lightmap"},
        {aiTextureType_REFLECTION, "Reflection"},
        {aiTextureType_UNKNOWN, "Unknown"},
        {aiTextureType_METALNESS, "Metal"},
        {aiTextureType_DIFFUSE_ROUGHNESS, "Roughness"}
    };

    auto it = textureTypeNames.find(type);
    if (it != textureTypeNames.end()) {
        return it->second;
    }
    return "Invalid Type";
}

static int GetTextureCount(const aiMaterial *pMaterial) {
    int TextureCount = 0;
    for (int i = 0; i <= aiTextureType_UNKNOWN; ++i) {
        aiTextureType ttype = (aiTextureType) (i);
        int Count = pMaterial->GetTextureCount(ttype);
        TextureCount += Count;
        if (Count > 0) {
            printf("Found texture %s\n", TextureTypeToString(ttype).c_str());
        }
    }
    return TextureCount;
}

void CoreModel::LoadTextures(const string &Dir, const aiMaterial *pMaterial, const int index) {
    const int TextureCount = GetTextureCount(pMaterial);
    printf("Number of textures %d\n", TextureCount);

    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_OPACITY, m_Materials[index].pOpacityVulkan, s_pDefaultOpacity, "../../content/textures/white.png", true);
    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_METALNESS, m_Materials[index].pMetallicVulkan, s_pDefaultMetallic, "../../content/textures/black.png", true);
    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_DIFFUSE_ROUGHNESS, m_Materials[index].pRoughnessVulkan, s_pDefaultRoughness, "../../content/textures/white.png", true);
    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_DIFFUSE, m_Materials[index].pDiffuseVulkan, s_pDefaultDiffuse, "../../content/textures/white.png", false);
    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_EMISSIVE, m_Materials[index].pEmissiveVulkan, s_pDefaultEmissive, "../../content/textures/black.png", false);
    LoadMaterialTexture(Dir, pMaterial, index, aiTextureType_HEIGHT, m_Materials[index].pNormalVulkan, s_pDefaultNormal, "../../content/textures/normal.png", false);
}

void CoreModel::LoadMaterialTexture(const string &Dir, const aiMaterial *pMaterial, int MaterialIndex, aiTextureType texType, VK::VulkanTexture *&destTexture, VK::VulkanTexture *&defaultTexture, const string &defaultTexPath, bool useChannels) {
    destTexture = nullptr;
    if (pMaterial->GetTextureCount(texType) > 0) {
        aiString Path;
        if (pMaterial->GetTexture(texType, 0, &Path, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS) {
            if (const aiTexture *paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str())) {
                LoadTextureEmbedded(paiTexture, destTexture);
            } else {
                LoadTextureFromFile(Dir, Path, destTexture, useChannels);
            }
        }
    } else {
        printf("Warning! no texture of type %d\n", texType);
        if (!defaultTexture) {
            printf("Loading default texture\n");
            defaultTexture = AllocTexture2DVulkan();
            if (useChannels) {
                defaultTexture->Load(defaultTexPath.c_str(), 1);
            } else {
                defaultTexture->LoadFromDisk(defaultTexPath.c_str());
            }
        }
        destTexture = defaultTexture;
    }
}

void CoreModel::LoadTextureEmbedded(const aiTexture *paiTexture, VK::VulkanTexture *&destTexture) {
    printf("Embedded texture type '%s'\n", paiTexture->achFormatHint);
    destTexture = AllocTexture2DVulkan();
    destTexture->Load(paiTexture->mWidth, paiTexture->pcData);
}

void CoreModel::LoadTextureFromFile(const string &Dir, const aiString &Path, VK::VulkanTexture *&destTexture, bool useChannels) {
    string p(Path.data);

    for (char &c : p) {
        if (c == '\\') {
            c = '/';
        }
    }

    if (p == "C://") {
        p = "";
    } else if (p.length() >= 2 && p.substr(0, 2) == "./") {
        p = p.substr(2);
    }

    string FullPath = Dir + "/" + p;
    destTexture = AllocTexture2DVulkan();

    if (useChannels) {
        destTexture->Load(FullPath.c_str(), 1);
    } else {
        destTexture->LoadFromDisk(FullPath.c_str());
    }
    printf("Loaded texture '%s'\n", FullPath.c_str());
}

void CoreModel::LoadColors(const aiMaterial *pMaterial, int index) {
    Material &material = m_Materials[index];
    material.m_name = pMaterial->GetName().C_Str();
    printf("Material Loaded: %s\n", material.m_name.c_str());

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        printf("  Shading Model: %d\n", ShadingModel);
    }

    aiColor4D color;
    float floatVal;

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
        printf("  Loaded Ka: [%f %f %f]\n", color.r, color.g, color.b);
        material.AmbientColor = Vector3f(color.r, color.g, color.b);
    } else {
        material.AmbientColor = Vector3f(0.0f, 0.0f, 0.0f);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        printf("  Loaded Kd: [%f %f %f]\n", color.r, color.g, color.b);
        material.DiffuseColor = Vector3f(color.r, color.g, color.b);
    } else {
        material.DiffuseColor = Vector3f(0.0f, 0.0f, 0.0f);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        printf("  Loaded Ks: [%f %f %f]\n", color.r, color.g, color.b);
        material.SpecularColor = Vector3f(color.r, color.g, color.b);
    } else {
        material.SpecularColor = Vector3f(0.0f, 0.0f, 0.0f);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
        printf("  Loaded Ke: [%f %f %f]\n", color.r, color.g, color.b);
        material.EmissiveColor = Vector3f(color.r, color.g, color.b);
    } else {
        material.EmissiveColor = Vector3f(0.0f, 0.0f, 0.0f);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == AI_SUCCESS) {
        printf("  Loaded Tf: [%f %f %f]\n", color.r, color.g, color.b);
        material.Transmission = Vector3f(color.r, color.g, color.b);
    } else {
        material.Transmission = Vector3f(1.0f, 1.0f, 1.0f);
    }

    if (pMaterial->Get(AI_MATKEY_SHININESS, floatVal) == AI_SUCCESS) {
        printf("  Loaded Ns: %f\n", floatVal);
        material.Ns = floatVal;
    } else {
        material.Ns = 0.0f;
    }

    if (pMaterial->Get(AI_MATKEY_REFRACTI, floatVal) == AI_SUCCESS) {
        printf("  Loaded Ni: %f\n", floatVal);
        material.Ni = floatVal;
    } else {
        material.Ni = 1.0f;
    }

    if (pMaterial->Get(AI_MATKEY_OPACITY, floatVal) == AI_SUCCESS) {
        printf("  Loaded d : %f\n", floatVal);
        material.Opacity = floatVal;
    } else {
        material.Opacity = 1.0f;
    }
}

static void traverse(int depth, aiNode *pNode) {
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }
    printf("%s\n", pNode->mName.C_Str());
    Matrix4f NodeTransformation(pNode->mTransformation);
    NodeTransformation.Print();

    for (u32 i = 0; i < pNode->mNumChildren; i++) {
        traverse(depth + 1, pNode->mChildren[i]);
    }
}