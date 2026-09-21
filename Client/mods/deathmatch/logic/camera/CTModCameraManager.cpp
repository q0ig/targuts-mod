#include "StdInc.h"
#include "CTModCameraManager.h"
#include "CClientGame.h"
#include "CClientCamera.h"
#include "CClientPlayerManager.h"
#include "CClientPlayer.h"
#include "CClientPed.h"
#include "../physics/CTModPhysicsManager.h"
#include <game/CPlayerPed.h>
#include <game/CWorld.h>
#include <game/CColPoint.h>
#include <game/CPad.h>
#include <game/CCam.h>
#include <game/RenderWare.h>
#include <algorithm>

/**
 * CTModCameraManager
 *
 * Garry's Mod / Source Engine benzeri First Person (FPV) ve Third Person (TPV) kamera modlarini yonetir.
 *
 * Neden var?
 * 1. Orijinal GTA SA kamerasi karakterin cevresinde serbest donerken karakterin yonunu degistirmez,
 *    bu da FPS veya Source Engine tarzinda oynanisi bozar (karakterin arkasini gormek veya kafanin icine girmek gibi).
 * 2. GTA SA'nin kendi CCamera::Process() dongusu her karede CCam::Process_FollowPed calistirarak
 *    kamerayi oyuncunun ~2.5 - 4.0 metre arkasina iter. Sadece DoPulse() icerisinde matris degistirmek
 *    yetersizdir cunku GTA SA ardindan kamerayi ezer.
 * 3. Bu yonetici, hem CGame::Process() sonrasinda (IdleHandler) hem de 3D sahne cizilmeden once (PreRenderSkyHandler)
 *    kamerayi dogrudan oyuncunun kafa kemigine (BONE_HEAD) kilitler ve RenderWare RwFrame matrislerini
 *    senkronize ederek gercek First Person gorunumu saglar.
 */

void CTModCameraManager::Init()
{
    m_mode = ETModCameraMode::DEFAULT;
    m_bLocalPedAlphaHidden = false;
}

void CTModCameraManager::SetCameraMode(const std::string& mode)
{
    ETModCameraMode oldMode = m_mode;

    if (mode == "firstperson")
        m_mode = ETModCameraMode::FIRST_PERSON;
    else if (mode == "thirdperson")
        m_mode = ETModCameraMode::THIRD_PERSON;
    else
        m_mode = ETModCameraMode::DEFAULT;

    // First Person modundan cikildiginda yerel oyuncunun gizlenen gorunurlugunu (alpha) geri yukle
    if (oldMode == ETModCameraMode::FIRST_PERSON && m_mode != ETModCameraMode::FIRST_PERSON)
    {
        if (m_bLocalPedAlphaHidden && g_pClientGame && g_pClientGame->GetPlayerManager())
        {
            CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
            if (pLocalPlayer)
            {
                pLocalPlayer->SetAlpha(255);
                m_bLocalPedAlphaHidden = false;
            }
        }
    }

    // Standart GTA kamerasina donuldugunde kamera modunu varsayilana dondur
    if (m_mode == ETModCameraMode::DEFAULT)
    {
        CClientCamera* pCamera = g_pClientGame ? g_pClientGame->GetManager()->GetCamera() : nullptr;
        if (pCamera)
            pCamera->ToggleCameraFixedMode(false);
    }
}

std::string CTModCameraManager::GetCameraMode() const
{
    if (m_mode == ETModCameraMode::FIRST_PERSON)
        return "firstperson";
    if (m_mode == ETModCameraMode::THIRD_PERSON)
        return "thirdperson";
    return "default";
}

void CTModCameraManager::SetCameraOffset(float x, float y, float z)
{
    m_fOffsetX = x;
    m_fOffsetY = y;
    m_fOffsetZ = z;
}

void CTModCameraManager::SetCameraDistance(float distance)
{
    m_fDistance = distance;
}

void CTModCameraManager::DoPulse()
{
    if (m_mode == ETModCameraMode::DEFAULT)
        return;

    // Kamera pozisyon ve yonelim guncellemesini uygula
    UpdateCameraPreRender();
}

/**
 * UpdateCameraPreRender
 *
 * Neden bu fonksiyon var?
 * GTA SA'nin dahili CGame::Process() dongusu her karede TheCamera.Process() cagrisi yapar.
 * Takip kamerasi (FollowPed) aktifken GTA, kamera kaynagini (Source) pedin govde merkezinden
 * geriye dogru (Distance = 2.5m - 4.0m) hesaplar. Eger kamera pozisyonu yalnizca DoPulse()'da
 * degistirilirse, GTA bir sonraki karede bunu hemen uzerine yazar.
 *
 * Bu fonksiyon:
 * 1. GTA CGame::Process() bittikten hemen sonra (CClientGame::IdleHandler) ve
 * 2. 3D cizim baslamadan hemen once (CClientGame::PreRenderSkyHandler) calisarak:
 *    - GTA CCamera matrisini,
 *    - Aktif CCam::Source vektorunu,
 *    - RenderWare RwCamera frame LTM ve modelling pozisyonlarini
 * dogrudan BONE_HEAD (kafa kemigi) goz hizasina sabitler.
 */
