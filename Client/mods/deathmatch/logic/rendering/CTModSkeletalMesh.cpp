#include "StdInc.h"
#include "CTModSkeletalMesh.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>

#pragma comment(lib, "d3dx9.lib")

static const char* g_SkinningHLSL = R"(
#pragma pack_matrix(row_major)

float4x4 ViewProjection : register(c0);
float4x4 World          : register(c4);
float4x4 BoneMatrices[60] : register(c8); // Max 60 bones (240 registers)

struct VS_INPUT {
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 Weights  : BLENDWEIGHT;
    float4 Indices  : BLENDINDICES;
};

struct VS_OUTPUT {
    float4 Position : POSITION;
    float3 Normal   : TEXCOORD1;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT mainVS(VS_INPUT input) {
    VS_OUTPUT output;
    
    int index0 = (int)input.Indices.x;
    int index1 = (int)input.Indices.y;
    int index2 = (int)input.Indices.z;
    int index3 = (int)input.Indices.w;
    
    float4x4 boneTransform = 
        BoneMatrices[index0] * input.Weights.x +
        BoneMatrices[index1] * input.Weights.y +
        BoneMatrices[index2] * input.Weights.z +
        BoneMatrices[index3] * input.Weights.w;
        
    float4 localPos = mul(float4(input.Position, 1.0f), boneTransform);
    float3 localNormal = mul(input.Normal, (float3x3)boneTransform);
    
    float4 worldPos = mul(localPos, World);
    output.Position = mul(worldPos, ViewProjection);
    output.Normal = normalize(mul(localNormal, (float3x3)World));
    output.TexCoord = input.TexCoord;
    
    return output;
}

sampler2D DiffuseTexture : register(s0);

float4 mainPS(VS_OUTPUT input) : COLOR {
    // Directional sunlight from sky (+Z in GTA SA) + ambient light
    float3 lightDir = normalize(float3(-0.3, 0.4, -1.0));
    float ndotl = max(0.40f, dot(input.Normal, -lightDir));
    float4 texColor = tex2D(DiffuseTexture, input.TexCoord);
    return float4(texColor.rgb * ndotl, 1.0f);
}
)";

CTModSkeletalMesh::CTModSkeletalMesh()
{
}

CTModSkeletalMesh::~CTModSkeletalMesh()
{
    if (m_pVertexBuffer)
        m_pVertexBuffer->Release();
    if (m_pIndexBuffer)
        m_pIndexBuffer->Release();
    if (m_pVertexDecl)
        m_pVertexDecl->Release();
    if (m_pVertexShader)
        m_pVertexShader->Release();
    if (m_pPixelShader)
        m_pPixelShader->Release();

    // Release textures safely without double-releasing textures shared across subsets or fallback
    std::set<IDirect3DTexture9*> released;
    if (m_pTexture)
    {
        m_pTexture->Release();
        released.insert(m_pTexture);
        m_pTexture = nullptr;
    }
    if (m_pFallbackTexture)
    {
        m_pFallbackTexture->Release();
        released.insert(m_pFallbackTexture);
        m_pFallbackTexture = nullptr;
    }
    for (auto& pair : m_loadedTextures)
    {
        if (pair.second && released.find(pair.second) == released.end())
        {
            pair.second->Release();
            released.insert(pair.second);
        }
    }
    m_loadedTextures.clear();
    m_subsets.clear();
}

void CTModSkeletalMesh::EnsureFallbackTexture(IDirect3DDevice9* pDevice)
{
    if (m_pFallbackTexture || !pDevice)
        return;
    if (SUCCEEDED(pDevice->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pFallbackTexture, nullptr)))
    {
        D3DLOCKED_RECT rect;
        if (SUCCEEDED(m_pFallbackTexture->LockRect(0, &rect, nullptr, 0)))
        {
            *(DWORD*)rect.pBits = 0xFFFFFFFF;  // Solid white fallback
            m_pFallbackTexture->UnlockRect(0);
        }
    }
}

