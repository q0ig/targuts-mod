#include "StdInc.h"
#include "CTModViewmodelManager.h"
#include "CTModSkeletalMesh.h"
#include "../animation/CTModAnimationManager.h"
#include "../camera/CTModCameraManager.h"
#include "CClientGame.h"
#include "CClientPlayerManager.h"
#include "CClientPlayer.h"
#include "CClientCamera.h"
#include <core/CCoreInterface.h>
#include <windows.h>
#include <fstream>
#include <cmath>
#include <algorithm>

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

std::string CTModViewmodelManager::ResolveAssetPath(const std::string& relPath)
{
    std::string              basePath = GetTMODBinPath();
    std::vector<std::string> candidates = {
        basePath + relPath,
        CalcMTASAPath(relPath),
        CalcMTASAPath("mods/deathmatch/" + relPath),
        basePath + "mods/deathmatch/" + relPath,
        CalcMTASAPath("server/mods/deathmatch/" + relPath),
        basePath + "server/mods/deathmatch/" + relPath,
    };
    for (const auto& cand : candidates)
    {
        if (std::ifstream(cand.c_str()).good())
        {
            return cand;
        }
    }
    return basePath + relPath;
}

CTModViewmodelManager::CTModViewmodelManager()
{
}

CTModViewmodelManager::~CTModViewmodelManager()
{
    Shutdown();
}

void CTModViewmodelManager::Init()
{
    m_fTime = 0.0f;
    m_fWalkTime = 0.0f;
    m_fAnimTimer = 0.0f;
    m_bIsPlayingAnim = false;
    m_fAnimPlaybackTime = 0.0f;
    m_nCurrentWeaponType = 0;

    // Slot 0 (Unarmed/Fist): Varsayılan birinci şahıs elleri (arms.fbx)
    STModWeaponViewmodel defaultHands;
    defaultHands.fbxPath = "first-person/source/arms.fbx";
    defaultHands.texturePath = "first-person/textures/material_baseColor.png";
    defaultHands.scaleForward = m_fCustomScaleForward;
    defaultHands.scaleUp = m_fCustomScaleUp;
    defaultHands.scaleRight = m_fCustomScaleRight;
    defaultHands.offsetForward = m_fCustomOffsetForward;
    defaultHands.offsetDown = m_fCustomOffsetDown;
    defaultHands.offsetRight = m_fCustomOffsetRight;
    defaultHands.pitchDeg = m_fCustomPitchDeg;
    m_weaponRegistry[0] = defaultHands;
}

void CTModViewmodelManager::Shutdown()
{
    for (auto& [id, entry] : m_weaponRegistry)
    {
        if (entry.pAnimManager)
        {
            delete entry.pAnimManager;
            entry.pAnimManager = nullptr;
        }
        if (entry.pMesh)
        {
            delete entry.pMesh;
            entry.pMesh = nullptr;
        }
        entry.bAttemptedLoad = false;
    }
    m_weaponRegistry.clear();
}

bool CTModViewmodelManager::IsLoaded() const
{
    auto it = m_weaponRegistry.find(m_nCurrentWeaponType);
    if (it != m_weaponRegistry.end() && it->second.pMesh != nullptr)
        return true;
    auto defIt = m_weaponRegistry.find(0);
    return (defIt != m_weaponRegistry.end() && defIt->second.pMesh != nullptr);
}

