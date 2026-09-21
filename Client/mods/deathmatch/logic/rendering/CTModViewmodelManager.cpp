#include "StdInc.h"
#include "CTModViewmodelManager.h"
#include "CTModSkeletalMesh.h"
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

void CTModViewmodelManager::Init()
{
    m_fTime = 0.0f;
    m_fWalkTime = 0.0f;
}

void CTModViewmodelManager::Shutdown()
{
    if (m_pArmsMesh)
    {
        delete m_pArmsMesh;
        m_pArmsMesh = nullptr;
    }
    m_bAttemptedLoad = false;
}

void CTModViewmodelManager::DoPulse(float fDeltaTime)
{
    if (CTModCameraManager::GetSingleton().GetMode() != ETModCameraMode::FIRST_PERSON)
        return;

    m_fTime += fDeltaTime;

    // Oyuncu hızına bağlı yürüme sallantısı
    float moveSpeed = 0.0f;
    if (g_pClientGame && g_pClientGame->GetPlayerManager())
    {
        CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
        if (pLocalPlayer && !pLocalPlayer->IsDead())
        {
            CVector vel;
            pLocalPlayer->GetMoveSpeed(vel);
            moveSpeed = sqrtf(vel.fX * vel.fX + vel.fY * vel.fY) * 50.0f;
        }
    }

    if (moveSpeed > 0.1f)
    {
        m_fWalkTime += fDeltaTime * std::min(moveSpeed, 6.0f);
    }
}

