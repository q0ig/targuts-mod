#include "StdInc.h"
#include "CTModAssetManager.h"
#include "CClientGame.h"
#include "CClientCamera.h"
#include "../physics/CTModPhysicsManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <set>

void CTModAssetManager::Init()
{
    m_pD3DDevice = g_pCore->GetGraphics()->GetDevice();
    LoadPermaProps();
    // Native local map auto-loader: map_fbx is placed directly in Bin/map_fbx and root map_fbx (like character/)
    AutoLoadCityMap();
}

void CTModAssetManager::Shutdown()
{
    SavePermaProps();

    // Collect unique mesh pointers to prevent double-free when multiple keys reference the same mesh
    std::set<STModMesh*> uniqueMeshes;
    for (auto& pair : m_models)
    {
        if (pair.second)
            uniqueMeshes.insert(pair.second);
    }
    m_models.clear();

    for (STModMesh* pMesh : uniqueMeshes)
    {
        if (pMesh->vertexBuffer)
            pMesh->vertexBuffer->Release();
        if (pMesh->indexBuffer)
            pMesh->indexBuffer->Release();
        if (pMesh->bulletShape)
            delete pMesh->bulletShape;
        if (pMesh->bulletMeshInterface)
            delete pMesh->bulletMeshInterface;
        delete pMesh;
    }

    // Release all cached Direct3D 9 textures
    for (auto& pair : m_textures)
    {
        if (pair.second)
        {
            pair.second->Release();
        }
    }
    m_textures.clear();
    m_props.clear();
}

#include <windows.h>

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