static D3DXMATRIX ConvertAssimpMatrix(const aiMatrix4x4& aiMat)
{
    D3DXMATRIX m;
    m._11 = aiMat.a1;
    m._12 = aiMat.b1;
    m._13 = aiMat.c1;
    m._14 = aiMat.d1;
    m._21 = aiMat.a2;
    m._22 = aiMat.b2;
    m._23 = aiMat.c2;
    m._24 = aiMat.d2;
    m._31 = aiMat.a3;
    m._32 = aiMat.b3;
    m._33 = aiMat.c3;
    m._34 = aiMat.d3;
    m._41 = aiMat.a4;
    m._42 = aiMat.b4;
    m._43 = aiMat.c4;
    m._44 = aiMat.d4;
    return m;
}

static std::string GetTMODBinPath()
{
    char    path[MAX_PATH];
    HMODULE hModule = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetTMODBinPath, &hModule);
    GetModuleFileNameA(hModule, path, sizeof(path));
    std::string strPath = path;
    size_t      pos = strPath.find("mods\\deathmatch");
    if (pos != std::string::npos)
        return strPath.substr(0, pos);

    pos = strPath.find_last_of("\\/");
    if (pos != std::string::npos)
        return strPath.substr(0, pos + 1);
    return "";
}

static std::vector<std::string> ListDirectoryFiles(const std::string& dirPath)
{
    std::vector<std::string> result;
    std::string              searchMask = dirPath + "/*.*";
    WIN32_FIND_DATAA         fd;
    HANDLE                   hFind = FindFirstFileA(searchMask.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                result.push_back(dirPath + "/" + fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
    return result;
}

static std::string ToLower(const std::string& str)
{
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Resolves a character material name (e.g. mike_facemap, citizen_sheet, eyeball_r, mouth)
// to a concrete texture file discovered in the asset's texture directories.
// This ensures characters like Male07 that have separate face, clothes, eye, and mouth textures
// don't get the wrong clothes texture pasted all over their face.
std::string CTModSkeletalMesh::ResolveMaterialTexture(const std::string& matName, const std::string& baseDir, const std::vector<std::string>& textureFiles)
{
    std::string lowerMat = ToLower(matName);

    // 1. Direct match by filename stem or full bare filename
    for (const auto& file : textureFiles)
    {
        std::string lowerFile = ToLower(file);
        size_t      slash = lowerFile.find_last_of("/\\");
        std::string bare = (slash != std::string::npos) ? lowerFile.substr(slash + 1) : lowerFile;
        size_t      dot = bare.rfind('.');
        std::string stem = (dot != std::string::npos) ? bare.substr(0, dot) : bare;
        if (stem == lowerMat || bare == lowerMat)
            return file;
    }

    // 2. Semantic keyword matching:
    // Face / Head (e.g. mike_facemap -> cara_del_serote_ciete.png)
    if (lowerMat.find("face") != std::string::npos || lowerMat.find("head") != std::string::npos || lowerMat.find("cara") != std::string::npos ||
        lowerMat.find("facemap") != std::string::npos)
    {
        for (const auto& file : textureFiles)
        {
            std::string lf = ToLower(file);
            if (lf.find("cara") != std::string::npos || lf.find("face") != std::string::npos || lf.find("head") != std::string::npos)
                return file;
        }
    }

    // Clothes / Body / Citizen sheet (e.g. citizen_sheet -> ropa_del_email_serote_siete.png)
    if (lowerMat.find("cloth") != std::string::npos || lowerMat.find("body") != std::string::npos || lowerMat.find("ropa") != std::string::npos ||
        lowerMat.find("sheet") != std::string::npos || lowerMat.find("citizen") != std::string::npos)
    {
        for (const auto& file : textureFiles)
        {
            std::string lf = ToLower(file);
            if (lf.find("ropa") != std::string::npos || lf.find("cloth") != std::string::npos || lf.find("body") != std::string::npos ||
                lf.find("sheet") != std::string::npos)
                return file;
        }
    }

    // Eyes (e.g. eyeball_l / eyeball_r -> eyeball_r_baseColor.png)
    if (lowerMat.find("eye") != std::string::npos)
    {
        for (const auto& file : textureFiles)
        {
            std::string lf = ToLower(file);
            if (lf.find("eye") != std::string::npos)
                return file;
        }
    }

    // Mouth / Teeth (e.g. mouth -> Material.002_baseColor.png)
    if (lowerMat.find("mouth") != std::string::npos || lowerMat.find("teeth") != std::string::npos || lowerMat.find("002") != std::string::npos)
    {
        for (const auto& file : textureFiles)
        {
            std::string lf = ToLower(file);
            if (lf.find("mouth") != std::string::npos || lf.find("teeth") != std::string::npos || lf.find("002") != std::string::npos)
                return file;
        }
    }

    // 3. Substring fallback match
    for (const auto& file : textureFiles)
    {
        std::string lf = ToLower(file);
        if (lf.find(lowerMat) != std::string::npos || lowerMat.find(lf) != std::string::npos)
            return file;
    }

    return "";
}

IDirect3DTexture9* CTModSkeletalMesh::GetOrCreateTexture(const std::string& filePath, const std::string& baseDir, IDirect3DDevice9* pDevice)
{
    if (filePath.empty() || !pDevice)
        return nullptr;

    auto it = m_loadedTextures.find(filePath);
    if (it != m_loadedTextures.end())
        return it->second;

    std::string normPath = filePath;
    for (auto& c : normPath)
    {
        if (c == '\\')
            c = '/';
    }

    std::string bareName = normPath;
    size_t      lastSlash = bareName.find_last_of('/');
    if (lastSlash != std::string::npos)
        bareName = bareName.substr(lastSlash + 1);

    auto itBare = m_loadedTextures.find(bareName);
    if (itBare != m_loadedTextures.end())
    {
        m_loadedTextures[filePath] = itBare->second;
        return itBare->second;
    }

    std::vector<std::string> candidates;
    if (!baseDir.empty())
    {
        candidates.push_back(baseDir + "/" + bareName);
        candidates.push_back(baseDir + "/textures/" + bareName);
        candidates.push_back(baseDir + "/" + normPath);
    }
    candidates.push_back(normPath);
    candidates.push_back(bareName);

    std::string basePath = GetTMODBinPath();
    if (!baseDir.empty())
    {
        candidates.push_back(basePath + baseDir + "/" + bareName);
        candidates.push_back(basePath + baseDir + "/textures/" + bareName);
        candidates.push_back(SharedUtil::CalcMTASAPath(baseDir + "/" + bareName));
        candidates.push_back(SharedUtil::CalcMTASAPath(baseDir + "/textures/" + bareName));
    }
    candidates.push_back(basePath + normPath);
    candidates.push_back(SharedUtil::CalcMTASAPath(normPath));
    candidates.push_back(basePath + "Bin/" + normPath);
    candidates.push_back(SharedUtil::CalcMTASAPath("Bin/" + normPath));

    IDirect3DTexture9* pTex = nullptr;
    for (const auto& cand : candidates)
    {
        if (std::ifstream(cand.c_str()).good())
        {
            if (SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, cand.c_str(), &pTex)))
            {
                m_loadedTextures[filePath] = pTex;
                m_loadedTextures[bareName] = pTex;
                m_loadedTextures[cand] = pTex;
                return pTex;
            }
        }
    }

    return nullptr;
}

bool CTModSkeletalMesh::LoadTexture(const std::string& filePath, IDirect3DDevice9* pDevice)
{
    if (m_pTexture)
    {
        m_pTexture->Release();
        m_pTexture = nullptr;
    }
    return SUCCEEDED(D3DXCreateTextureFromFileA(pDevice, filePath.c_str(), &m_pTexture));
}

bool CTModSkeletalMesh::LoadFBX(const std::string& filePath, IDirect3DDevice9* pDevice)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(
        filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        return false;
    }

    std::vector<STModSkeletalVertex> vertices;
    std::vector<uint16_t>            indices;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[i];
        int     vertexOffset = vertices.size();

        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            STModSkeletalVertex vertex = {};
            vertex.position = D3DXVECTOR3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
            if (mesh->HasNormals())
            {
                vertex.normal = D3DXVECTOR3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
            }
            if (mesh->mTextureCoords[0])
            {
                vertex.texcoord = D3DXVECTOR2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            }

            // Default weights
            vertex.weights[0] = 1.0f;
            vertex.weights[1] = 0;
            vertex.weights[2] = 0;
            vertex.weights[3] = 0;
            vertex.boneIndices[0] = 0;
            vertex.boneIndices[1] = 0;
            vertex.boneIndices[2] = 0;
            vertex.boneIndices[3] = 0;

            vertices.push_back(vertex);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            aiFace face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(vertexOffset + face.mIndices[j]);
        }

        ExtractBoneWeights(vertices, mesh, vertexOffset);
    }

    ProcessNode(scene->mRootNode, scene, -1);

    m_numVertices = vertices.size();
    m_numIndices = indices.size();

    // Determine base directory of the FBX file for relative texture resolution
    std::string baseDir = filePath;
    size_t      lastSlash = baseDir.find_last_of("\\/");
    if (lastSlash != std::string::npos)
        baseDir = baseDir.substr(0, lastSlash);

    // Collect all candidate texture files in base directory and standard character texture locations
    std::vector<std::string> availableTextures;
    std::vector<std::string> scanDirs = {baseDir + "/textures",
                                         baseDir,
                                         GetTMODBinPath() + baseDir + "/textures",
                                         GetTMODBinPath() + "character/Male07/textures",
                                         GetTMODBinPath() + "Bin/character/Male07/textures",
                                         SharedUtil::CalcMTASAPath(baseDir + "/textures"),
                                         SharedUtil::CalcMTASAPath("character/Male07/textures"),
                                         SharedUtil::CalcMTASAPath("Bin/character/Male07/textures")};
    for (const auto& sDir : scanDirs)
    {
        std::vector<std::string> files = ListDirectoryFiles(sDir);
        for (const auto& f : files)
        {
            std::string lf = ToLower(f);
            if (lf.find(".png") != std::string::npos || lf.find(".jpg") != std::string::npos || lf.find(".jpeg") != std::string::npos ||
                lf.find(".dds") != std::string::npos || lf.find(".tga") != std::string::npos || lf.find(".bmp") != std::string::npos)
            {
                availableTextures.push_back(f);
            }
        }
    }

    m_subsets.clear();

    // Multi-texture partitioning:
    // Characters such as Male07 have multiple texture maps (clothes, mouth/teeth, eyes, and face/head).
    // If Assimp loads multiple meshes, we extract each mesh as a subset.
    // If Assimp collapsed into a single mesh (such as Male07 Idle.fbx which has 4540 faces),
    // we inspect companion definitions (e.g. NoAnim.obj) or known face partition counts
    // so the face texture (cara_del_serote_ciete.png) is bound to the face instead of the clothes texture.
    if (scene->mNumMeshes > 1)
    {
        uint32_t currentStartIndex = 0;
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            aiMesh* mesh = scene->mMeshes[m];
            if (!mesh)
                continue;

            uint32_t meshIndexCount = mesh->mNumFaces * 3;
            if (meshIndexCount == 0)
                continue;

            STModSkeletalSubset subset;
            subset.startIndex = currentStartIndex;
            subset.indexCount = meshIndexCount;

            if (mesh->mMaterialIndex < scene->mNumMaterials)
            {
                aiMaterial* pMat = scene->mMaterials[mesh->mMaterialIndex];
                aiString    matName;
                if (pMat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS)
                    subset.materialName = matName.C_Str();

                aiString texPath;
                for (int t = 0; t <= aiTextureType_UNKNOWN; ++t)
                {
                    if (pMat->GetTexture((aiTextureType)t, 0, &texPath) == AI_SUCCESS && texPath.length > 0)
                    {
                        subset.textureName = texPath.C_Str();
                        subset.texture = GetOrCreateTexture(subset.textureName, baseDir, pDevice);
                        break;
                    }
                }
                if (!subset.texture && !subset.materialName.empty())
                {
                    std::string resolvedTex = ResolveMaterialTexture(subset.materialName, baseDir, availableTextures);
                    if (!resolvedTex.empty())
                    {
                        subset.texture = GetOrCreateTexture(resolvedTex, baseDir, pDevice);
                        subset.textureName = resolvedTex;
                    }
                }
            }

            m_subsets.push_back(subset);
            currentStartIndex += meshIndexCount;
        }
    }
    else if (scene->mNumMeshes == 1)
    {
        std::vector<std::string> objCandidates = {baseDir + "/NoAnim.obj", GetTMODBinPath() + baseDir + "/NoAnim.obj",
                                                  SharedUtil::CalcMTASAPath(baseDir + "/NoAnim.obj"), SharedUtil::CalcMTASAPath("character/Male07/NoAnim.obj"),
                                                  SharedUtil::CalcMTASAPath("Bin/character/Male07/NoAnim.obj")};

        std::string companionObj = "";
        for (const auto& cand : objCandidates)
        {
            if (std::ifstream(cand.c_str()).good())
            {
                companionObj = cand;
                break;
            }
        }

        std::vector<std::pair<std::string, uint32_t>> partitions;
        if (!companionObj.empty())
        {
            std::ifstream objFile(companionObj.c_str());
            std::string   currentMat = "";
            uint32_t      currentFaceCount = 0;
            std::string   line;
            while (std::getline(objFile, line))
            {
                if (line.compare(0, 7, "usemtl ") == 0)
                {
                    if (!currentMat.empty() && currentFaceCount > 0)
                    {
                        partitions.push_back({currentMat, currentFaceCount});
                        currentFaceCount = 0;
                    }
                    currentMat = line.substr(7);
                    while (!currentMat.empty() && (currentMat.back() == '\r' || currentMat.back() == ' ' || currentMat.back() == '\t'))
                        currentMat.pop_back();
                }
                else if (line.compare(0, 2, "f ") == 0)
                {
                    currentFaceCount++;
                }
            }
            if (!currentMat.empty() && currentFaceCount > 0)
            {
                partitions.push_back({currentMat, currentFaceCount});
            }
        }

        // Verify total faces in companion OBJ matches the FBX mesh
        uint32_t totalObjFaces = 0;
        for (const auto& p : partitions)
            totalObjFaces += p.second;

        if (totalObjFaces * 3 != (uint32_t)m_numIndices)
        {
            partitions.clear();
            // Male07 built-in fallback: citizen_sheet (2474), mouth (224), eyeball_l (14), eyeball_r (14), mike_facemap (1814)
            if ((filePath.find("Male07") != std::string::npos || filePath.find("male07") != std::string::npos) && m_numIndices == 13620)
            {
                partitions = {{"citizen_sheet", 2474}, {"mouth", 224}, {"eyeball_l", 14}, {"eyeball_r", 14}, {"mike_facemap", 1814}};
            }
        }

        if (!partitions.empty())
        {
            uint32_t curStart = 0;
            for (const auto& p : partitions)
            {
                STModSkeletalSubset sub;
                sub.materialName = p.first;
                sub.startIndex = curStart;
                sub.indexCount = p.second * 3;
                std::string resolvedTex = ResolveMaterialTexture(p.first, baseDir, availableTextures);
                if (!resolvedTex.empty())
                {
                    sub.texture = GetOrCreateTexture(resolvedTex, baseDir, pDevice);
                    sub.textureName = resolvedTex;
                }
                m_subsets.push_back(sub);
                curStart += sub.indexCount;
            }
        }
    }

    // Merge adjacent subsets that share the exact same texture pointer to minimize draw calls
    if (m_subsets.size() > 1)
    {
        std::vector<STModSkeletalSubset> merged;
        for (const auto& sub : m_subsets)
        {
            if (!merged.empty() && merged.back().texture == sub.texture && (merged.back().startIndex + merged.back().indexCount == sub.startIndex))
            {
                merged.back().indexCount += sub.indexCount;
            }
            else
            {
                merged.push_back(sub);
            }
        }
        m_subsets = merged;
    }

    // Create D3D9 buffers
    pDevice->CreateVertexBuffer(m_numVertices * sizeof(STModSkeletalVertex), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &m_pVertexBuffer, nullptr);
    void* pVerts;
    m_pVertexBuffer->Lock(0, 0, &pVerts, 0);
    memcpy(pVerts, vertices.data(), m_numVertices * sizeof(STModSkeletalVertex));
    m_pVertexBuffer->Unlock();

    pDevice->CreateIndexBuffer(m_numIndices * sizeof(uint16_t), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIndexBuffer, nullptr);
    void* pInds;
    m_pIndexBuffer->Lock(0, 0, &pInds, 0);
    memcpy(pInds, indices.data(), m_numIndices * sizeof(uint16_t));
    m_pIndexBuffer->Unlock();

    D3DVERTEXELEMENT9 decl[] = {{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
                                {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
                                {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
                                {0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0},
                                {0, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
                                D3DDECL_END()};
    pDevice->CreateVertexDeclaration(decl, &m_pVertexDecl);

    // Compile Shaders: try vs_3_0 / ps_3_0 first, fallback to vs_2_0 / ps_2_0
    ID3DXBuffer* pVSBuffer = nullptr;
    ID3DXBuffer* pPSBuffer = nullptr;
    ID3DXBuffer* pError = nullptr;

    HRESULT hrVS = D3DXCompileShader(g_SkinningHLSL, strlen(g_SkinningHLSL), nullptr, nullptr, "mainVS", "vs_3_0", 0, &pVSBuffer, &pError, nullptr);
    if (FAILED(hrVS))
    {
        if (pError)
        {
            OutputDebugStringA((char*)pError->GetBufferPointer());
            pError->Release();
            pError = nullptr;
        }
        hrVS = D3DXCompileShader(g_SkinningHLSL, strlen(g_SkinningHLSL), nullptr, nullptr, "mainVS", "vs_2_0", 0, &pVSBuffer, &pError, nullptr);
    }
    if (SUCCEEDED(hrVS) && pVSBuffer)
    {
        pDevice->CreateVertexShader((DWORD*)pVSBuffer->GetBufferPointer(), &m_pVertexShader);
        pVSBuffer->Release();
    }
    else if (pError)
    {
        OutputDebugStringA((char*)pError->GetBufferPointer());
        pError->Release();
        pError = nullptr;
    }

    HRESULT hrPS = D3DXCompileShader(g_SkinningHLSL, strlen(g_SkinningHLSL), nullptr, nullptr, "mainPS", "ps_3_0", 0, &pPSBuffer, &pError, nullptr);
    if (FAILED(hrPS))
    {
        if (pError)
        {
            OutputDebugStringA((char*)pError->GetBufferPointer());
            pError->Release();
            pError = nullptr;
        }
        hrPS = D3DXCompileShader(g_SkinningHLSL, strlen(g_SkinningHLSL), nullptr, nullptr, "mainPS", "ps_2_0", 0, &pPSBuffer, &pError, nullptr);
    }
    if (SUCCEEDED(hrPS) && pPSBuffer)
    {
        pDevice->CreatePixelShader((DWORD*)pPSBuffer->GetBufferPointer(), &m_pPixelShader);
        pPSBuffer->Release();
    }
    else if (pError)
    {
        OutputDebugStringA((char*)pError->GetBufferPointer());
        pError->Release();
        pError = nullptr;
    }

    if (!m_pVertexShader || !m_pPixelShader)
    {
        OutputDebugStringA("[TMOD-ERROR] Failed to compile skinning vertex or pixel shader!\n");
        return false;
    }

    EnsureFallbackTexture(pDevice);
    return true;
}

void CTModSkeletalMesh::ExtractBoneWeights(std::vector<STModSkeletalVertex>& vertices, aiMesh* mesh, int vertexOffset)
{
    // Track weights assigned per vertex for this specific sub-mesh
    std::vector<int> weightCounts(mesh->mNumVertices, 0);

    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int         boneID = 0;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (m_boneMapping.find(boneName) == m_boneMapping.end())
        {
            STModBone boneInfo;
            boneInfo.name = boneName;
            boneInfo.offsetMatrix = ConvertAssimpMatrix(mesh->mBones[boneIndex]->mOffsetMatrix);
            D3DXMatrixIdentity(&boneInfo.finalMatrix);

            boneID = m_boneCount;
            m_boneMapping[boneName] = boneID;
            m_bones.push_back(boneInfo);
            m_boneCount++;
        }
        else
        {
            boneID = m_boneMapping[boneName];
        }

        aiBone* bone = mesh->mBones[boneIndex];
        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
        {
            int   localVertexId = bone->mWeights[weightIndex].mVertexId;
            float weight = bone->mWeights[weightIndex].mWeight;

            if (localVertexId >= 0 && localVertexId < (int)mesh->mNumVertices)
            {
                int currWeightCount = weightCounts[localVertexId];
                if (currWeightCount < MAX_BONES_PER_VERTEX)
                {
                    int globalVertexId = vertexOffset + localVertexId;
                    if (globalVertexId < (int)vertices.size())
                    {
                        // Clear the initial default 1.0f weight on the first real bone assignment
                        if (currWeightCount == 0)
                        {
                            vertices[globalVertexId].weights[0] = 0.0f;
                        }
                        vertices[globalVertexId].boneIndices[currWeightCount] = (float)boneID;
                        vertices[globalVertexId].weights[currWeightCount] = weight;
                        weightCounts[localVertexId]++;
                    }
                }
            }
        }
    }

    // Normalize weights for all vertices in this sub-mesh that have assigned bones
    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
    {
        if (weightCounts[v] > 0)
        {
            int   globalVertexId = vertexOffset + v;
            float totalWeight = 0.0f;
            for (int k = 0; k < MAX_BONES_PER_VERTEX; ++k)
            {
                totalWeight += vertices[globalVertexId].weights[k];
            }
            if (totalWeight > 0.0001f)
            {
                for (int k = 0; k < MAX_BONES_PER_VERTEX; ++k)
                {
                    vertices[globalVertexId].weights[k] /= totalWeight;
                }
            }
        }
    }
}

void CTModSkeletalMesh::ProcessNode(aiNode* node, const aiScene* scene, int parentIndex)
{
    std::string nodeName = node->mName.C_Str();
    int         currentBoneIndex = parentIndex;

    if (m_boneMapping.find(nodeName) != m_boneMapping.end())
    {
        currentBoneIndex = m_boneMapping[nodeName];
        m_bones[currentBoneIndex].parentIndex = parentIndex;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, currentBoneIndex);
    }
}

void CTModSkeletalMesh::UpdateBoneMatrices(IDirect3DDevice9* pDevice, const std::vector<D3DXMATRIX>& boneMatrices)
{
    // Cache the updated bone matrices so they can be reliably uploaded during the render pass.
    // Setting shader constants during DoPulse() is ineffective because GTA SA's scene rendering
    // overwrites all shader constant registers between DoPulse() and PreFxRender.
    m_currentBoneMatrices = boneMatrices;
}

void CTModSkeletalMesh::Render(IDirect3DDevice9* pDevice)
{
    if (!m_pVertexBuffer || !m_pIndexBuffer || !m_pVertexDecl || !pDevice)
        return;

    EnsureFallbackTexture(pDevice);

    pDevice->SetVertexDeclaration(m_pVertexDecl);
    pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(STModSkeletalVertex));
    pDevice->SetIndices(m_pIndexBuffer);

    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

    if (m_pVertexShader)
    {
        pDevice->SetVertexShader(m_pVertexShader);

        // Upload bone matrices right before the draw call to ensure registers c8..c247
        // hold valid animation transforms rather than leftover constants from GTA's world shaders.
        if (!m_currentBoneMatrices.empty())
        {
            int numBones = std::min((int)m_currentBoneMatrices.size(), 60);
            pDevice->SetVertexShaderConstantF(8, (const float*)m_currentBoneMatrices.data(), numBones * 4);
        }
        else
        {
            // Identity fallback so vertices do not collapse if animation has not yet ticked
            std::vector<D3DXMATRIX> idBones(60);
            for (int k = 0; k < 60; ++k)
                D3DXMatrixIdentity(&idBones[k]);
            pDevice->SetVertexShaderConstantF(8, (const float*)idBones.data(), 60 * 4);
        }
    }
    if (m_pPixelShader)
        pDevice->SetPixelShader(m_pPixelShader);

    // Multi-material subset rendering:
    // If the skeletal mesh contains subsets with individual textures (e.g. face vs clothes vs eyes vs mouth),
    // bind the corresponding texture for each subset and draw only its assigned triangle range.
    // Otherwise fallback to drawing the full mesh with m_pTexture.
    if (!m_subsets.empty())
    {
        for (const auto& subset : m_subsets)
        {
            IDirect3DTexture9* pActiveTex = subset.texture ? subset.texture : (m_pTexture ? m_pTexture : m_pFallbackTexture);
            if (pActiveTex)
            {
                pDevice->SetTexture(0, pActiveTex);
            }
            pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_numVertices, subset.startIndex, subset.indexCount / 3);
        }
    }
    else
    {
        IDirect3DTexture9* pActiveTex = m_pTexture ? m_pTexture : m_pFallbackTexture;
        if (pActiveTex)
        {
            pDevice->SetTexture(0, pActiveTex);
        }
        pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_numVertices, 0, m_numIndices / 3);
    }

    pDevice->SetVertexShader(nullptr);
    pDevice->SetPixelShader(nullptr);
    pDevice->SetTexture(0, nullptr);
}
