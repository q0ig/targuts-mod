#pragma once
#include <string>
#include <vector>
#include <map>
#include <d3d9.h>
#include <d3dx9.h>
#include <CMatrix.h>

class btCollisionShape;
class btTriangleIndexVertexArray;

struct VertexPosNormalTex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

#define TMOD_FVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

struct STModSubset
{
    uint32_t           startIndex = 0;
    uint32_t           indexCount = 0;
    IDirect3DTexture9* texture = nullptr;
    std::string        textureName;
};

struct STModMesh
{
    IDirect3DVertexBuffer9* vertexBuffer = nullptr;
    IDirect3DIndexBuffer9*  indexBuffer = nullptr;
    int                     vertexCount = 0;
    int                     indexCount = 0;
    IDirect3DTexture9*      texture = nullptr;

    bool bIndex32 = false;

    // Coklu materyal/doku alt parcalari (FBX haritalar ve modeller icin)
    std::vector<STModSubset> subsets;

    // Collision verileri (Bullet Physics)
    std::vector<float>          vertices;
    std::vector<int>            indices;
    btCollisionShape*           bulletShape = nullptr;
    btTriangleIndexVertexArray* bulletMeshInterface = nullptr;

    // Direct3D 9 ham veri yedegi (cihaz kaybinda veya gecikmeli yuklemede tekrar olusturabilmek icin)
    std::vector<VertexPosNormalTex> rawVertexData;
    std::vector<uint32_t>           rawIndexData;
};

struct STModProp
{
    int         id = 0;
    std::string modelHandle;
    int         physicsBodyId = 0;
    CMatrix     transform;
    CVector     rotation;
    bool        bStatic = false;
    bool        bPermanent = false;
};

class CTModAssetManager
{
public:
    static CTModAssetManager& GetSingleton()
    {
        static CTModAssetManager instance;
        return instance;
    }

    void Init();
    void Shutdown();
    void Render();  // D3D9 Render Hook

    std::string LoadModel(const std::string& filePath);
    int         SpawnProp(const std::string& modelHandle, float x, float y, float z, float rx, float ry, float rz, float mass, bool isStatic);
    void        SetPropPerma(int propId, bool permanent);

    // Otomatik sehir haritasi yukleyici (map_fbx yerel dizininden)
    void AutoLoadCityMap();

    // Doku yonetimi (D3D9)
    IDirect3DTexture9* GetOrCreateTexture(const std::string& texturePath, const std::string& baseDir = "");

    void SavePermaProps();
    void LoadPermaProps();

private:
    CTModAssetManager() = default;
    ~CTModAssetManager() = default;

    void RenderMesh(STModMesh* pMesh, const STModProp& prop);
    void CreateD3DBuffers(STModMesh* pMesh);

    std::map<std::string, STModMesh*>         m_models;
    std::map<std::string, IDirect3DTexture9*> m_textures;
    std::map<int, STModProp>                  m_props;
    int                                       m_nextPropId = 1;

    IDirect3DDevice9* m_pD3DDevice = nullptr;
};