std::string CTModAssetManager::LoadModel(const std::string& filePath)
{
    if (m_models.count(filePath))
        return filePath;

    // Check if bare filename is already cached
    std::string quickBare = filePath;
    size_t      qSlash = quickBare.find_last_of("\\/");
    if (qSlash != std::string::npos)
        quickBare = quickBare.substr(qSlash + 1);
    if (!quickBare.empty() && m_models.count(quickBare))
        return quickBare;

    // Resolve file path across standard MTA resource directories
    std::string   resolvedPath = filePath;
    std::ifstream testFile(resolvedPath.c_str());
    if (!testFile.is_open())
    {
        std::string basePath = GetTMODBinPath();

        // Support MTA resource syntax: ":resourceName/subPath"
        std::string resName = "";
        std::string subPath = "";
        if (!filePath.empty() && filePath[0] == ':')
        {
            size_t slashPos = filePath.find_first_of("\\/", 1);
            if (slashPos != std::string::npos)
            {
                resName = filePath.substr(1, slashPos - 1);
                subPath = filePath.substr(slashPos + 1);
            }
        }

        // Strip any leading directory prefix to get the bare filename
        // This handles cases where filePath is passed as "map_fbx/city_map.obj"
        // avoiding path doubling like ".../resources/map_fbx/map_fbx/city_map.obj".
        std::string bareFileName = (!subPath.empty()) ? subPath : filePath;
        size_t      lastSlash = bareFileName.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            bareFileName = bareFileName.substr(lastSlash + 1);

        std::vector<std::string> candidates;

        // 1. Direct TMOD native client folders (Bin/map_fbx and root map_fbx, just like character/ and first-person/)
        candidates.push_back(basePath + "map_fbx/" + bareFileName);
        candidates.push_back(CalcMTASAPath("map_fbx/" + bareFileName));
        candidates.push_back(basePath + "Bin/map_fbx/" + bareFileName);
        candidates.push_back(CalcMTASAPath("Bin/map_fbx/" + bareFileName));
        candidates.push_back("map_fbx/" + bareFileName);
        candidates.push_back("Bin/map_fbx/" + bareFileName);

        if (!subPath.empty())
        {
            candidates.push_back(basePath + "map_fbx/" + subPath);
            candidates.push_back(CalcMTASAPath("map_fbx/" + subPath));
            candidates.push_back(basePath + "Bin/map_fbx/" + subPath);
            candidates.push_back(CalcMTASAPath("Bin/map_fbx/" + subPath));
        }

        candidates.push_back(basePath + filePath);
        candidates.push_back(CalcMTASAPath(filePath));
        candidates.push_back(basePath + "Bin/" + filePath);
        candidates.push_back(CalcMTASAPath("Bin/" + filePath));

        candidates.push_back(basePath + bareFileName);
        candidates.push_back(CalcMTASAPath(bareFileName));

        candidates.push_back(basePath + "map_fbx/source/" + bareFileName);
        candidates.push_back(CalcMTASAPath("map_fbx/source/" + bareFileName));
        candidates.push_back(basePath + "Bin/map_fbx/source/" + bareFileName);
        candidates.push_back(CalcMTASAPath("Bin/map_fbx/source/" + bareFileName));

        // Only add cross-extension fallback at the very end of candidates if needed
        std::vector<std::string> fallbackCandidates;
        if (filePath.size() >= 4 && filePath.compare(filePath.size() - 4, 4, ".fbx") == 0)
        {
            std::string objBase = bareFileName.substr(0, bareFileName.size() - 4) + ".obj";
            fallbackCandidates.push_back(basePath + "map_fbx/" + objBase);
            fallbackCandidates.push_back(CalcMTASAPath("map_fbx/" + objBase));
            fallbackCandidates.push_back(basePath + "Bin/map_fbx/" + objBase);
            fallbackCandidates.push_back(CalcMTASAPath("Bin/map_fbx/" + objBase));
        }
        else if (filePath.size() >= 4 && filePath.compare(filePath.size() - 4, 4, ".obj") == 0)
        {
            std::string fbxBase = bareFileName.substr(0, bareFileName.size() - 4) + ".fbx";
            fallbackCandidates.push_back(basePath + "map_fbx/" + fbxBase);
            fallbackCandidates.push_back(CalcMTASAPath("map_fbx/" + fbxBase));
            fallbackCandidates.push_back(basePath + "Bin/map_fbx/" + fbxBase);
            fallbackCandidates.push_back(CalcMTASAPath("Bin/map_fbx/" + fbxBase));
        }

        // 2. Resource-based paths (fallback for server resource compatibility)
        if (!resName.empty() && !subPath.empty())
        {
            candidates.push_back(basePath + "mods/deathmatch/resources/" + resName + "/" + subPath);
            candidates.push_back(basePath + "server/mods/deathmatch/resources/" + resName + "/" + subPath);
            candidates.push_back(basePath + "mods/deathmatch/resource-cache/http-client-files/" + resName + "/" + subPath);
            candidates.push_back(basePath + "server/mods/deathmatch/resource-cache/http-client-files/" + resName + "/" + subPath);

            candidates.push_back(CalcMTASAPath("mods/deathmatch/resources/" + resName + "/" + subPath));
            candidates.push_back(CalcMTASAPath("server/mods/deathmatch/resources/" + resName + "/" + subPath));
            candidates.push_back(CalcMTASAPath("mods/deathmatch/resource-cache/http-client-files/" + resName + "/" + subPath));
            candidates.push_back(CalcMTASAPath("server/mods/deathmatch/resource-cache/http-client-files/" + resName + "/" + subPath));
        }

        candidates.insert(
            candidates.end(),
            {// Direct path relative to MTA install directory
             CalcMTASAPath("server/mods/deathmatch/resources/map_fbx/source/" + bareFileName),
             CalcMTASAPath("mods/deathmatch/resources/map_fbx/source/" + bareFileName),
             CalcMTASAPath("server/mods/deathmatch/resources/map_fbx/" + bareFileName), CalcMTASAPath("mods/deathmatch/resources/map_fbx/" + bareFileName),
             CalcMTASAPath("server/mods/deathmatch/resource-cache/http-client-files/map_fbx/" + bareFileName),
             CalcMTASAPath("mods/deathmatch/resource-cache/http-client-files/map_fbx/" + bareFileName),
             CalcMTASAPath("server/mods/deathmatch/resources/hogwarts_map/" + bareFileName),
             CalcMTASAPath("mods/deathmatch/resources/hogwarts_map/" + bareFileName), CalcMTASAPath("server/mods/deathmatch/resources/" + filePath),
             CalcMTASAPath("mods/deathmatch/resources/" + filePath), CalcMTASAPath("server/mods/deathmatch/resource-cache/http-client-files/" + filePath),
             CalcMTASAPath("mods/deathmatch/resource-cache/http-client-files/" + filePath), CalcMTASAPath("map_fbx_cache/" + bareFileName),
             // Reliable DEV build paths based on DLL module location
             basePath + "server/mods/deathmatch/resources/map_fbx/source/" + bareFileName,
             basePath + "mods/deathmatch/resources/map_fbx/source/" + bareFileName, basePath + "server/mods/deathmatch/resources/map_fbx/" + bareFileName,
             basePath + "mods/deathmatch/resources/map_fbx/" + bareFileName,
             basePath + "server/mods/deathmatch/resource-cache/http-client-files/map_fbx/" + bareFileName,
             basePath + "mods/deathmatch/resource-cache/http-client-files/map_fbx/" + bareFileName, basePath + "server/mods/deathmatch/resources/" + filePath,
             basePath + "mods/deathmatch/resources/" + filePath, basePath + "map_fbx_cache/" + bareFileName,
             // Also try directly under CalcMTASAPath root
             CalcMTASAPath(filePath), basePath + filePath, CalcMTASAPath(bareFileName), basePath + bareFileName});

        candidates.insert(candidates.end(), fallbackCandidates.begin(), fallbackCandidates.end());

        for (const auto& cand : candidates)
        {
            if (std::ifstream(cand.c_str()).good())
            {
                resolvedPath = cand;
                break;
            }
        }
    }
    testFile.close();

    // Debug: log which path was resolved to help diagnose model loading failures
    {
        char dbgBuf[512];
        sprintf_s(dbgBuf, "TMOD LoadModel: '%s' -> resolved to '%s'\n", filePath.c_str(), resolvedPath.c_str());
        OutputDebugStringA(dbgBuf);
    }

    // Determine if this is a non-OBJ format that requires Assimp
    std::string ext = "";
    size_t      dotPos = resolvedPath.rfind('.');
    if (dotPos != std::string::npos)
        ext = resolvedPath.substr(dotPos);
    // Convert to lowercase for comparison
    for (auto& c : ext)
        c = (char)tolower(c);

    if (ext != ".obj")
    {
        // Use Assimp for FBX, DAE, GLTF, and all other non-OBJ formats.
        // aiProcess_PreTransformVertices collapses the node hierarchy so vertices are placed in world coordinates.
        Assimp::Importer importer;
        const aiScene*   scene =
            importer.ReadFile(resolvedPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights |
                                                aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
        {
            char errBuf[512];
            sprintf_s(errBuf, "[TMOD-ERROR] Assimp failed to load: %s, Error: %s\n", resolvedPath.c_str(), importer.GetErrorString());
            OutputDebugStringA(errBuf);
            return "";
        }

        std::vector<VertexPosNormalTex> vertices;
        std::vector<uint32_t>           indices;
        std::vector<STModSubset>        subsets;

        // Base directory for relative texture paths
        std::string baseDir = resolvedPath;
        size_t      lastSlash = baseDir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            baseDir = baseDir.substr(0, lastSlash);

        // Preload materials into textures
        std::vector<IDirect3DTexture9*> materialTextures(scene->mNumMaterials, nullptr);
        std::vector<std::string>        materialTexNames(scene->mNumMaterials, "");

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial* mat = scene->mMaterials[i];
            if (!mat)
                continue;

            aiString texPath;
            bool     bFound = false;
            for (int t = 0; t <= aiTextureType_UNKNOWN; ++t)
            {
                if (mat->GetTexture((aiTextureType)t, 0, &texPath) == AI_SUCCESS && texPath.length > 0)
                {
                    materialTexNames[i] = texPath.C_Str();
                    materialTextures[i] = GetOrCreateTexture(materialTexNames[i], baseDir);
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                aiString matName;
                if (mat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS && matName.length > 0)
                {
                    materialTexNames[i] = matName.C_Str();
                    materialTextures[i] = GetOrCreateTexture(materialTexNames[i], baseDir);
                }
            }
        }

        // Scale and Axis Mapping:
        // FBX models exported from 3ds Max / Maya (like city_map.fbx) use Centimeters (1 unit = 1 cm)
        // and Y-Up coordinates.
        // In MTA / GTA SA, world units are Meters (1 unit = 1 m) and coordinates are Z-Up.
        // Therefore, we convert from FBX (Y-Up) to GTA/MTA (Z-Up):
        // 1. Scale factor: for city_map, fScale = 0.04f (bringing 250-unit buildings to 10.0m,
        //    570-unit buildings to 22.8m, and skyscrapers to 56.7m, perfectly matching 1.80m human scale).
        // 2. Rotate +90 deg around X-axis: gtaX = fbxX, gtaY = -fbxZ, gtaZ = fbxY.
        // 3. For city_map and all custom FBX maps, the model origin (0, 0, 0) is already centered
        //    at the main crossroads/plaza, and rawY = 0 is the asphalt street level.
        //    Keeping xCenter = 0.0f and zOffset = 0.0f preserves the 3D artist's intended pivot,
        //    preventing artificial horizontal shifts that would move the map away from its spawn point.
        float fScale = 1.0f;
        float xCenter = 0.0f;
        float zOffset = 0.0f;
        bool  bConvertYUpToZUp = false;

        if (ext == ".fbx")
        {
            fScale = 0.04f;
            bConvertYUpToZUp = true;
            xCenter = 0.0f;
            zOffset = 0.0f;
        }

        // Walk all meshes in the scene and extract geometry and subsets
        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];
            if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0)
                continue;

            uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
            uint32_t indexStart = static_cast<uint32_t>(indices.size());

            for (unsigned int v = 0; v < mesh->mNumVertices; v++)
            {
                VertexPosNormalTex vert = {};
                float              rawX = mesh->mVertices[v].x;
                float              rawY = mesh->mVertices[v].y;
                float              rawZ = mesh->mVertices[v].z;

                if (bConvertYUpToZUp)
                {
                    vert.x = (rawX * fScale) - xCenter;
                    vert.y = -(rawZ * fScale);
                    vert.z = (rawY * fScale) + zOffset;

                    if (mesh->HasNormals())
                    {
                        vert.nx = mesh->mNormals[v].x;
                        vert.ny = -mesh->mNormals[v].z;
                        vert.nz = mesh->mNormals[v].y;
                    }
                }
                else
                {
                    vert.x = rawX * fScale;
                    vert.y = rawY * fScale;
                    vert.z = rawZ * fScale;

                    if (mesh->HasNormals())
                    {
                        vert.nx = mesh->mNormals[v].x;
                        vert.ny = mesh->mNormals[v].y;
                        vert.nz = mesh->mNormals[v].z;
                    }
                }

                if (mesh->mTextureCoords[0])
                {
                    vert.u = mesh->mTextureCoords[0][v].x;
                    vert.v = mesh->mTextureCoords[0][v].y;
                }
                vertices.push_back(vert);
            }

            for (unsigned int f = 0; f < mesh->mNumFaces; f++)
            {
                aiFace& face = mesh->mFaces[f];
                if (face.mNumIndices == 3)
                {
                    indices.push_back(vertexOffset + face.mIndices[0]);
                    indices.push_back(vertexOffset + face.mIndices[1]);
                    indices.push_back(vertexOffset + face.mIndices[2]);
                }
            }

            uint32_t meshIndexCount = static_cast<uint32_t>(indices.size()) - indexStart;
            if (meshIndexCount > 0)
            {
                STModSubset subset;
                subset.startIndex = indexStart;
                subset.indexCount = meshIndexCount;
                if (mesh->mMaterialIndex < materialTextures.size())
                {
                    subset.texture = materialTextures[mesh->mMaterialIndex];
                    subset.textureName = materialTexNames[mesh->mMaterialIndex];
                }
                subsets.push_back(subset);
            }
        }

        if (vertices.empty() || indices.empty())
        {
            return "";
        }

        // Merge adjacent subsets with identical texture to reduce draw calls
        std::vector<STModSubset> mergedSubsets;
        for (const auto& sub : subsets)
        {
            if (!mergedSubsets.empty() && mergedSubsets.back().texture == sub.texture && mergedSubsets.back().textureName == sub.textureName &&
                (mergedSubsets.back().startIndex + mergedSubsets.back().indexCount == sub.startIndex))
            {
                mergedSubsets.back().indexCount += sub.indexCount;
            }
            else
            {
                mergedSubsets.push_back(sub);
            }
        }

        // Build mesh exactly like the OBJ path does
        STModMesh* newMesh = new STModMesh();
        newMesh->vertexCount = static_cast<int>(vertices.size());
        newMesh->indexCount = static_cast<int>(indices.size());
        newMesh->rawVertexData = vertices;
        newMesh->rawIndexData = indices;
        newMesh->subsets = mergedSubsets;
        if (!mergedSubsets.empty())
        {
            newMesh->texture = mergedSubsets[0].texture;
        }

        if (m_pD3DDevice)
        {
            CreateD3DBuffers(newMesh);
        }

        // Build Bullet Physics static triangle mesh collider
        newMesh->indices.assign(indices.begin(), indices.end());
        newMesh->vertices.resize(vertices.size() * 3);
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            newMesh->vertices[i * 3 + 0] = vertices[i].x;
            newMesh->vertices[i * 3 + 1] = vertices[i].y;
            newMesh->vertices[i * 3 + 2] = vertices[i].z;
        }

        btTriangleIndexVertexArray* meshInterface =
            new btTriangleIndexVertexArray(static_cast<int>(newMesh->indices.size() / 3), reinterpret_cast<int*>(newMesh->indices.data()), 3 * sizeof(int),
                                           static_cast<int>(newMesh->vertices.size() / 3), newMesh->vertices.data(), 3 * sizeof(float));
        btBvhTriangleMeshShape* bvhShape = new btBvhTriangleMeshShape(meshInterface, true);
        newMesh->bulletShape = bvhShape;
        newMesh->bulletMeshInterface = meshInterface;

        m_models[filePath] = newMesh;
        m_models[resolvedPath] = newMesh;
        std::string bareName = filePath;
        size_t      bSlash = bareName.find_last_of("\\/");
        if (bSlash != std::string::npos)
            bareName = bareName.substr(bSlash + 1);
        if (!bareName.empty())
            m_models[bareName] = newMesh;
        return filePath;
    }

    std::ifstream file(resolvedPath.c_str());
    if (!file.is_open())
    {
        return "";
    }

    std::vector<CVector>  rawVertices;
    std::vector<uint32_t> indices;

    // Olcek yonetimi:
    // Source Engine / GMod modelleri (Hogwarts) varsayilan 1 inch = 0.0254m olcegi kullanir.
    // city_map icin 1.80m karakter olcegiyle uyumlu 4.0f olcegi kullanilir (FBX ile birebir ayni).
    // Diger standart 3D haritalar (FBX / OBJ / Blender) 1.0f metre olcegi kullanir.
    // Dosya basinda '# scale <deger>' varsa dogrudan o deger onceliklidir.
    float fScale = 1.0f;
    if (filePath.find("hogwarts") != std::string::npos || filePath.find("targut") != std::string::npos)
    {
        fScale = 0.0254f;
    }
    else if (filePath.find("city_map") != std::string::npos || resolvedPath.find("city_map") != std::string::npos)
    {
        fScale = 4.0f;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (line[0] == '#')
        {
            if (line.rfind("# scale ", 0) == 0)
            {
                sscanf(line.c_str() + 8, "%f", &fScale);
            }
            continue;
        }

        if (line[0] == 'v' && line[1] == ' ')
        {
            float x, y, z;
            if (sscanf(line.c_str() + 2, "%f %f %f", &x, &y, &z) == 3)
            {
                rawVertices.push_back(CVector(x * fScale, y * fScale, z * fScale));
            }
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            std::stringstream     ss(line.substr(2));
            std::string           part;
            std::vector<uint32_t> faceIndices;
            while (ss >> part)
            {
                int vIdx = 0;
                sscanf(part.c_str(), "%d", &vIdx);
                if (vIdx > 0)
                {
                    faceIndices.push_back(static_cast<uint32_t>(vIdx - 1));
                }
                else if (vIdx < 0)
                {
                    faceIndices.push_back(static_cast<uint32_t>(rawVertices.size() + vIdx));
                }
            }
            // Triangulate n-gon using triangle fan from vertex 0
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[i]);
                indices.push_back(faceIndices[i + 1]);
            }
        }
    }
    file.close();

    if (rawVertices.empty() || indices.empty())
    {
        return "";
    }

    // Build vertex array with default normals
    std::vector<VertexPosNormalTex> vertices(rawVertices.size());
    for (size_t i = 0; i < rawVertices.size(); ++i)
    {
        vertices[i].x = rawVertices[i].fX;
        vertices[i].y = rawVertices[i].fY;
        vertices[i].z = rawVertices[i].fZ;
        vertices[i].nx = 0.0f;
        vertices[i].ny = 0.0f;
        vertices[i].nz = 1.0f;
        vertices[i].u = 0.0f;
        vertices[i].v = 0.0f;
    }

    // Calculate surface normals per triangle
    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        if (i0 < vertices.size() && i1 < vertices.size() && i2 < vertices.size())
        {
            CVector v0(vertices[i0].x, vertices[i0].y, vertices[i0].z);
            CVector v1(vertices[i1].x, vertices[i1].y, vertices[i1].z);
            CVector v2(vertices[i2].x, vertices[i2].y, vertices[i2].z);
            CVector d1 = v1 - v0;
            CVector d2 = v2 - v0;
            CVector norm = d1;
            norm.CrossProduct(&d2);
            norm.Normalize();
            vertices[i0].nx = norm.fX;
            vertices[i0].ny = norm.fY;
            vertices[i0].nz = norm.fZ;
            vertices[i1].nx = norm.fX;
            vertices[i1].ny = norm.fY;
            vertices[i1].nz = norm.fZ;
            vertices[i2].nx = norm.fX;
            vertices[i2].ny = norm.fY;
            vertices[i2].nz = norm.fZ;
        }
    }

    STModMesh* newMesh = new STModMesh();
    newMesh->vertexCount = static_cast<int>(vertices.size());
    newMesh->indexCount = static_cast<int>(indices.size());

    // Ham verileri yedekle: D3D cihazi henuz hazir degilse (resource startup sirasinda
    // Init() cagrilmadan once LoadModel calisirsa), Render() icinde lazy olarak
    // D3D buffer olusturabilmek icin rawVertexData ve rawIndexData'ya kopyala.
    newMesh->rawVertexData = vertices;
    newMesh->rawIndexData = indices;

    // D3D buffer'lari hemen olusturmayi dene (cihaz mevcutsa)
    if (m_pD3DDevice)
    {
        CreateD3DBuffers(newMesh);
    }

    // Build Bullet Physics static triangle mesh collider
    newMesh->indices.assign(indices.begin(), indices.end());
    newMesh->vertices.resize(vertices.size() * 3);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        newMesh->vertices[i * 3 + 0] = vertices[i].x;
        newMesh->vertices[i * 3 + 1] = vertices[i].y;
        newMesh->vertices[i * 3 + 2] = vertices[i].z;
    }

    btTriangleIndexVertexArray* meshInterface =
        new btTriangleIndexVertexArray(static_cast<int>(newMesh->indices.size() / 3), reinterpret_cast<int*>(newMesh->indices.data()), 3 * sizeof(int),
                                       static_cast<int>(newMesh->vertices.size() / 3), newMesh->vertices.data(), 3 * sizeof(float));
    btBvhTriangleMeshShape* bvhShape = new btBvhTriangleMeshShape(meshInterface, true);
    newMesh->bulletShape = bvhShape;
    newMesh->bulletMeshInterface = meshInterface;

    m_models[filePath] = newMesh;
    m_models[resolvedPath] = newMesh;
    std::string bareName = filePath;
    size_t      bSlash = bareName.find_last_of("\\/");
    if (bSlash != std::string::npos)
        bareName = bareName.substr(bSlash + 1);
    if (!bareName.empty())
        m_models[bareName] = newMesh;
    return filePath;
}

