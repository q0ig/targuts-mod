#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>

class CTModSkeletalMesh;

/**
 * CTModViewmodelManager
 *
 * Source Engine / Counter-Strike ve Garry's Mod tarzı First Person Viewmodel ellerini (arms.fbx)
 * sadece yerel oyuncunun ekranında kamera uzayında (Camera Space) yönetir ve çizer.
 *
 * Neden var?
 * - FPS modundayken yerel oyuncunun 3. şahıs modelinin kamerayı engellemesini önler.
 * - Duvara gömülmeyi engellemek için D3D9 Depth Hack (Z-Clip aralığı) uygular.
 * - Gerçekçi iki el (sağ ve sol simetrik) Counter-Strike / GMod birinci şahıs perspektifi.
 * - Doğal prosedürel nefes alma (breathing sway) ve adım salınımı (walk bobbing).
 */
class CTModViewmodelManager
{
public:
    static CTModViewmodelManager& GetSingleton()
    {
        static CTModViewmodelManager instance;
        return instance;
    }

    void Init();
    void Shutdown();
    void DoPulse(float fDeltaTime);
    void Render(IDirect3DDevice9* pDevice);

    bool IsLoaded() const { return m_pArmsMesh != nullptr; }

private:
    CTModViewmodelManager() = default;
    ~CTModViewmodelManager() = default;

    std::string ResolveAssetPath(const std::string& relPath);

    CTModSkeletalMesh* m_pArmsMesh = nullptr;
    bool               m_bAttemptedLoad = false;

    // Prosedürel nefes alma ve yürüme salınımları
    float m_fTime = 0.0f;
    float m_fWalkTime = 0.0f;
};
