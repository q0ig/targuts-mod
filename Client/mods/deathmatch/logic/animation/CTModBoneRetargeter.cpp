#include "StdInc.h"
#include "CTModBoneRetargeter.h"
#include "../rendering/CTModSkeletalMesh.h"
#include "CClientPlayer.h"
#include <game/RenderWare.h> // MTA RenderWare structs
#include <game/CGame.h>

typedef RpHAnimHierarchy*(__cdecl* GetAnimHierarchyFromSkinClump_t)(RpClump*);
#define GetAnimHierarchyFromSkinClump ((GetAnimHierarchyFromSkinClump_t)0x734A40)

// Instead of calling a RW function, we will just access the struct field later.

// RenderWare matrix to D3DXMATRIX
static D3DXMATRIX RwMatrixToD3DXMATRIX(const RwMatrix* rwMat)
{
    D3DXMATRIX m;
    m._11 = rwMat->right.x; m._12 = rwMat->right.y; m._13 = rwMat->right.z; m._14 = 0.0f;
    m._21 = rwMat->up.x;    m._22 = rwMat->up.y;    m._23 = rwMat->up.z;    m._24 = 0.0f;
    m._31 = rwMat->at.x;    m._32 = rwMat->at.y;    m._33 = rwMat->at.z;    m._34 = 0.0f;
    m._41 = rwMat->pos.x;   m._42 = rwMat->pos.y;   m._43 = rwMat->pos.z;   m._44 = 1.0f;
    return m;
}

void CTModBoneRetargeter::ApplyUpperBodyRetargeting(CClientPlayer* pPlayer, CTModSkeletalMesh* pMesh, std::vector<D3DXMATRIX>& outBoneMatrices)
{
    if (!pPlayer || !pMesh || outBoneMatrices.empty()) return;

    // Check if player is aiming (task type etc.)
    // For now, always try to sync spine and arms for testing.
    
    RpClump* clump = pPlayer->GetClump();
    if (!clump) return;

    RpHAnimHierarchy* hAnimHier = GetAnimHierarchyFromSkinClump(clump);
    if (!hAnimHier) return;

    RwMatrix* boneMatrices = hAnimHier->pMatrixArray;
    if (!boneMatrices) return;

    // Mapping between Mixamo bones and GTA SA Bone IDs
    // GTA SA Bone IDs (BONE_SPINE1 = 2, BONE_HEAD = 5, BONE_RIGHTSHOULDER = 22, BONE_LEFTSHOULDER = 32, etc.)
    // We will just do a basic test on "mixamorig:Spine" mapped to BONE_SPINE1 (2)
    // and "mixamorig:RightArm" to BONE_RIGHTARM (23), etc.
    
    struct BoneMapping {
        std::string mixamoName;
        int gtaBoneID;
    };
    
    std::vector<BoneMapping> mappings = {
        {"mixamorig:Spine", 2},
        {"mixamorig:Spine1", 3},
        {"mixamorig:Spine2", 4},
        {"mixamorig:Neck", 5}, // Head
        {"mixamorig:Head", 5},
        {"mixamorig:RightShoulder", 22},
        {"mixamorig:RightArm", 23},
        {"mixamorig:RightForeArm", 24},
        {"mixamorig:RightHand", 25},
        {"mixamorig:LeftShoulder", 32},
        {"mixamorig:LeftArm", 33},
        {"mixamorig:LeftForeArm", 34},
        {"mixamorig:LeftHand", 35},
        {"mixamorig:RightUpLeg", 51},
        {"mixamorig:RightLeg", 52},
        {"mixamorig:RightFoot", 53},
        {"mixamorig:RightToeBase", 54},
        {"mixamorig:LeftUpLeg", 41},
        {"mixamorig:LeftLeg", 42},
        {"mixamorig:LeftFoot", 43},
        {"mixamorig:LeftToeBase", 44}
    };

    auto& boneMap = pMesh->GetBoneMap();
    auto& bones = pMesh->GetBones();

    for (const auto& mapping : mappings) {
        auto it = boneMap.find(mapping.mixamoName);
        if (it != boneMap.end()) {
            int meshBoneIdx = it->second;
            
            // Note: GTA matrices are global space (world space).
            // D3DXMATRIX needs to be in local space relative to the character root, 
            // OR we can just overwrite the final matrix if we adjust for the character's root transform.
            // outBoneMatrices expects: OffsetMatrix * GlobalTransform.
            
            // To properly do it:
            // 1. Get GTA Bone Global Matrix (rwMat)
            // 2. Convert to Local Space relative to GTA Ped (by multiplying with Inverse Ped Matrix)
            // 3. Multiply by Assimp's bone offset matrix
            
            D3DXMATRIX gtaBoneWorld = RwMatrixToD3DXMATRIX(&boneMatrices[mapping.gtaBoneID]);
            
            // Get Ped World Matrix
            CVector pedPos; pPlayer->GetPosition(pedPos);
            CVector pedRot; pPlayer->GetRotationDegrees(pedRot);
            
            D3DXMATRIX pedWorld, pedWorldInv;
            D3DXMatrixRotationYawPitchRoll(&pedWorld, pedRot.fZ, pedRot.fY, pedRot.fX);
            pedWorld._41 = pedPos.fX; pedWorld._42 = pedPos.fY; pedWorld._43 = pedPos.fZ;
            D3DXMatrixInverse(&pedWorldInv, nullptr, &pedWorld);
            
            D3DXMATRIX localGtaBone = gtaBoneWorld * pedWorldInv;
            
            // Mixamo bones usually have a different orientation (Y-up vs Z-up). 
            // We'll apply a crude coordinate conversion (swap axes) if needed.
            // For now, directly inject the local transform.
            outBoneMatrices[meshBoneIdx] = bones[meshBoneIdx].offsetMatrix * localGtaBone;
        }
    }
}