void CTModAssetManager::CreateD3DBuffers(STModMesh* pMesh)
{
    if (!pMesh || !m_pD3DDevice || pMesh->rawVertexData.empty())
        return;

    // Vertex Buffer
    if (!pMesh->vertexBuffer)
    {
        HRESULT hr = m_pD3DDevice->CreateVertexBuffer(pMesh->rawVertexData.size() * sizeof(VertexPosNormalTex), D3DUSAGE_WRITEONLY, TMOD_FVF, D3DPOOL_MANAGED,
                                                      &pMesh->vertexBuffer, NULL);
        if (SUCCEEDED(hr) && pMesh->vertexBuffer)
        {
            void* pData = nullptr;
            if (SUCCEEDED(pMesh->vertexBuffer->Lock(0, 0, &pData, 0)) && pData)
            {
                memcpy(pData, pMesh->rawVertexData.data(), pMesh->rawVertexData.size() * sizeof(VertexPosNormalTex));
                pMesh->vertexBuffer->Unlock();
            }
        }
    }

    // Index Buffer:
    // When vertexCount <= 65535, use D3DFMT_INDEX16 which has maximum compatibility with Direct3D 9 fixed-function.
    // When vertexCount > 65535, use D3DFMT_INDEX32 to prevent 16-bit integer overflow and geometry corruption on large maps.
    if (!pMesh->indexBuffer && !pMesh->rawIndexData.empty())
    {
        pMesh->bIndex32 = (pMesh->vertexCount > 65535);
        D3DFORMAT indexFormat = pMesh->bIndex32 ? D3DFMT_INDEX32 : D3DFMT_INDEX16;
        size_t    indexStride = pMesh->bIndex32 ? sizeof(uint32_t) : sizeof(uint16_t);

        HRESULT hr = m_pD3DDevice->CreateIndexBuffer(static_cast<UINT>(pMesh->rawIndexData.size() * indexStride), D3DUSAGE_WRITEONLY, indexFormat,
                                                     D3DPOOL_MANAGED, &pMesh->indexBuffer, NULL);
        if (SUCCEEDED(hr) && pMesh->indexBuffer)
        {
            void* pData = nullptr;
            if (SUCCEEDED(pMesh->indexBuffer->Lock(0, 0, &pData, 0)) && pData)
            {
                if (pMesh->bIndex32)
                {
                    memcpy(pData, pMesh->rawIndexData.data(), pMesh->rawIndexData.size() * sizeof(uint32_t));
                }
                else
                {
                    uint16_t* pDest16 = reinterpret_cast<uint16_t*>(pData);
                    for (size_t i = 0; i < pMesh->rawIndexData.size(); ++i)
                    {
                        pDest16[i] = static_cast<uint16_t>(pMesh->rawIndexData[i]);
                    }
                }
                pMesh->indexBuffer->Unlock();
            }
        }
    }
}