void CTModViewmodelManager::SetWeaponViewmodel(int weaponType, const std::string& fbxPath, const std::string& texturePath)
{
    STModWeaponViewmodel entry;
    entry.fbxPath = fbxPath;
    entry.texturePath = texturePath;
    entry.scaleForward = m_fCustomScaleForward;
    entry.scaleUp = m_fCustomScaleUp;
    entry.scaleRight = m_fCustomScaleRight;
    entry.offsetForward = m_fCustomOffsetForward;
    entry.offsetDown = m_fCustomOffsetDown;
    entry.offsetRight = m_fCustomOffsetRight;
    entry.pitchDeg = m_fCustomPitchDeg;

    // Eğer eski bir mesh varsa temizle
    auto it = m_weaponRegistry.find(weaponType);
    if (it != m_weaponRegistry.end())
    {
        if (it->second.pAnimManager)
            delete it->second.pAnimManager;
        if (it->second.pMesh)
            delete it->second.pMesh;
    }

    m_weaponRegistry[weaponType] = entry;

    if (g_pCore && g_pCore->GetConsole())
    {
        g_pCore->GetConsole()->Printf("[TMOD::VIEWMODEL] Weapon slot %d bound to FBX: %s", weaponType, fbxPath.c_str());
    }
}

void CTModViewmodelManager::SetViewmodelOffset(float right, float forward, float down, float pitchDeg)
{
    m_fCustomOffsetRight = right;
    m_fCustomOffsetForward = forward;
    m_fCustomOffsetDown = down;
    m_fCustomPitchDeg = pitchDeg;

    for (auto& [id, entry] : m_weaponRegistry)
    {
        entry.offsetRight = right;
        entry.offsetForward = forward;
        entry.offsetDown = down;
        entry.pitchDeg = pitchDeg;
    }
}

void CTModViewmodelManager::SetViewmodelScale(float scaleForward, float scaleUp, float scaleRight)
{
    m_fCustomScaleForward = scaleForward;
    m_fCustomScaleUp = scaleUp;
    m_fCustomScaleRight = scaleRight;

    for (auto& [id, entry] : m_weaponRegistry)
    {
        entry.scaleForward = scaleForward;
        entry.scaleUp = scaleUp;
        entry.scaleRight = scaleRight;
    }
}

void CTModViewmodelManager::DoPulse(float fDeltaTime)
{
    if (CTModCameraManager::GetSingleton().GetMode() != ETModCameraMode::FIRST_PERSON)
        return;

    m_fTime += fDeltaTime;

    // 1. Oyuncu ve Silah Değişim Tespiti
    if (g_pClientGame && g_pClientGame->GetPlayerManager())
    {
        CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
        if (pLocalPlayer && !pLocalPlayer->IsDead())
        {
            int currentWeapon = (int)pLocalPlayer->GetCurrentWeaponType();
            if (currentWeapon != m_nCurrentWeaponType)
            {
                // Silah değiştiğinde yeni silah modeline geç ve animasyon döngüsünü sıfırla
                m_nCurrentWeaponType = currentWeapon;
                m_fAnimTimer = 0.0f;
                m_bIsPlayingAnim = false;
                m_fAnimPlaybackTime = 0.0f;
            }

            // Oyuncu hızına bağlı yürüme sallantısı
            CVector vel;
            pLocalPlayer->GetMoveSpeed(vel);
            float moveSpeed = sqrtf(vel.fX * vel.fX + vel.fY * vel.fY) * 50.0f;
            if (moveSpeed > 0.1f)
            {
                m_fWalkTime += fDeltaTime * std::min(moveSpeed, 6.0f);
            }
        }
    }

    // 2. Her 3 saniyede bir animasyon oynatma zamanlayıcısı (Anim Interval Machine)
    auto it = m_weaponRegistry.find(m_nCurrentWeaponType);
    if (it == m_weaponRegistry.end())
        it = m_weaponRegistry.find(0);

    float activeAnimDuration = 1.2f;
    if (it != m_weaponRegistry.end() && it->second.animDuration > 0.01f)
    {
        activeAnimDuration = it->second.animDuration;
    }

    if (!m_bIsPlayingAnim)
    {
        // Statik Pozisyon: 3 saniye dolana kadar bekle
        m_fAnimTimer += fDeltaTime;
        if (m_fAnimTimer >= m_fAnimInterval)
        {
            m_bIsPlayingAnim = true;
            m_fAnimPlaybackTime = 0.0f;
            m_fAnimTimer = 0.0f;
        }
    }
    else
    {
        // Animasyon devrede: Oynat ve süre bitince tekrar Statik Duruşa geç
        m_fAnimPlaybackTime += fDeltaTime;
        if (m_fAnimPlaybackTime >= activeAnimDuration)
        {
            m_bIsPlayingAnim = false;
            m_fAnimPlaybackTime = 0.0f;
            m_fAnimTimer = 0.0f;  // Yeniden 3 saniye sayacak
        }
    }
}