void CTModCameraManager::UpdateCameraPreRender()
{
    if (m_mode == ETModCameraMode::DEFAULT)
        return;
    if (!g_pClientGame || !g_pClientGame->GetPlayerManager())
        return;

    CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
    if (!pLocalPlayer || !pLocalPlayer->GetGamePlayer())
        return;

    CClientCamera* pCamera = g_pClientGame->GetManager() ? g_pClientGame->GetManager()->GetCamera() : nullptr;
    if (!pCamera)
        return;

    // First Person modunda yerel oyuncunun GTA ped modelini gizle (kafa/boyun kameraya girmesin)
    if (m_mode == ETModCameraMode::FIRST_PERSON)
    {
        if (!m_bLocalPedAlphaHidden)
        {
            pLocalPlayer->SetAlpha(0);
            m_bLocalPedAlphaHidden = true;
        }
    }
    else if (m_bLocalPedAlphaHidden)
    {
        pLocalPlayer->SetAlpha(255);
        m_bLocalPedAlphaHidden = false;
    }

    // Kamera matrisini al (fare acilari GTA tarafindan guncellenmistir)
    CMatrix camMatrix;
    pCamera->GetMatrix(camMatrix);
    CVector vecForward = camMatrix.vFront;
    vecForward.Normalize();
    CVector vecUp = camMatrix.vUp;
    vecUp.Normalize();
    CVector vecRight = camMatrix.vRight;
    vecRight.Normalize();

    // Source Engine karakter yonelimi:
    // FPV ve TPV modlarinda farenin baktigi yatay yon (yaw) karakterin baktigi yonu belirler.
    // Ancak oyuncu Alt tusuna (Free-Look) basili tutuyorsa govde dondurulmez.
    bool bAltFreeLook = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    if (!pLocalPlayer->IsDead() && !pLocalPlayer->IsInVehicle() && !bAltFreeLook)
    {
        float fHorizLen = sqrtf(vecForward.fX * vecForward.fX + vecForward.fY * vecForward.fY);
        if (fHorizLen > 0.001f)
        {
            float camYaw = atan2f(-vecForward.fX, vecForward.fY);
            pLocalPlayer->SetCurrentRotation(camYaw, true);
            pLocalPlayer->SetTargetRotation(camYaw);
            if (pLocalPlayer->GetGamePlayer())
            {
                pLocalPlayer->GetGamePlayer()->SetCurrentRotation(camYaw);
                pLocalPlayer->GetGamePlayer()->SetTargetRotation(camYaw);
            }
        }
    }

    // Bas kemigi (BONE_HEAD) konumu. Eger kemik pozisyonu okunamadiysa ped kokune guvenli fallback uygula.
    CVector vecHeadPos(0.0f, 0.0f, 0.0f);
    if (!pLocalPlayer->GetBonePosition(BONE_HEAD, vecHeadPos) || vecHeadPos.LengthSquared() < 0.1f)
    {
        pLocalPlayer->GetPosition(vecHeadPos);
        vecHeadPos.fZ += 0.70f;  // Ped kokunden goz hizasina yaklasik ofset
    }

    CVector targetPos;
    if (m_mode == ETModCameraMode::FIRST_PERSON)
    {
        // Goz hizasi konumlandirmasi:
        // BONE_HEAD kemigi kafatasinin tam merkezindedir (kulaklar arasi).
        // Kamerayi dogrudan goz hizasina ve yuzun onune yerlestirmek icin:
        // Bakis yonunde ileri +0.12m ve yukari +0.08m ofset uygulanir.
        // Bu sayede kafatasinin icine girme engellenir ve gorus acisi gozlerle kusursuz eslesir.
        targetPos = vecHeadPos + (vecForward * 0.12f) + (vecUp * 0.08f);
        targetPos += (vecRight * m_fOffsetX) + (vecForward * m_fOffsetY) + (vecUp * m_fOffsetZ);

        // Pitch acisini [-89.0f, +89.0f] araligina sinirla (gimbal lock ve kafa donmesini engeller)
        float pitch = asinf(std::clamp(camMatrix.vFront.fZ, -0.9998f, 0.9998f)) * (180.0f / 3.14159265f);
        if (pitch > 89.0f || pitch < -89.0f)
        {
            float clampedPitch = std::clamp(pitch, -89.0f, 89.0f) * (3.14159265f / 180.0f);
            float horizLen = sqrtf(camMatrix.vFront.fX * camMatrix.vFront.fX + camMatrix.vFront.fY * camMatrix.vFront.fY);
            if (horizLen > 0.001f)
            {
                float newHoriz = cosf(clampedPitch);
                camMatrix.vFront.fX = (camMatrix.vFront.fX / horizLen) * newHoriz;
                camMatrix.vFront.fY = (camMatrix.vFront.fY / horizLen) * newHoriz;
                camMatrix.vFront.fZ = sinf(clampedPitch);
                camMatrix.vFront.Normalize();
            }
        }
    }
    else if (m_mode == ETModCameraMode::THIRD_PERSON)
    {
        // TPV: Kafadan geriye dogru distance kadar git
        targetPos = vecHeadPos - (vecForward * m_fDistance) + (vecRight * m_fOffsetX) + (vecUp * m_fOffsetY);
        targetPos.fZ += m_fOffsetZ;

        // Camera Clipping: Duvarlarin icine girmeyi onlemek icin Raycast at
        CColPoint*        pColPoint = nullptr;
        CEntity*          pEntity = nullptr;
        SLineOfSightFlags flags;
        flags.bCheckBuildings = true;
        flags.bCheckVehicles = true;
        flags.bCheckPeds = false;
        flags.bCheckObjects = true;

        CGame* pGame = g_pCore ? g_pCore->GetGame() : nullptr;
        if (pGame && pGame->GetWorld())
        {
            bool bHit = pGame->GetWorld()->ProcessLineOfSight(&vecHeadPos, &targetPos, &pColPoint, &pEntity, flags);

            if (bHit && pColPoint)
            {
                // Duvara carpiyorsa kamerayi carpma noktasina yaklastir
                const CVector& vecHit = pColPoint->GetPosition();
                CVector        vecDir = vecHit - vecHeadPos;
                float          fHitDist = vecDir.Length();
                if (fHitDist > 0.2f)
                    fHitDist -= 0.2f;
                else
                    fHitDist = 0.0f;

                targetPos = vecHeadPos + (vecForward * -fHitDist);
                pColPoint->Destroy();
            }
        }

        // 2. RenderWare disindaki Bullet Physics binalari ve haritalar icin raycast denetimi
        CVector bulletHitPos, bulletHitNorm;
        if (CTModPhysicsManager::GetSingleton().RayCast(vecHeadPos, targetPos, bulletHitPos, bulletHitNorm))
        {
            CVector vecDir = bulletHitPos - vecHeadPos;
            float   fHitDist = vecDir.Length();
            if (fHitDist > 0.2f)
                fHitDist -= 0.2f;
            else
                fHitDist = 0.0f;

            targetPos = vecHeadPos + (vecForward * -fHitDist);
        }
    }

    // Kamera pozisyonunu uygula:
    // 1. GTA CCamera matrisi
    // 2. Aktif CCam kaynagi (pCam->Source)
    // 3. RenderWare RwCamera frame LTM ve modelling matrisleri
    camMatrix.vPos = targetPos;
    CGame* pGame = g_pCore ? g_pCore->GetGame() : nullptr;
    if (pGame)
    {
        CCamera* pGtaCamera = pGame->GetCamera();
        if (pGtaCamera)
        {
            // GTA CCamera matrisini guncelle
            pGtaCamera->SetMatrix(&camMatrix);

            // Aktif CCam kaynak vektorunu guncelle (FollowPed'in uzerine yazdigi Source'u ez)
            CCam* pCam = pGtaCamera->GetCam(pGtaCamera->GetActiveCam());
            if (pCam)
            {
                CVector* pSource = pCam->GetSource();
                if (pSource)
                    *pSource = targetPos;
            }

            // RenderWare Camera Frame Matrix (RwFrame LTM ve Modelling) pozisyonlarini guncelle.
            // RwCameraBeginUpdate() sahneyi cizerken frame LTM'ini ters cevirerek D3D View matrisini olusturur.
            // LTM ve ondan hemen onceki 64-baytlik modelling matrisi guncellenerek D3D'nin kafa pozisyonundan
            // render almasi garanti edilir.
            RwMatrix* pLtm = pGtaCamera->GetLTM();
            if (pLtm)
            {
                pLtm->pos.x = targetPos.fX;
                pLtm->pos.y = targetPos.fY;
                pLtm->pos.z = targetPos.fZ;

                RwMatrix* pModelling = pLtm - 1;
                pModelling->pos.x = targetPos.fX;
                pModelling->pos.y = targetPos.fY;
                pModelling->pos.z = targetPos.fZ;
            }
        }
    }

    // MTA CClientCamera onbellek donusumlerini gecersiz kil (GetMatrix vb. sorgular yeni pozisyonu donsun)
    pCamera->InvalidateCachedTransforms();
}