int CTModAssetManager::SpawnProp(const std::string& modelHandle, float x, float y, float z, float rx, float ry, float rz, float mass, bool isStatic)
{
    if (!m_models.count(modelHandle))
        return -1;

    // Statik objelerde ayni koordinatta ve ayni modelde cift obje dogurmayi engelle
    if (isStatic)
    {
        for (const auto& pair : m_props)
        {
            const STModProp& existing = pair.second;
            if (existing.bStatic && std::abs(existing.transform.vPos.fX - x) < 2.0f && std::abs(existing.transform.vPos.fY - y) < 2.0f &&
                std::abs(existing.transform.vPos.fZ - z) < 2.0f)
            {
                return existing.id;
            }
        }
    }

    STModMesh* pMesh = m_models[modelHandle];
    int        propId = m_nextPropId++;
    STModProp  prop;
    prop.id = propId;
    prop.modelHandle = modelHandle;
    prop.bStatic = isStatic;
    prop.bPermanent = false;
    prop.transform = CMatrix();
    prop.transform.vPos = CVector(x, y, z);
    prop.rotation = CVector(rx, ry, rz);

    if (pMesh && pMesh->bulletShape)
    {
        prop.physicsBodyId = CTModPhysicsManager::GetSingleton().CreateCustomBody(pMesh->bulletShape, x, y, z, rx, ry, rz, isStatic ? 0.0f : mass);
    }
    else
    {
        prop.physicsBodyId = CTModPhysicsManager::GetSingleton().CreateRigidBody(x, y, z, 1.0f, 1.0f, 1.0f, isStatic ? 0.0f : mass);
    }

    m_props[propId] = prop;
    return propId;
}