void CTModViewmodelManager::LoadWeaponMesh(STModWeaponViewmodel& entry, IDirect3DDevice9* pDevice)
{
    if (entry.bAttemptedLoad)
        return;
    entry.bAttemptedLoad = true;

    std::string fbxResolved = ResolveAssetPath(entry.fbxPath);
    std::string texResolved = ResolveAssetPath(entry.texturePath);

    if (!std::ifstream(texResolved.c_str()).good())
        texResolved = ResolveAssetPath("first-person/textures/material_baseColor.png");
    if (!std::ifstream(texResolved.c_str()).good())
        texResolved = ResolveAssetPath("first-person/textures/arm1Color.png");
    if (!std::ifstream(texResolved.c_str()).good())
        texResolved = ResolveAssetPath("first-person/textures/armColor.png");

    entry.pMesh = new CTModSkeletalMesh();
    if (!entry.pMesh->LoadFBX(fbxResolved, pDevice))
    {
        char errBuf[512];
        sprintf_s(errBuf, "[TMOD-ERROR] Failed to load viewmodel FBX: %s\n", fbxResolved.c_str());
        OutputDebugStringA(errBuf);
        if (g_pCore && g_pCore->GetConsole())
            g_pCore->GetConsole()->Printf("%s", errBuf);

        delete entry.pMesh;
        entry.pMesh = nullptr;
        return;
    }
    entry.pMesh->LoadTexture(texResolved, pDevice);

    // Animasyon Yöneticisi başlat
    entry.pAnimManager = new CTModAnimationManager(entry.pMesh);
    if (entry.pAnimManager->LoadAnimation(m_strAnimName, fbxResolved))
    {
        entry.bHasSkeletalAnim = true;
        entry.animDuration = entry.pAnimManager->GetAnimationDuration(m_strAnimName);
        if (entry.animDuration <= 0.05f)
            entry.animDuration = 1.2f;
    }
    else
    {
        // Dosyada iskelet animasyonu yoksa prosedürel animasyon kullanılır
        entry.bHasSkeletalAnim = false;
        entry.animDuration = 1.2f;
    }
}

