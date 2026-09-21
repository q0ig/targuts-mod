#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

class CClientPlayer;
class CTModSkeletalMesh;

class CTModBoneRetargeter
{
public:
    static void ApplyUpperBodyRetargeting(CClientPlayer* pPlayer, CTModSkeletalMesh* pMesh, std::vector<D3DXMATRIX>& outBoneMatrices);
};