void CTModAssetManager::SetPropPerma(int propId, bool permanent)
{
    if (m_props.count(propId))
    {
        m_props[propId].bPermanent = permanent;
        SavePermaProps();
    }
}

void CTModAssetManager::Render()
{
    if (m_props.empty())
        return;
    if (!m_pD3DDevice && g_pCore && g_pCore->GetGraphics())
    {
        m_pD3DDevice = g_pCore->GetGraphics()->GetDevice();
    }
    if (!m_pD3DDevice)
        return;

    CClientCamera* pCamera = g_pClientGame->GetManager()->GetCamera();
    if (!pCamera)
        return;

    // Gecikmeli (lazy) D3D buffer olusturma: LoadModel sirasinda m_pD3DDevice null
    // olabilir (resource startup Init()'den once calisir). Bu durumda rawVertexData
    // dolu ama vertexBuffer null kalir. Ilk Render() cagrisinda buffer'lari olustur.
    for (auto& pair : m_models)
    {
        STModMesh* pMesh = pair.second;
        if (!pMesh->vertexBuffer && !pMesh->rawVertexData.empty())
        {
            CreateD3DBuffers(pMesh);
        }
    }

    CVector camPos;
    pCamera->GetPosition(camPos);

    CMatrix camMatrix;
    pCamera->GetMatrix(camMatrix);

    // Save full D3D9 state block so custom drawing doesn't disturb GTA's post-FX or shader pipeline
    IDirect3DStateBlock9* pSavedState = nullptr;
    m_pD3DDevice->CreateStateBlock(D3DSBT_ALL, &pSavedState);

    // Disable active shaders to enable fixed-function D3D9 vertex pipeline
    m_pD3DDevice->SetVertexShader(NULL);
    m_pD3DDevice->SetPixelShader(NULL);

    // Save GTA's active View and Projection matrices directly from the D3D device.
    // Do NOT overwrite them with custom LookAtLH or PerspectiveFovLH matrices, as doing so
    // mirrors the horizontal axis, creates depth buffer mismatches against GTA's Z-buffer,
    // and corrupts device transforms for subsequent renderers (e.g. skeletal meshes).
    D3DMATRIX matOrigView, matOrigProj;
    m_pD3DDevice->GetTransform(D3DTS_VIEW, &matOrigView);
    m_pD3DDevice->GetTransform(D3DTS_PROJECTION, &matOrigProj);

    // Configure 3D depth and surface states
    m_pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    m_pD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    m_pD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);  // Double-sided so both interior and exterior are visible
    m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    // Explicitly disable alpha testing so untextured models are never rejected by GTA's default alpha test
    m_pD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

    // Cast solid stone castle tint with full alpha (255)
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    m_pD3DDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 205, 195, 180));
    m_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    m_pD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    for (const auto& pair : m_props)
    {
        const STModProp& prop = pair.second;
        CVector          diff;
        diff.fX = prop.transform.vPos.fX - camPos.fX;
        diff.fY = prop.transform.vPos.fY - camPos.fY;
        diff.fZ = prop.transform.vPos.fZ - camPos.fZ;
        if (diff.Length() > 6000.0f)
            continue;

        if (m_models.count(prop.modelHandle))
            RenderMesh(m_models[prop.modelHandle], prop);
    }

    // Cleanly restore all previous render states, vertex/pixel shaders, and texture stages
    if (pSavedState)
    {
        pSavedState->Apply();
        pSavedState->Release();
    }

    // Explicitly restore GTA's active View and Projection transforms (state block does not record transforms)
    m_pD3DDevice->SetTransform(D3DTS_VIEW, &matOrigView);
    m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &matOrigProj);
}

