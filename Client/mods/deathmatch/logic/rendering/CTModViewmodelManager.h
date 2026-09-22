#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <vector>
#include <map>

class CTModSkeletalMesh;
class CTModAnimationManager;

/**
 * STModWeaponViewmodel
 *
 * Belirli bir silaha veya varsayılan çıplak ellere (unarmed) ait FBX modelini,
 * kaplamasını ve animasyon yöneticisini saklayan yapılandırma.
 * Silah değiştirildiğinde otomatik olarak ilgili FBX modeline geçiş yapılır.
 */
struct STModWeaponViewmodel
{
    std::string            fbxPath;
    std::string            texturePath;
    CTModSkeletalMesh*     pMesh = nullptr;
    CTModAnimationManager* pAnimManager = nullptr;
    bool                   bAttemptedLoad = false;
    bool                   bHasSkeletalAnim = false;
    float                  animDuration = 1.2f;

    float scaleForward = 1.45f;
    float scaleUp = 1.15f;
    float scaleRight = 1.15f;

    float offsetForward = 0.38f;
    float offsetDown = -0.22f;
    float offsetRight = 0.0f;
    float pitchDeg = -12.0f;
};

/**
 * CTModViewmodelManager
 *
 * Source Engine / Counter-Strike ve Garry's Mod tarzı First Person Viewmodel ellerini ve
 * silah modellerini kamera uzayında yönetir, uzatılmış kolları çizer ve animasyon döngüsünü işletir.
 *
 * Neden var?
 * - FPS modunda oyuncunun ellerini doğal ve uzatılmış boyutta gösterir (kesik dirseklerin kırpılmasını önler).
 * - İlk açılışta statik duruşta (Static Pose) bekler, her 3 saniyede bir CINEMA_4D_Main / silah animasyonunu oynatır.
 * - Oyuncu silah değiştirdiğinde ilgili silahın FBX modeline dinamik olarak geçiş yapar.
 * - Duvara gömülmeyi engellemek için D3D9 Depth Hack (Z-Clip aralığı) uygular.
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

    bool IsLoaded() const;

    // Silah viewmodel modeli yapılandırma API'si
    void SetWeaponViewmodel(int weaponType, const std::string& fbxPath, const std::string& texturePath);
    void SetViewmodelOffset(float right, float forward, float down, float pitchDeg);
    void SetViewmodelScale(float scaleForward, float scaleUp, float scaleRight);
    void SetAnimInterval(float intervalSeconds) { m_fAnimInterval = intervalSeconds; }
    void SetAnimName(const std::string& animName) { m_strAnimName = animName; }

    int GetCurrentWeaponType() const { return m_nCurrentWeaponType; }

private:
    CTModViewmodelManager();
    ~CTModViewmodelManager();

    std::string ResolveAssetPath(const std::string& relPath);
    void        LoadWeaponMesh(STModWeaponViewmodel& entry, IDirect3DDevice9* pDevice);

    // Silah modelleri kayıt tablosu (Weapon Type -> Model Entry)
    int                                 m_nCurrentWeaponType = 0;  // 0 = Unarmed / Default Hands
    std::map<int, STModWeaponViewmodel> m_weaponRegistry;

    // Animasyon durum makinesi (Her 3 saniyede bir tetiklenir)
    float       m_fAnimTimer = 0.0f;         // 3 saniyelik geri sayım sayacı
    float       m_fAnimInterval = 3.0f;      // Animasyon periyodu (varsayılan: 3.0 saniye)
    bool        m_bIsPlayingAnim = false;    // Animasyon aktif oynatılıyor mu?
    float       m_fAnimPlaybackTime = 0.0f;  // Mevcut oynatma süresi (saniye)
    std::string m_strAnimName = "CINEMA_4D_Main";

    // Genel geometri ve uzatma çarpanları
    float m_fCustomOffsetRight = 0.0f;
    float m_fCustomOffsetForward = 0.38f;
    float m_fCustomOffsetDown = -0.22f;
    float m_fCustomPitchDeg = -12.0f;

    float m_fCustomScaleForward = 1.45f;
    float m_fCustomScaleUp = 1.15f;
    float m_fCustomScaleRight = 1.15f;

    // Prosedürel nefes alma ve yürüme salınımları
    float m_fTime = 0.0f;
    float m_fWalkTime = 0.0f;
};