void CTModViewmodelManager::Render(IDirect3DDevice9* pDevice)
{
    // YALNIZCA First Person kamera modu aktifken çiz
    if (CTModCameraManager::GetSingleton().GetMode() != ETModCameraMode::FIRST_PERSON)
        return;

    if (!pDevice)
        return;

    // Lazy load: Viewmodel kollarını ilk ihtiyaç duyulduğunda yükle
    if (!m_pArmsMesh && !m_bAttemptedLoad)
    {
        m_bAttemptedLoad = true;
        std::string fbxPath = ResolveAssetPath("first-person/source/arms.fbx");
        std::string texPath = ResolveAssetPath("first-person/textures/material_baseColor.png");
        if (!std::ifstream(texPath.c_str()).good())
            texPath = ResolveAssetPath("first-person/textures/arm1Color.png");
        if (!std::ifstream(texPath.c_str()).good())
            texPath = ResolveAssetPath("first-person/textures/armColor.png");

        m_pArmsMesh = new CTModSkeletalMesh();
        if (!m_pArmsMesh->LoadFBX(fbxPath, pDevice))
        {
            char errBuf[512];
            sprintf_s(errBuf, "[TMOD-ERROR] Failed to load viewmodel FBX: %s\n", fbxPath.c_str());
            OutputDebugStringA(errBuf);
            if (g_pCore && g_pCore->GetConsole())
                g_pCore->GetConsole()->Printf("%s", errBuf);

            delete m_pArmsMesh;
            m_pArmsMesh = nullptr;
            return;
        }
        m_pArmsMesh->LoadTexture(texPath, pDevice);
    }

    if (!m_pArmsMesh)
        return;

    if (!g_pClientGame || !g_pClientGame->GetManager())
        return;

    CClientCamera* pCamera = g_pClientGame->GetManager()->GetCamera();
    if (!pCamera)
        return;

    CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager() ? g_pClientGame->GetPlayerManager()->GetLocalPlayer() : nullptr;
    if (!pLocalPlayer || pLocalPlayer->IsDead())
        return;

    CMatrix camMat;
    pCamera->GetMatrix(camMat);

    // Hız tespiti
    CVector vel(0, 0, 0);
    pLocalPlayer->GetMoveSpeed(vel);
    float moveSpeed = sqrtf(vel.fX * vel.fX + vel.fY * vel.fY) * 50.0f;

    // --- PROSEDÜREL SALINIM & YÜRÜME BOBBING ---
    // Nefes alma salınımı (Breathing sway)
    float breathY = sinf(m_fTime * 1.5f) * 0.002f;
    float breathX = cosf(m_fTime * 0.75f) * 0.0015f;

    // Yürüme sallantısı (Walk bobbing)
    float bobY = 0.0f;
    float bobX = 0.0f;
    if (moveSpeed > 0.1f)
    {
        bobY = -fabsf(sinf(m_fWalkTime * 7.0f)) * 0.005f;
        bobX = cosf(m_fWalkTime * 3.5f) * 0.003f;
    }

    // Kamera uzayı yerleşim ofsetleri (Metre cinsinden):
    // CS / Garry's Mod tarzında iki el ekranın alt-orta kısmında doğal bir açıyla durur.
    // totalRight: 0.0f ile sol ve sağ kollar vizör merkezine göre tam simetrik dengelenir.
    // totalForward: 0.32f ile kollar kameranın 32 cm önünde durur (bilekler 11 cm, parmaklar 53 cm).
    // totalDown: -0.15f ile kollar göz hizasının 15 cm altında durur; nişangahı ve ekranı asla kapatmaz.
    float totalRight = 0.0f + breathX + bobX;
    float totalForward = 0.32f;
    float totalDown = -0.15f + breathY + bobY;

    // Modelin yerel uzaydaki geometrik merkezleri (arms.fbx):
    // X (İleri ekseni): [0.086m, 0.504m], merkez = 0.295m
    // Y (Yukarı ekseni): [0.355m, 0.511m], merkez = 0.433m
    // Z (Sol/Sağ ekseni): [-0.211m, +0.211m], merkez = 0.000m (+Z: Sol kol, -Z: Sağ kol)
    const float centerFwd = 0.295f;
    const float centerUp = 0.433f;

    D3DXMATRIX fpWorld;
    D3DXMatrixIdentity(&fpWorld);

    // Satır 1: Local X (İleri ekseni) -> +camFront
    fpWorld._11 = camMat.vFront.fX;
    fpWorld._12 = camMat.vFront.fY;
    fpWorld._13 = camMat.vFront.fZ;
    fpWorld._14 = 0.0f;

    // Satır 2: Local Y (Yukarı ekseni) -> +camUp
    fpWorld._21 = camMat.vUp.fX;
    fpWorld._22 = camMat.vUp.fY;
    fpWorld._23 = camMat.vUp.fZ;
    fpWorld._24 = 0.0f;

    // Satır 3: Local Z (Sol/Sağ ekseni: +Z Sol, -Z Sağ) -> -camRight
    // Sol kol (+Z) -> -camRight (Kameranın Solu)
    // Sağ kol (-Z) -> +camRight (Kameranın Sağı)
    fpWorld._31 = -camMat.vRight.fX;
    fpWorld._32 = -camMat.vRight.fY;
    fpWorld._33 = -camMat.vRight.fZ;
    fpWorld._34 = 0.0f;

    // Satır 4: Dünya konumu (Kamera Konumu + Kamera Uzayı Merkezli Ofsetler)
    fpWorld._41 = camMat.vPos.fX + camMat.vRight.fX * totalRight + camMat.vFront.fX * (totalForward - centerFwd) + camMat.vUp.fX * (totalDown - centerUp);
    fpWorld._42 = camMat.vPos.fY + camMat.vRight.fY * totalRight + camMat.vFront.fY * (totalForward - centerFwd) + camMat.vUp.fY * (totalDown - centerUp);
    fpWorld._43 = camMat.vPos.fZ + camMat.vRight.fZ * totalRight + camMat.vFront.fZ * (totalForward - centerFwd) + camMat.vUp.fZ * (totalDown - centerUp);
    fpWorld._44 = 1.0f;

    // --- BIND-POSE STABİLİTESİ ---
    // Model saf statik eller olarak yüklendiği için shader kemik sabitlerine
    // Identity matrisi verilir. Böylece deformasyon, burkulma veya titreme sıfırlanır.
    std::vector<D3DXMATRIX> bindPose(60);
    for (size_t i = 0; i < bindPose.size(); i++)
    {
        D3DXMatrixIdentity(&bindPose[i]);
    }
    m_pArmsMesh->UpdateBoneMatrices(pDevice, bindPose);

    // Shader sabitleri (c4: World, c0: ViewProj)
    pDevice->SetVertexShaderConstantF(4, (float*)&fpWorld, 4);

    D3DMATRIX matGtaView, matGtaProj;
    pDevice->GetTransform(D3DTS_VIEW, &matGtaView);
    pDevice->GetTransform(D3DTS_PROJECTION, &matGtaProj);
    D3DXMATRIX viewProj = (*(const D3DXMATRIX*)&matGtaView) * (*(const D3DXMATRIX*)&matGtaProj);
    pDevice->SetVertexShaderConstantF(0, (const float*)&viewProj, 4);

    // --- D3D9 DEPTH HACK (Z-CLIP / DUVARA GÖMÜLME ENGELİ) ---
    // Viewmodel'in Z-Buffer derinlik aralığını [0.0f, 0.25f] aralığına sıkıştırarak
    // duvarların veya nesnelerin kolların içinden geçmesini (clipping) engelle.
    D3DVIEWPORT9 origVp, vmVp;
    pDevice->GetViewport(&origVp);
    vmVp = origVp;
    vmVp.MinZ = 0.0f;
    vmVp.MaxZ = 0.25f;
    pDevice->SetViewport(&vmVp);

    // Arka yüzey kırpmasını kapat (İki taraflı çizim) böylece modelin hiçbir parçası görünmez olmaz
    DWORD origCull;
    pDevice->GetRenderState(D3DRS_CULLMODE, &origCull);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    m_pArmsMesh->Render(pDevice);

    // Durumları geri yükle
    pDevice->SetRenderState(D3DRS_CULLMODE, origCull);
    pDevice->SetViewport(&origVp);
}