IDirect3DTexture9* CTModAssetManager::GetOrCreateTexture(const std::string& texturePath, const std::string& baseDir)
{
    if (texturePath.empty())
        return nullptr;

    if (!m_pD3DDevice && g_pCore && g_pCore->GetGraphics())
    {
        m_pD3DDevice = g_pCore->GetGraphics()->GetDevice();
    }
    if (!m_pD3DDevice)
        return nullptr;

    // Direct cache hit
    auto it = m_textures.find(texturePath);
    if (it != m_textures.end())
        return it->second;

    // Normalize path separators to forward slash
    std::string normPath = texturePath;
    for (auto& c : normPath)
    {
        if (c == '\\')
            c = '/';
    }

    std::string bareName = normPath;
    size_t      lastSlash = bareName.find_last_of('/');
    if (lastSlash != std::string::npos)
        bareName = bareName.substr(lastSlash + 1);

    auto itBare = m_textures.find(bareName);
    if (itBare != m_textures.end())
    {
        m_textures[texturePath] = itBare->second;
        return itBare->second;
    }

    // Build candidate filenames for this texture
    std::vector<std::string> nameVariants;
    nameVariants.push_back(bareName);

    // .jpg <-> .jpeg conversion
    if (bareName.size() >= 5 && bareName.compare(bareName.size() - 5, 5, ".jpeg") == 0)
    {
        nameVariants.push_back(bareName.substr(0, bareName.size() - 5) + ".jpg");
    }
    else if (bareName.size() >= 4 && bareName.compare(bareName.size() - 4, 4, ".jpg") == 0)
    {
        nameVariants.push_back(bareName.substr(0, bareName.size() - 4) + ".jpeg");
    }

    // Check if filename contains Russian/Cyrillic prefix like "Без названия" or "Без_названия"
    // and provide clean ASCII variants (e.g. 72_20260721190836.png, 73_20260721190030.png)
    for (size_t i = 0; i < bareName.size(); ++i)
    {
        if (isdigit((unsigned char)bareName[i]))
        {
            std::string subVariant = bareName.substr(i);
            if (subVariant.find(".png") != std::string::npos || subVariant.find(".jpg") != std::string::npos || subVariant.find(".jpeg") != std::string::npos)
            {
                nameVariants.push_back(subVariant);
            }
            break;
        }
    }

    std::string              basePath = GetTMODBinPath();
    std::vector<std::string> searchDirs;
    if (!baseDir.empty())
    {
        searchDirs.push_back(baseDir);
        searchDirs.push_back(baseDir + "/textures");
        searchDirs.push_back(baseDir + "/.fbm");
    }
    searchDirs.push_back(basePath + "map_fbx/textures");
    searchDirs.push_back(CalcMTASAPath("map_fbx/textures"));
    searchDirs.push_back(basePath + "Bin/map_fbx/textures");
    searchDirs.push_back(CalcMTASAPath("Bin/map_fbx/textures"));
    searchDirs.push_back(basePath + "map_fbx");
    searchDirs.push_back(CalcMTASAPath("map_fbx"));
    searchDirs.push_back(basePath + "Bin/map_fbx");
    searchDirs.push_back(CalcMTASAPath("Bin/map_fbx"));
    searchDirs.push_back(basePath + "server/mods/deathmatch/resources/map_fbx/textures");
    searchDirs.push_back(CalcMTASAPath("server/mods/deathmatch/resources/map_fbx/textures"));
    searchDirs.push_back(basePath + "mods/deathmatch/resources/map_fbx/textures");
    searchDirs.push_back(CalcMTASAPath("mods/deathmatch/resources/map_fbx/textures"));
    searchDirs.push_back("map_fbx/textures");
    searchDirs.push_back("Bin/map_fbx/textures");

    std::string resolvedFile = "";
    for (const auto& variant : nameVariants)
    {
        if (std::ifstream(variant.c_str()).good())
        {
            resolvedFile = variant;
            break;
        }

        for (const auto& dir : searchDirs)
        {
            std::string candidate = dir + "/" + variant;
            if (std::ifstream(candidate.c_str()).good())
            {
                resolvedFile = candidate;
                break;
            }
        }
        if (!resolvedFile.empty())
            break;
    }

    if (resolvedFile.empty())
    {
        char dbgBuf[512];
        sprintf_s(dbgBuf, "[TMOD-WARN] Texture not found on disk: %s (bare: %s)\n", texturePath.c_str(), bareName.c_str());
        OutputDebugStringA(dbgBuf);
        m_textures[texturePath] = nullptr;
        m_textures[bareName] = nullptr;
        return nullptr;
    }

    IDirect3DTexture9* pTexture = nullptr;
    HRESULT            hr = D3DXCreateTextureFromFileA(m_pD3DDevice, resolvedFile.c_str(), &pTexture);
    if (SUCCEEDED(hr) && pTexture)
    {
        m_textures[texturePath] = pTexture;
        m_textures[bareName] = pTexture;
        m_textures[resolvedFile] = pTexture;

        char dbgBuf[512];
        sprintf_s(dbgBuf, "[TMOD] Loaded texture: %s -> %s\n", texturePath.c_str(), resolvedFile.c_str());
        OutputDebugStringA(dbgBuf);
        if (g_pCore && g_pCore->GetConsole())
        {
            g_pCore->GetConsole()->Printf("[TMOD] Loaded map texture: %s\n", bareName.c_str());
        }
        return pTexture;
    }
    else
    {
        char dbgBuf[512];
        sprintf_s(dbgBuf, "[TMOD-ERROR] D3DXCreateTextureFromFileA failed (0x%08X) for: %s\n", hr, resolvedFile.c_str());
        OutputDebugStringA(dbgBuf);
        m_textures[texturePath] = nullptr;
        m_textures[bareName] = nullptr;
        return nullptr;
    }
}

