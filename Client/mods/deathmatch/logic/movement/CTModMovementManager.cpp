#include "StdInc.h"
#include "CTModMovementManager.h"
#include "../camera/CTModCameraManager.h"
#include "../physics/CTModPhysicsManager.h"
#include "../rendering/CTModViewmodelManager.h"
#include "CClientGame.h"
#include "CClientPlayerManager.h"
#include "CClientPlayer.h"
#include "CClientPed.h"
#include "CClientCamera.h"
#include "CStaticFunctionDefinitions.h"
#include <game/CPlayerPed.h>
#include <game/CPad.h>
#include <game/Task.h>
#include <game/CTaskManager.h>
#include <game/TaskTypes.h>

/**
 * CTModMovementManager
 *
 * Source Engine / Garry's Mod hareket fizigini (sv_accelerate, air-strafe, bhop)
 * ve Source tarzı yonelme/strafe (karakter hareket yonune bakar) mekanigini saglar.
 *
 * Hareket Mantigi:
 * - Duruyorken: Kamera serbestce donebilir, karakter donmez.
 * - Yururken (WASD): Karakter kamera yonu + tus girdisinden hesaplanan hareket yonune
 *   doner ve GTA'nin default yurume animasyonuyla o yone dogru ilerler.
 *   Sola/saga/geriye kosmak yerine karakter her zaman ileri bakar ve yurur.
 */

void CTModMovementManager::Init()
{
    m_mode = "default";
    m_bWasOnGround = true;
    m_strActiveAnim = "";
}

#include "../rendering/CTModSkeletalMesh.h"
#include "../animation/CTModAnimationManager.h"
#include "../animation/CTModBoneRetargeter.h"
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

