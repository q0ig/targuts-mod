#pragma once
#include <string>
#include <vector>
#include <map>
#include <d3d9.h>
#include <d3dx9.h>
#include "../rendering/CTModSkeletalMesh.h"

struct STModKeyframeVec3
{
    double      time;
    D3DXVECTOR3 value;
};

struct STModKeyframeQuat
{
    double         time;
    D3DXQUATERNION value;
};

struct STModNodeAnim
{
    std::string                    nodeName;
    std::vector<STModKeyframeVec3> positionKeys;
    std::vector<STModKeyframeQuat> rotationKeys;
    std::vector<STModKeyframeVec3> scalingKeys;
};

struct STModAnimation
{
    std::string                name;
    double                     duration;
    double                     ticksPerSecond;
    std::vector<STModNodeAnim> channels;
    std::map<std::string, int> channelMapping;
};

enum class ETModAnimState
{
    STATE_LOCOMOTION,
    STATE_JUMP,
    STATE_LANDING
};

class CTModAnimationManager
{
public:
    CTModAnimationManager(CTModSkeletalMesh* pMesh);
    ~CTModAnimationManager();

    bool LoadAnimation(const std::string& name, const std::string& filePath);

    // Updates blend tree and writes final local-to-world matrices to outBoneMatrices
    void Update(float deltaTime, float forwardSpeed, float strafeSpeed, bool bOnGround, bool bTriggerJump, std::vector<D3DXMATRIX>& outBoneMatrices);

    // Plays a single named animation at a specific playback time in seconds
    bool  PlaySingleAnimation(const std::string& animName, float animTimeInSeconds, std::vector<D3DXMATRIX>& outBoneMatrices);
    float GetAnimationDuration(const std::string& animName) const;

private:
    void CalculateBoneTransforms(STModAnimation* anim, double time, int boneIndex, const D3DXMATRIX& parentTransform, std::vector<D3DXMATRIX>& outMatrices);
    void BlendBoneTransforms(STModAnimation* anim1, double time1, STModAnimation* anim2, double time2, float blendFactor, int boneIndex,
                             const D3DXMATRIX& parentTransform, std::vector<D3DXMATRIX>& outMatrices);

    D3DXVECTOR3    InterpolatePosition(const STModNodeAnim& nodeAnim, double time);
    D3DXQUATERNION InterpolateRotation(const STModNodeAnim& nodeAnim, double time);
    D3DXVECTOR3    InterpolateScaling(const STModNodeAnim& nodeAnim, double time);

    CTModSkeletalMesh*                    m_pMesh;
    std::map<std::string, STModAnimation> m_animations;

    ETModAnimState m_animState = ETModAnimState::STATE_LOCOMOTION;
    double         m_currentTime = 0.0;
    double         m_jumpTime = 0.0;
    float          m_landingBlendTime = 0.0f;
};