void CTModAssetManager::RenderMesh(STModMesh* pMesh, const STModProp& prop)
{
    if (!pMesh || !pMesh->vertexBuffer || !pMesh->indexBuffer)
        return;

    // Both OBJ maps and Bullet collision are Z-Up. Visual mesh world transform
    // directly matches prop rotation and translation without an artificial X-axis tilt.
    D3DXMATRIX matRotation, matTranslation, matWorld;

    // Lua'dan gelen Euler acilarini (RotX, RotY, RotZ) yaw-pitch-roll matrisine cevir
    // GTA rotasyon sistemi: Z(Yaw), X(Pitch), Y(Roll)
    D3DXMatrixRotationYawPitchRoll(&matRotation, prop.rotation.fZ * (D3DX_PI / 180.0f), prop.rotation.fX * (D3DX_PI / 180.0f),
                                   prop.rotation.fY * (D3DX_PI / 180.0f));

    D3DXMatrixTranslation(&matTranslation, prop.transform.vPos.fX, prop.transform.vPos.fY, prop.transform.vPos.fZ);

    // Birlestir: (CustomRotation) * (Translation)
    matWorld = matRotation * matTranslation;

    m_pD3DDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&matWorld);
    m_pD3DDevice->SetFVF(TMOD_FVF);
    m_pD3DDevice->SetStreamSource(0, pMesh->vertexBuffer, 0, sizeof(VertexPosNormalTex));
    m_pD3DDevice->SetIndices(pMesh->indexBuffer);

    // Ensure sampler states for textures: wrapping and linear filtering
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

    const UINT maxTrisPerCall = 32768;

    if (!pMesh->subsets.empty())
    {
        for (auto& subset : pMesh->subsets)
        {
            if (subset.indexCount == 0)
                continue;

            // Lazy texture resolve if not loaded earlier
            if (!subset.texture && !subset.textureName.empty())
            {
                subset.texture = GetOrCreateTexture(subset.textureName);
            }

            if (subset.texture)
            {
                m_pD3DDevice->SetTexture(0, subset.texture);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
                // Set full bright white for true texture colors
                m_pD3DDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 255, 255, 255));
            }
            else
            {
                m_pD3DDevice->SetTexture(0, NULL);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
                m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
                m_pD3DDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 205, 195, 180));
            }

            UINT totalSubsetTris = subset.indexCount / 3;
            for (UINT triOffset = 0; triOffset < totalSubsetTris; triOffset += maxTrisPerCall)
            {
                UINT numTris = std::min(maxTrisPerCall, totalSubsetTris - triOffset);
                UINT startIndex = subset.startIndex + (triOffset * 3);
                m_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pMesh->vertexCount, startIndex, numTris);
            }
        }
    }
    else
    {
        // Single material / untextured fallback
        if (pMesh->texture)
        {
            m_pD3DDevice->SetTexture(0, pMesh->texture);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            m_pD3DDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 255, 255, 255));
        }
        else
        {
            m_pD3DDevice->SetTexture(0, NULL);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
            m_pD3DDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 205, 195, 180));
        }

        UINT totalTriangles = static_cast<UINT>(pMesh->indexCount / 3);
        for (UINT triOffset = 0; triOffset < totalTriangles; triOffset += maxTrisPerCall)
        {
            UINT numTris = std::min(maxTrisPerCall, totalTriangles - triOffset);
            UINT startIndex = triOffset * 3;
            m_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, pMesh->vertexCount, startIndex, numTris);
        }
    }
}

