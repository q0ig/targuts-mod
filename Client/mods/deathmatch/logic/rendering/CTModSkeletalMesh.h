#pragma once

#include <string>
#include <vector>
#include <map>
#include <d3d9.h>
#include <d3dx9.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define MAX_BONES_PER_VERTEX 4

struct STModSkeletalVertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
    D3DXVECTOR2 texcoord;
    float       weights[MAX_BONES_PER_VERTEX];
    float       boneIndices[MAX_BONES_PER_VERTEX];
};

struct STModBone
{
    std::string name;
    D3DXMATRIX  offsetMatrix;  // InvBindPose
    D3DXMATRIX  finalMatrix;   // Local to World, updated per frame
    int         parentIndex = -1;
};

// Submesh material subset definition to support characters with multiple textures
// (e.g. Male07 face/head, body/clothes, eyes, and mouth having separate texture maps).
struct STModSkeletalSubset
{
    uint32_t           startIndex = 0;
    uint32_t           indexCount = 0;
    IDirect3DTexture9* texture = nullptr;
    std::string        textureName;
    std::string        materialName;
};

class CTModSkeletalMesh
{
public:
    CTModSkeletalMesh();
    ~CTModSkeletalMesh();

    bool LoadFBX(const std::string& filePath, IDirect3DDevice9* pDevice);
    bool LoadTexture(const std::string& filePath, IDirect3DDevice9* pDevice);
    void Render(IDirect3DDevice9* pDevice);

    std::vector<STModBone>&     GetBones() { return m_bones; }
    std::map<std::string, int>& GetBoneMap() { return m_boneMapping; }

    // Updates hardware skinning matrices
    void UpdateBoneMatrices(IDirect3DDevice9* pDevice, const std::vector<D3DXMATRIX>& boneMatrices);

    const std::vector<STModSkeletalSubset>& GetSubsets() const { return m_subsets; }

private:
    void ProcessNode(aiNode* node, const aiScene* scene, int parentIndex);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void ExtractBoneWeights(std::vector<STModSkeletalVertex>& vertices, aiMesh* mesh, int vertexOffset);

    void               EnsureFallbackTexture(IDirect3DDevice9* pDevice);
    IDirect3DTexture9* GetOrCreateTexture(const std::string& filePath, const std::string& baseDir, IDirect3DDevice9* pDevice);
    std::string        ResolveMaterialTexture(const std::string& matName, const std::string& baseDir, const std::vector<std::string>& textureFiles);

    IDirect3DVertexBuffer9*      m_pVertexBuffer = nullptr;
    IDirect3DIndexBuffer9*       m_pIndexBuffer = nullptr;
    IDirect3DVertexDeclaration9* m_pVertexDecl = nullptr;
    IDirect3DPixelShader9*       m_pPixelShader = nullptr;
    IDirect3DVertexShader9*      m_pVertexShader = nullptr;
    IDirect3DTexture9*           m_pTexture = nullptr;
    IDirect3DTexture9*           m_pFallbackTexture = nullptr;

    std::vector<STModSkeletalSubset>          m_subsets;
    std::map<std::string, IDirect3DTexture9*> m_loadedTextures;

    int m_numVertices = 0;
    int m_numIndices = 0;

    std::vector<STModBone>     m_bones;
    std::map<std::string, int> m_boneMapping;
    int                        m_boneCount = 0;
    std::vector<D3DXMATRIX>    m_currentBoneMatrices;
};