void CTModViewmodelManager::Render(IDirect3DDevice9* pDevice)
{
    // YALNIZCA First Person kamera modu aktifken çiz
    if (CTModCameraManager::GetSingleton().GetMode() != ETModCameraMode::FIRST_PERSON)
        return;

    if (!pDevice)
        return;

    if (!g_pClientGame || !g_pClientGame->GetManager())
        return;

    CClientCamera* pCamera = g_pClientGame->GetManager()->GetCamera();
    if (!pCamera)
        return;

    CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager() ? g_pClientGame->GetPlayerManager()->GetLocalPlayer() : nullptr;
    if (!pLocalPlayer || pLocalPlayer->IsDead())
        return;

    // Aktif silah modeli yapılandırmasını bul
    auto it = m_weaponRegistry.find(m_nCurrentWeaponType);
    if (it == m_weaponRegistry.end())
    {
        // Eğer bu silah için özel tanımlı model yoksa varsayılan eller (0) kullanılır
        it = m_weaponRegistry.find(0);
    }

    if (it == m_weaponRegistry.end())
        return;

    STModWeaponViewmodel& activeEntry = it->second;

    // Gerekliyse mesh'i yükle
    if (!activeEntry.pMesh && !activeEntry.bAttemptedLoad)
    {
        LoadWeaponMesh(activeEntry, pDevice);
    }

    if (!activeEntry.pMesh)
        return;

    CMatrix camMat;
    pCamera->GetMatrix(camMat);

    // Hız tespiti
    CVector vel(0, 0, 0);
    pLocalPlayer->GetMoveSpeed(vel);
    float moveSpeed = sqrtf(vel.fX * vel.fX + vel.fY * vel.fY) * 50.0f;

    // --- PROSEDÜREL SALINIM & YÜRÜME BOBBING ---
    float breathY = sinf(m_fTime * 1.5f) * 0.002f;
    float breathX = cosf(m_fTime * 0.75f) * 0.0015f;

    float bobY = 0.0f;
    float bobX = 0.0f;
    if (moveSpeed > 0.1f)
    {
        bobY = -fabsf(sinf(m_fWalkTime * 7.0f)) * 0.005f;
        bobX = cosf(m_fWalkTime * 3.5f) * 0.003f;
    }

    // --- HER 3 SANİYEDE BİR OYNAYAN ANİMASYON / İNCELEME HAREKETİ ---
    float animLift = 0.0f;
    float animSwayX = 0.0f;
    float animPitchExtra = 0.0f;

    if (m_bIsPlayingAnim && !activeEntry.bHasSkeletalAnim)
    {
        // İskelet animasyonu olmayan modeller için pürüzsüz prosedürel inceleme (inspect flourish)
        float tNorm = m_fAnimPlaybackTime / activeEntry.animDuration;
        float bellWeight = sinf(tNorm * 3.14159265f);  // 0 -> 1 -> 0 eğrisi
        animLift = bellWeight * 0.025f;                // 2.5 cm yukarı kaldırma
        animSwayX = sinf(tNorm * 3.14159265f * 2.0f) * 0.012f;
        animPitchExtra = bellWeight * 5.5f;  // 5.5 derece doğal yukarı eğim
    }

    // --- KAMERA UZAYI VE KOL UZATMA TRANSFORMASYONU ---
    // scaleForward = 1.45f: Kolları ileriye doğru uzatır (41 cm'den ~60 cm'ye çıkarır, kısa durmayı engeller).
    // pitchDeg = -12.0f: Kolları aşağıya doğru doğal bir açıyla eğer; kesik dirsek uçlarını vizörün tamamen altına saklar.
    // offsetForward = 0.38f, offsetDown = -0.22f: Elleri ekranın alt-orta kısmında mükemmel bir perspektifte tutar.
    float totalRight = activeEntry.offsetRight + breathX + bobX + animSwayX;
    float totalForward = activeEntry.offsetForward;
    float totalDown = activeEntry.offsetDown + breathY + bobY + animLift;

    float totalPitchRad = D3DXToRadian(activeEntry.pitchDeg + animPitchExtra);
    float cosP = cosf(totalPitchRad);
    float sinP = sinf(totalPitchRad);

    float sFwd = activeEntry.scaleForward;
    float sUp = activeEntry.scaleUp;
    float sRight = activeEntry.scaleRight;

    // Model yerel merkezleri (arms.fbx)
    const float centerFwd = 0.295f;
    const float centerUp = 0.433f;

    // Kamera yön vektörleri
    CVector cFront(camMat.vFront.fX, camMat.vFront.fY, camMat.vFront.fZ);
    CVector cUp(camMat.vUp.fX, camMat.vUp.fY, camMat.vUp.fZ);
    CVector cRight(camMat.vRight.fX, camMat.vRight.fY, camMat.vRight.fZ);

    // Eğim ve Uzatma Uygulanmış Kamera Uzayı Eksenleri:
    // Local X (İleri) -> (camFront * cosP + camUp * sinP) * sFwd
    CVector axX = (cFront * cosP + cUp * sinP) * sFwd;
    // Local Y (Yukarı) -> (-camFront * sinP + camUp * cosP) * sUp
    CVector axY = (cFront * (-sinP) + cUp * cosP) * sUp;
    // Local Z (Sol/Sağ: +Z Sol Kol, -Z Sağ Kol) -> (-camRight) * sRight
    CVector axZ = (cRight * (-1.0f)) * sRight;

    D3DXMATRIX fpWorld;
    D3DXMatrixIdentity(&fpWorld);

    fpWorld._11 = axX.fX;
    fpWorld._12 = axX.fY;
    fpWorld._13 = axX.fZ;
    fpWorld._14 = 0.0f;

    fpWorld._21 = axY.fX;
    fpWorld._22 = axY.fY;
    fpWorld._23 = axY.fZ;
    fpWorld._24 = 0.0f;

    fpWorld._31 = axZ.fX;
    fpWorld._32 = axZ.fY;
    fpWorld._33 = axZ.fZ;
    fpWorld._34 = 0.0f;

    // Dünya Konumu: Modelin merkezini ofsetleyerek elleri tam istenen mesafeye yerleştir
    CVector targetPos = camMat.vPos + cRight * totalRight + cFront * totalForward + cUp * totalDown;
    targetPos.fX -= (axX.fX * centerFwd + axY.fX * centerUp);
    targetPos.fY -= (axX.fY * centerFwd + axY.fY * centerUp);
    targetPos.fZ -= (axX.fZ * centerFwd + axY.fZ * centerUp);

    fpWorld._41 = targetPos.fX;
    fpWorld._42 = targetPos.fY;
    fpWorld._43 = targetPos.fZ;
    fpWorld._44 = 1.0f;

    // --- KEMİK / ANİMASYON MATRİSLERİ GÜNCELLEMESİ ---
    std::vector<D3DXMATRIX> boneMatrices(60);
    D3DXMATRIX              identityMat;
    D3DXMatrixIdentity(&identityMat);
    for (size_t i = 0; i < boneMatrices.size(); i++)
    {
        boneMatrices[i] = identityMat;
    }

    if (m_bIsPlayingAnim && activeEntry.bHasSkeletalAnim && activeEntry.pAnimManager)
    {
        // 3 saniyede bir gelen FBX iskelet animasyonunu oynat
        activeEntry.pAnimManager->PlaySingleAnimation(m_strAnimName, m_fAnimPlaybackTime, boneMatrices);
    }
    // Aksi halde (Static Pose): Kemik matrisleri Identity kalır (sıfır bozulma / saf statik duruş)

    activeEntry.pMesh->UpdateBoneMatrices(pDevice, boneMatrices);

    // Shader sabitleri (c4: World, c0: ViewProj)
    pDevice->SetVertexShaderConstantF(4, (float*)&fpWorld, 4);

    D3DMATRIX matGtaView, matGtaProj;
    pDevice->GetTransform(D3DTS_VIEW, &matGtaView);
    pDevice->GetTransform(D3DTS_PROJECTION, &matGtaProj);
    D3DXMATRIX viewProj = (*(const D3DXMATRIX*)&matGtaView) * (*(const D3DXMATRIX*)&matGtaProj);
    pDevice->SetVertexShaderConstantF(0, (const float*)&viewProj, 4);

    // --- D3D9 DEPTH HACK (Z-CLIP / DUVARA GÖMÜLME ENGELİ) ---
    D3DVIEWPORT9 origVp, vmVp;
    pDevice->GetViewport(&origVp);
    vmVp = origVp;
    vmVp.MinZ = 0.0f;
    vmVp.MaxZ = 0.25f;
    pDevice->SetViewport(&vmVp);

    DWORD origCull;
    pDevice->GetRenderState(D3DRS_CULLMODE, &origCull);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    activeEntry.pMesh->Render(pDevice);

    pDevice->SetRenderState(D3DRS_CULLMODE, origCull);
    pDevice->SetViewport(&origVp);
}