void CTModAssetManager::SavePermaProps()
{
}
void CTModAssetManager::LoadPermaProps()
{
}

void CTModAssetManager::AutoLoadCityMap()
{
    // Check if city map prop is already spawned around (-315.19, -1204.00, 275.00)
    for (const auto& pair : m_props)
    {
        const STModProp& prop = pair.second;
        if (prop.bStatic && std::abs(prop.transform.vPos.fX - (-315.19327f)) < 5.0f && std::abs(prop.transform.vPos.fY - (-1203.99744f)) < 5.0f)
        {
            return;
        }
    }

    // Direct TMOD native map_fbx directory loading: prioritize .fbx to load full multi-material textures and UVs
    std::string modelHandle = LoadModel("map_fbx/city_map.fbx");
    if (modelHandle.empty())
        modelHandle = LoadModel("city_map.fbx");
    if (modelHandle.empty())
        modelHandle = LoadModel("map_fbx/city_map.obj");
    if (modelHandle.empty())
        modelHandle = LoadModel("city_map.obj");

    if (!modelHandle.empty())
    {
        int propId = SpawnProp(modelHandle, -315.19327f, -1203.99744f, 275.00000f, 0.0f, 0.0f, 0.0f, 0.0f, true);
        if (propId != -1 && g_pCore && g_pCore->GetConsole())
        {
            g_pCore->GetConsole()->Printf("[TMOD] FBX City Map auto-loaded successfully at (-315.2, -1204.0, 275.0) (Prop ID: %d)\n", propId);
        }
    }
}