static std::string ResolveCharacterAssetPath(const std::string& relPath)
{
    std::string              basePath = GetTMODBinPath();
    std::vector<std::string> candidates = {
        basePath + relPath,
        CalcMTASAPath(relPath),
        relPath,
        basePath + "Bin/" + relPath,
        CalcMTASAPath("Bin/" + relPath),
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

void CTModMovementManager::SetMovementMode(const std::string& mode)
{
    if (mode == "source")
    {
        m_mode = "source";

        // Lazy load skeletal meshes
        if (!m_pSkeletalMesh && g_pCore && g_pCore->GetGraphics() && g_pCore->GetGraphics()->GetDevice())
        {
            m_pSkeletalMesh = new CTModSkeletalMesh();
            std::string fbxPath = ResolveCharacterAssetPath("character/Male07/Idle.fbx");
            if (!m_pSkeletalMesh->LoadFBX(fbxPath, g_pCore->GetGraphics()->GetDevice()))
            {
                char errBuf[512];
                sprintf_s(errBuf, "[TMOD-ERROR] Failed to load character FBX: %s\n", fbxPath.c_str());
                OutputDebugStringA(errBuf);
                if (g_pCore && g_pCore->GetConsole())
                {
                    g_pCore->GetConsole()->Printf("%s", errBuf);
                }

                delete m_pSkeletalMesh;
                m_pSkeletalMesh = nullptr;
            }
            else
            {
                // Multi-texture subsets (face/head, body/clothes, eyes, mouth) are now automatically
                // resolved and mapped inside LoadFBX. We also retain LoadTexture as the base/fallback texture.
                std::string texPath = ResolveCharacterAssetPath("character/Male07/textures/ropa_del_email_serote_siete.png");
                m_pSkeletalMesh->LoadTexture(texPath, g_pCore->GetGraphics()->GetDevice());
                m_pAnimationManager = new CTModAnimationManager(m_pSkeletalMesh);
                m_pAnimationManager->LoadAnimation("Idle", ResolveCharacterAssetPath("character/Male07/Idle.fbx"));
                m_pAnimationManager->LoadAnimation("Walking", ResolveCharacterAssetPath("character/Walking.fbx"));
                m_pAnimationManager->LoadAnimation("WalkingBackwards", ResolveCharacterAssetPath("character/WalkingBackwards.fbx"));
                m_pAnimationManager->LoadAnimation("RightStrafeWalk", ResolveCharacterAssetPath("character/RightStrafeWalk.fbx"));
                m_pAnimationManager->LoadAnimation("LeftStrafeWalk", ResolveCharacterAssetPath("character/LeftStrafeWalk.fbx"));
                m_pAnimationManager->LoadAnimation("Jumping", ResolveCharacterAssetPath("character/Jumping.fbx"));
            }
        }
        if (!m_pFirstPersonMesh && g_pCore && g_pCore->GetGraphics() && g_pCore->GetGraphics()->GetDevice())
        {
            m_pFirstPersonMesh = new CTModSkeletalMesh();
            std::string fbxPath = ResolveCharacterAssetPath("first-person/source/arms.fbx");
            if (!m_pFirstPersonMesh->LoadFBX(fbxPath, g_pCore->GetGraphics()->GetDevice()))
            {
                char errBuf[512];
                sprintf_s(errBuf, "[TMOD-ERROR] Failed to load first-person FBX: %s\n", fbxPath.c_str());
                OutputDebugStringA(errBuf);

                delete m_pFirstPersonMesh;
                m_pFirstPersonMesh = nullptr;
            }
            else
            {
                m_pFirstPersonMesh->LoadTexture(ResolveCharacterAssetPath("first-person/textures/armColor.png"), g_pCore->GetGraphics()->GetDevice());
            }
        }
    }
    else
    {
        // Switching back to default GTA movement. Clean up all Source mode state
        // so the player isn't left invisible, frozen, or with dangling pointers.
        std::string prevMode = m_mode;
        m_mode = "default";

        // Restore GTA ped visibility — Source mode sets alpha to 0 every frame
        CClientPlayerManager* pPlayerManager = g_pClientGame ? g_pClientGame->GetPlayerManager() : nullptr;
        if (pPlayerManager)
        {
            CClientPlayer* pLocalPlayer = pPlayerManager->GetLocalPlayer();
            if (pLocalPlayer && pLocalPlayer->GetGamePlayer())
            {
                pLocalPlayer->GetGamePlayer()->SetAlpha(255);
            }
            // Unfreeze the player so GTA controls work again
            if (pLocalPlayer)
            {
                pLocalPlayer->SetFrozen(false);
            }
        }

        // Clean up skeletal mesh resources to free GPU memory
        if (m_pAnimationManager)
        {
            delete m_pAnimationManager;
            m_pAnimationManager = nullptr;
        }
        if (m_pSkeletalMesh)
        {
            delete m_pSkeletalMesh;
            m_pSkeletalMesh = nullptr;
        }
        if (m_pFirstPersonMesh)
        {
            delete m_pFirstPersonMesh;
            m_pFirstPersonMesh = nullptr;
        }

        // Reset movement state so re-entering source mode starts clean
        m_bWasOnGround = true;
        m_strActiveAnim = "";
    }
}

std::string CTModMovementManager::GetMovementMode() const
{
    return m_mode;
}
void CTModMovementManager::SetAirAccelerate(float value)
{
    m_sv_airaccelerate = value;
}
void CTModMovementManager::SetAutoBhop(bool enabled)
{
    m_bAutoBhop = enabled;
}

// Source Engine Vectorial Projection - Quake / Source sv_accelerate formulu
void CTModMovementManager::Accelerate(CVector wishdir, float wishspeed, float accel, float fDeltaTime)
{
    if (!g_pClientGame)
        return;
    CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
    if (!pLocalPlayer)
        return;

    CVector velocity;
    pLocalPlayer->GetMoveSpeed(velocity);

    float currentspeed = velocity.DotProduct(&wishdir);
    float addspeed = wishspeed - currentspeed;
    if (addspeed <= 0)
        return;

    float accelspeed = accel * fDeltaTime * wishspeed;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    velocity.fX += accelspeed * wishdir.fX;
    velocity.fY += accelspeed * wishdir.fY;
    pLocalPlayer->SetMoveSpeed(velocity);
}

void CTModMovementManager::DoPulse(float fDeltaTime)
{
    CTModViewmodelManager::GetSingleton().DoPulse(fDeltaTime);

    if (!g_pClientGame)
        return;

    CClientPlayer* pLocalPlayer = g_pClientGame->GetPlayerManager()->GetLocalPlayer();
    if (!pLocalPlayer || !pLocalPlayer->GetGamePlayer())
        return;
    if (pLocalPlayer->IsDead() || pLocalPlayer->IsInVehicle())
        return;

    bool bCustomCam = (CTModCameraManager::GetSingleton().GetMode() != ETModCameraMode::DEFAULT);
    bool bSourceMove = (m_mode == "source");
    bool bStrafeMode = bSourceMove || bCustomCam;

    CClientCamera* pCamera = g_pClientGame->GetManager()->GetCamera();
    if (!pCamera)
        return;

    CMatrix camMatrix;
    pCamera->GetMatrix(camMatrix);

    CVector forward = camMatrix.vFront;
    forward.fZ = 0.0f;
    forward.Normalize();

    CVector right = camMatrix.vRight;
    right.fZ = 0.0f;
    right.Normalize();

    CControllerState cs;
    pLocalPlayer->GetControllerState(cs);

    // Klavye tuslarini ve ControllerState'i birlestirerek tus algilama dalgalanmasini onle
    bool keyW = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool keyS = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool keyA = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool keyD = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool keySprint = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool keyJump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    bool bForward = keyW || (cs.LeftStickY < -32);
    bool bBackward = keyS || (cs.LeftStickY > 32);
    bool bLeft = keyA || (cs.LeftStickX < -32);
    bool bRight = keyD || (cs.LeftStickX > 32);
    bool bSprint = keySprint || (cs.ButtonCross != 0);
    bool bJump = keyJump || (cs.ButtonSquare != 0);

    bool bOnGround = pLocalPlayer->IsOnGround(true);
    if (!bOnGround && CTModPhysicsManager::GetSingleton().IsPlayerOnBulletGround())
    {
        bOnGround = true;
    }

    // Hareket yon vektorunu hesapla: kamera yonu + WASD girdisi
    CVector wishdir(0, 0, 0);
    if (bForward)
        wishdir += forward;
    if (bBackward)
        wishdir -= forward;
    if (bLeft)
        wishdir -= right;
    if (bRight)
        wishdir += right;

    float wishLen = wishdir.Length();
    if (wishLen > 0.0f)
        wishdir.Normalize();

    // KARAKTER YONELIMI (Source Tarzi Govde Yonelimi & Alt Free-Look)
    // - Karakterin govdesi kesinlikle basilan WASD tusunun yonune donmez.
    // - Govde daima kameranin yatay acisina (Camera Yaw) kilitli kalir.
    // - Sol Alt tusuna basili tutuldugunda (Free-Look) govde donmez, en son baktigi acida kilitli kalir.
    //   Kamera serbestce 360 derece doner, Alt birakildiginda karakter kamera yonune kilitlenir.
    bool bAltFreeLook = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    if (bStrafeMode && !bAltFreeLook)
    {
        float camYaw = atan2(-forward.fX, forward.fY);
        pLocalPlayer->SetCurrentRotation(camYaw, true);
        pLocalPlayer->SetTargetRotation(camYaw);
        if (pLocalPlayer->GetGamePlayer())
        {
            pLocalPlayer->GetGamePlayer()->SetCurrentRotation(camYaw);
            pLocalPlayer->GetGamePlayer()->SetTargetRotation(camYaw);
        }
    }

    // Eger Source hareket modu kapaliysa ama ozel kamera strafe modundaysak,
    // WASD tuslarinda kamera yonunde ilerlet
    if (!bSourceMove && bStrafeMode)
    {
        if (bOnGround)
        {
            if (wishLen > 0.0f)
            {
                float   moveSpeed = bSprint ? 0.16f : 0.08f;
                CVector curVel;
                pLocalPlayer->GetMoveSpeed(curVel);
                curVel.fX = wishdir.fX * moveSpeed;
                curVel.fY = wishdir.fY * moveSpeed;
                pLocalPlayer->SetMoveSpeed(curVel);
            }
            else
            {
                CVector curVel;
                pLocalPlayer->GetMoveSpeed(curVel);
                if (curVel.fX != 0.0f || curVel.fY != 0.0f)
                {
                    curVel.fX *= 0.5f;
                    curVel.fY *= 0.5f;
                    if (fabs(curVel.fX) < 0.005f)
                        curVel.fX = 0.0f;
                    if (fabs(curVel.fY) < 0.005f)
                        curVel.fY = 0.0f;
                    pLocalPlayer->SetMoveSpeed(curVel);
                }
            }
        }
    }

    // SOURCE HAREKET FIZIGI (Bunnyhop, Air-Strafe, Sürtünme ve İvmelenme)
    // Eger oyuncunun carpismalari kapaliysa (orn. /fly modu, noclip) veya donmussa,
    // hareket fizigi mudahale etmesin.
    bool bCollisionsDisabled = !pLocalPlayer->GetUsesCollision() || pLocalPlayer->IsFrozen();
    bool bTriggeredJumpThisFrame = false;
    if (bSourceMove && !bCollisionsDisabled)
    {
        static bool s_bLastJump = false;
        bool        bJumpJustPressed = bJump && !s_bLastJump;
        s_bLastJump = bJump;

        if (bOnGround)
        {
            // Zemin Sürtünmesi: Tuş bırakıldığında yumuşak ve kontrollü durma
            CVector vel;
            pLocalPlayer->GetMoveSpeed(vel);
            float speed = sqrt(vel.fX * vel.fX + vel.fY * vel.fY);
            if (speed > 0 && wishLen == 0)
            {
                float drop = speed * m_sv_friction * fDeltaTime;
                float newspeed = speed - drop;
                if (newspeed < 0)
                    newspeed = 0;
                newspeed /= speed;
                vel.fX *= newspeed;
                vel.fY *= newspeed;
                pLocalPlayer->SetMoveSpeed(vel);
            }

            // Zemin İvmelenmesi
            if (wishLen > 0)
                Accelerate(wishdir, m_sv_maxspeed, m_sv_accelerate, fDeltaTime);

            // Bunnyhop: Space basiliyken veya yere inildigi anda
            bool bTriggerJump = bJumpJustPressed || (m_bAutoBhop && bJump && !m_bWasOnGround);
            if (bTriggerJump)
            {
                bTriggeredJumpThisFrame = true;
                CVector jumpVel;
                pLocalPlayer->GetMoveSpeed(jumpVel);
                jumpVel.fZ = m_jumpVelocity;
                pLocalPlayer->SetMoveSpeed(jumpVel);

                // GTA'nin iniş gecikmesini (landing lag) kaldır
                CTaskManager* pTaskManager = pLocalPlayer->GetTaskManager();
                if (pTaskManager)
                {
                    CTask* pSimplest = pTaskManager->GetSimplestActiveTask();
                    if (pSimplest && pSimplest->GetTaskType() == TASK_SIMPLE_LAND)
                    {
                        pSimplest->MakeAbortable(pLocalPlayer->GetGamePlayer(), ABORT_PRIORITY_URGENT, nullptr);
                    }
                }
            }
            m_bWasOnGround = true;
        }
        else
        {
            // Havadayken (In Air):
            m_bWasOnGround = false;

            // Air Strafe: Havadayken A ve D ile yon degistirme ve ivme kazanma
            if (wishLen > 0)
                Accelerate(wishdir, m_sv_maxairspeed, m_sv_airaccelerate, fDeltaTime);
        }
    }

    // Skeletal Mesh Pipeline Update
    if (bSourceMove && m_pSkeletalMesh && m_pAnimationManager)
    {
        // Only hide GTA ped if our custom mesh actually loaded successfully
        if (pLocalPlayer->GetGamePlayer())
        {
            pLocalPlayer->GetGamePlayer()->SetAlpha(0);
        }

        // Calculate speeds relative to camera forward/right
        CVector vel;
        pLocalPlayer->GetMoveSpeed(vel);
        float fwdSpd = vel.DotProduct(&forward) * 50.0f;  // Scale to human speeds ~0-5
        float strSpd = vel.DotProduct(&right) * 50.0f;

        std::vector<D3DXMATRIX> boneTransforms;
        m_pAnimationManager->Update(fDeltaTime, fwdSpd, strSpd, bOnGround, bTriggeredJumpThisFrame, boneTransforms);

        // Upload final matrices to GPU (retargeting disabled to preserve authentic FBX animations)
        if (g_pCore && g_pCore->GetGraphics() && g_pCore->GetGraphics()->GetDevice())
        {
            m_pSkeletalMesh->UpdateBoneMatrices(g_pCore->GetGraphics()->GetDevice(), boneTransforms);
        }
    }
}

void CTModMovementManager::Render()
{
    if (m_mode == "source" && m_pSkeletalMesh && g_pCore && g_pCore->GetGraphics() && g_pCore->GetGraphics()->GetDevice())
    {
        CClientPlayerManager* pPlayerManager = g_pClientGame ? g_pClientGame->GetPlayerManager() : nullptr;
        if (!pPlayerManager)
            return;
        CClientPlayer* pLocalPlayer = pPlayerManager->GetLocalPlayer();
        if (!pLocalPlayer || pLocalPlayer->IsDead())
            return;

        IDirect3DDevice9*     pDevice = g_pCore->GetGraphics()->GetDevice();
        IDirect3DStateBlock9* pSavedState = nullptr;
        pDevice->CreateStateBlock(D3DSBT_ALL, &pSavedState);

        // Fetch GTA ped orientation and position directly via CMatrix
        CMatrix pedMatrix;
        pLocalPlayer->GetMatrix(pedMatrix);

        // Idle.fbx height is ~1.37m (meters, not centimeters). Scaling by 1.25 brings the character
        // to ~1.71m tall, which matches GTA SA standard ped height.
        // In the local FBX mesh coordinate system:
        //   +X = Left, so -vRight maps character's left to GTA's left (+vRight is GTA ped's right)
        //   +Y = Up (feet at 0, head at 1.37m), so +vUp maps to GTA's vertical axis
        //   +Z = Forward (heel to toes), so +vFront maps to GTA ped's forward vector
        // Feet are placed at ped's base level (pelvis z minus 0.95m).
        const float modelScale = 1.25f;

        D3DXMATRIX worldMat;
        D3DXMatrixIdentity(&worldMat);

        // Row 1: Local X maps to character's left (-pedMatrix.vRight)
        worldMat._11 = -pedMatrix.vRight.fX * modelScale;
        worldMat._12 = -pedMatrix.vRight.fY * modelScale;
        worldMat._13 = -pedMatrix.vRight.fZ * modelScale;
        worldMat._14 = 0.0f;

        // Row 2: Local Y maps to character's up (+pedMatrix.vUp)
        worldMat._21 = pedMatrix.vUp.fX * modelScale;
        worldMat._22 = pedMatrix.vUp.fY * modelScale;
        worldMat._23 = pedMatrix.vUp.fZ * modelScale;
        worldMat._24 = 0.0f;

        // Row 3: Local Z maps to character's forward (+pedMatrix.vFront)
        worldMat._31 = pedMatrix.vFront.fX * modelScale;
        worldMat._32 = pedMatrix.vFront.fY * modelScale;
        worldMat._33 = pedMatrix.vFront.fZ * modelScale;
        worldMat._34 = 0.0f;

        // Row 4: Ground level position (feet at bottom of ped, pelvis - 0.95m)
        worldMat._41 = pedMatrix.vPos.fX - 0.95f * pedMatrix.vUp.fX;
        worldMat._42 = pedMatrix.vPos.fY - 0.95f * pedMatrix.vUp.fY;
        worldMat._43 = pedMatrix.vPos.fZ - 0.95f * pedMatrix.vUp.fZ;
        worldMat._44 = 1.0f;

        // Query GTA's active camera View and Projection matrices directly from the D3D9 device.
        // This avoids calculating custom LH matrices that mirror the horizontal axis or mismatch
        // GTA SA's active depth buffer parameters.
        D3DMATRIX matGtaView, matGtaProj;
        pDevice->GetTransform(D3DTS_VIEW, &matGtaView);
        pDevice->GetTransform(D3DTS_PROJECTION, &matGtaProj);
        D3DXMATRIX viewProj = (*(const D3DXMATRIX*)&matGtaView) * (*(const D3DXMATRIX*)&matGtaProj);
        pDevice->SetVertexShaderConstantF(0, (const float*)&viewProj, 4);

        // Configure 3D Render States
        pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

        bool bFirstPerson = (CTModCameraManager::GetSingleton().GetMode() == ETModCameraMode::FIRST_PERSON);
        if (bFirstPerson)
        {
            // İSTEMCİ GÖRÜNÜRLÜK İZOLASYONU (Local Visibility Filtering):
            // First Person (FPS) modundayken yerel oyuncunun 3. şahıs Male07 modeli çizilmez.
            // Bu sayede kafa, boyun ve omuzların kameranın içine girmesi engellenir.
            // Viewmodel elleri sadece yerel istemcide ve Camera Space derinlik izolasyonuyla çizilir:
            CTModViewmodelManager::GetSingleton().Render(pDevice);
        }
        else
        {
            // 3. Şahıs Görünümü: Male07 karakterini renderla
            pDevice->SetVertexShaderConstantF(4, (float*)&worldMat, 4);
            m_pSkeletalMesh->Render(pDevice);
        }

        if (pSavedState)
        {
            pSavedState->Apply();
            pSavedState->Release();
        }
    }
}
