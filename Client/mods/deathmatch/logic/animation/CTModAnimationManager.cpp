#include "StdInc.h"
#include "CTModAnimationManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

CTModAnimationManager::CTModAnimationManager(CTModSkeletalMesh* pMesh) : m_pMesh(pMesh)
{
}

CTModAnimationManager::~CTModAnimationManager()
{
}

bool CTModAnimationManager::LoadAnimation(const std::string& name, const std::string& filePath)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(filePath, 0);

    if (!scene || !scene->HasAnimations())
    {
        return false;
    }

    aiAnimation*   aiAnim = scene->mAnimations[0];
    STModAnimation anim;
    anim.name = name;
    anim.duration = aiAnim->mDuration;
    anim.ticksPerSecond = aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 25.0f;

    for (unsigned int i = 0; i < aiAnim->mNumChannels; i++)
    {
        aiNodeAnim*   channel = aiAnim->mChannels[i];
        STModNodeAnim nodeAnim;
        nodeAnim.nodeName = channel->mNodeName.C_Str();

        for (unsigned int j = 0; j < channel->mNumPositionKeys; j++)
        {
            STModKeyframeVec3 key;
            key.time = channel->mPositionKeys[j].mTime;
            key.value = D3DXVECTOR3(channel->mPositionKeys[j].mValue.x, channel->mPositionKeys[j].mValue.y, channel->mPositionKeys[j].mValue.z);
            nodeAnim.positionKeys.push_back(key);
        }
        for (unsigned int j = 0; j < channel->mNumRotationKeys; j++)
        {
            STModKeyframeQuat key;
            key.time = channel->mRotationKeys[j].mTime;
            key.value = D3DXQUATERNION(channel->mRotationKeys[j].mValue.x, channel->mRotationKeys[j].mValue.y, channel->mRotationKeys[j].mValue.z,
                                       channel->mRotationKeys[j].mValue.w);
            nodeAnim.rotationKeys.push_back(key);
        }
        for (unsigned int j = 0; j < channel->mNumScalingKeys; j++)
        {
            STModKeyframeVec3 key;
            key.time = channel->mScalingKeys[j].mTime;
            key.value = D3DXVECTOR3(channel->mScalingKeys[j].mValue.x, channel->mScalingKeys[j].mValue.y, channel->mScalingKeys[j].mValue.z);
            nodeAnim.scalingKeys.push_back(key);
        }

        anim.channelMapping[nodeAnim.nodeName] = anim.channels.size();
        anim.channels.push_back(nodeAnim);
    }

    m_animations[name] = anim;
    return true;
}

D3DXVECTOR3 CTModAnimationManager::InterpolatePosition(const STModNodeAnim& nodeAnim, double time)
{
    if (nodeAnim.positionKeys.size() == 1)
        return nodeAnim.positionKeys[0].value;

    unsigned int p0Index = 0;
    for (unsigned int i = 0; i < nodeAnim.positionKeys.size() - 1; i++)
    {
        if (time < nodeAnim.positionKeys[i + 1].time)
        {
            p0Index = i;
            break;
        }
    }
    unsigned int p1Index = p0Index + 1;
    float        deltaTime = (float)(nodeAnim.positionKeys[p1Index].time - nodeAnim.positionKeys[p0Index].time);
    float        factor = (float)((time - nodeAnim.positionKeys[p0Index].time) / deltaTime);

    D3DXVECTOR3 out;
    D3DXVec3Lerp(&out, &nodeAnim.positionKeys[p0Index].value, &nodeAnim.positionKeys[p1Index].value, factor);
    return out;
}

D3DXQUATERNION CTModAnimationManager::InterpolateRotation(const STModNodeAnim& nodeAnim, double time)
{
    if (nodeAnim.rotationKeys.size() == 1)
        return nodeAnim.rotationKeys[0].value;

    unsigned int p0Index = 0;
    for (unsigned int i = 0; i < nodeAnim.rotationKeys.size() - 1; i++)
    {
        if (time < nodeAnim.rotationKeys[i + 1].time)
        {
            p0Index = i;
            break;
        }
    }
    unsigned int p1Index = p0Index + 1;
    float        deltaTime = (float)(nodeAnim.rotationKeys[p1Index].time - nodeAnim.rotationKeys[p0Index].time);
    float        factor = (float)((time - nodeAnim.rotationKeys[p0Index].time) / deltaTime);

    D3DXQUATERNION out;
    D3DXQuaternionSlerp(&out, &nodeAnim.rotationKeys[p0Index].value, &nodeAnim.rotationKeys[p1Index].value, factor);
    D3DXQuaternionNormalize(&out, &out);
    return out;
}

D3DXVECTOR3 CTModAnimationManager::InterpolateScaling(const STModNodeAnim& nodeAnim, double time)
{
    if (nodeAnim.scalingKeys.size() == 1)
        return nodeAnim.scalingKeys[0].value;

    unsigned int p0Index = 0;
    for (unsigned int i = 0; i < nodeAnim.scalingKeys.size() - 1; i++)
    {
        if (time < nodeAnim.scalingKeys[i + 1].time)
        {
            p0Index = i;
            break;
        }
    }
    unsigned int p1Index = p0Index + 1;
    float        deltaTime = (float)(nodeAnim.scalingKeys[p1Index].time - nodeAnim.scalingKeys[p0Index].time);
    float        factor = (float)((time - nodeAnim.scalingKeys[p0Index].time) / deltaTime);

    D3DXVECTOR3 out;
    D3DXVec3Lerp(&out, &nodeAnim.scalingKeys[p0Index].value, &nodeAnim.scalingKeys[p1Index].value, factor);
    return out;
}

static inline bool IsRootMotionBone(const std::string& name, int parentIndex)
{
    if (name.find("Hips") != std::string::npos || name.find("hips") != std::string::npos || name.find("Root") != std::string::npos ||
        name.find("root") != std::string::npos || name.find("Pelvis") != std::string::npos || name.find("pelvis") != std::string::npos)
    {
        return true;
    }
    return false;
}

void CTModAnimationManager::CalculateBoneTransforms(STModAnimation* anim, double time, int boneIndex, const D3DXMATRIX& parentTransform,
                                                    std::vector<D3DXMATRIX>& outMatrices)
{
    if (boneIndex < 0 || boneIndex >= (int)m_pMesh->GetBones().size())
        return;

    STModBone& bone = m_pMesh->GetBones()[boneIndex];
    D3DXMATRIX nodeTransform;
    D3DXMatrixIdentity(&nodeTransform);

    auto it = anim->channelMapping.find(bone.name);
    if (it != anim->channelMapping.end())
    {
        const STModNodeAnim& nodeAnim = anim->channels[it->second];

        D3DXVECTOR3    scaling = InterpolateScaling(nodeAnim, time);
        D3DXQUATERNION rotation = InterpolateRotation(nodeAnim, time);
        D3DXVECTOR3    position = InterpolatePosition(nodeAnim, time);

        // ROOT MOTION ISOLATION:
        // Mixamo walk/run animations translate Hips along local Z (forward) and local X (drift),
        // causing loop teleport/jitter. Zero out horizontal X and Z, keep vertical Y (hip height + bobbing).
        if (IsRootMotionBone(bone.name, bone.parentIndex))
        {
            position.x = 0.0f;
            position.z = 0.0f;
        }

        D3DXMATRIX scaleM, rotM, posM;
        D3DXMatrixScaling(&scaleM, scaling.x, scaling.y, scaling.z);
        D3DXMatrixRotationQuaternion(&rotM, &rotation);
        D3DXMatrixTranslation(&posM, position.x, position.y, position.z);

        nodeTransform = scaleM * rotM * posM;
    }

    D3DXMATRIX globalTransform = nodeTransform * parentTransform;
    bone.finalMatrix = bone.offsetMatrix * globalTransform;
    outMatrices[boneIndex] = bone.finalMatrix;

    // Recurse to children
    for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
    {
        if (m_pMesh->GetBones()[i].parentIndex == boneIndex)
        {
            CalculateBoneTransforms(anim, time, i, globalTransform, outMatrices);
        }
    }
}

void CTModAnimationManager::BlendBoneTransforms(STModAnimation* anim1, double time1, STModAnimation* anim2, double time2, float blendFactor, int boneIndex,
                                                const D3DXMATRIX& parentTransform, std::vector<D3DXMATRIX>& outMatrices)
{
    if (boneIndex < 0 || boneIndex >= (int)m_pMesh->GetBones().size())
        return;

    STModBone& bone = m_pMesh->GetBones()[boneIndex];
    D3DXMATRIX nodeTransform;
    D3DXMatrixIdentity(&nodeTransform);

    auto it1 = anim1 ? anim1->channelMapping.find(bone.name) : anim1->channelMapping.end();
    auto it2 = anim2 ? anim2->channelMapping.find(bone.name) : anim2->channelMapping.end();

    bool has1 = (anim1 && it1 != anim1->channelMapping.end());
    bool has2 = (anim2 && it2 != anim2->channelMapping.end());

    if (has1 || has2)
    {
        D3DXVECTOR3    s1(1.0f, 1.0f, 1.0f), s2(1.0f, 1.0f, 1.0f);
        D3DXVECTOR3    pos1(0.0f, 0.0f, 0.0f), pos2(0.0f, 0.0f, 0.0f);
        D3DXQUATERNION r1(0.0f, 0.0f, 0.0f, 1.0f), r2(0.0f, 0.0f, 0.0f, 1.0f);

        if (has1)
        {
            const STModNodeAnim& nodeAnim1 = anim1->channels[it1->second];
            s1 = InterpolateScaling(nodeAnim1, time1);
            r1 = InterpolateRotation(nodeAnim1, time1);
            pos1 = InterpolatePosition(nodeAnim1, time1);
        }
        if (has2)
        {
            const STModNodeAnim& nodeAnim2 = anim2->channels[it2->second];
            s2 = InterpolateScaling(nodeAnim2, time2);
            r2 = InterpolateRotation(nodeAnim2, time2);
            pos2 = InterpolatePosition(nodeAnim2, time2);
        }
        if (!has1)
        {
            s1 = s2;
            r1 = r2;
            pos1 = pos2;
        }
        if (!has2)
        {
            s2 = s1;
            r2 = r1;
            pos2 = pos1;
        }

        D3DXVECTOR3    finalScale, finalPos;
        D3DXQUATERNION finalRot;
        D3DXVec3Lerp(&finalScale, &s1, &s2, blendFactor);
        D3DXQuaternionSlerp(&finalRot, &r1, &r2, blendFactor);
        D3DXVec3Lerp(&finalPos, &pos1, &pos2, blendFactor);

        // ROOT MOTION ISOLATION
        if (IsRootMotionBone(bone.name, bone.parentIndex))
        {
            finalPos.x = 0.0f;
            finalPos.z = 0.0f;
        }

        D3DXMATRIX scaleM, rotM, posM;
        D3DXMatrixScaling(&scaleM, finalScale.x, finalScale.y, finalScale.z);
        D3DXMatrixRotationQuaternion(&rotM, &finalRot);
        D3DXMatrixTranslation(&posM, finalPos.x, finalPos.y, finalPos.z);

        nodeTransform = scaleM * rotM * posM;
    }

    D3DXMATRIX globalTransform = nodeTransform * parentTransform;
    bone.finalMatrix = bone.offsetMatrix * globalTransform;
    outMatrices[boneIndex] = bone.finalMatrix;

    for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
    {
        if (m_pMesh->GetBones()[i].parentIndex == boneIndex)
        {
            BlendBoneTransforms(anim1, time1, anim2, time2, blendFactor, i, globalTransform, outMatrices);
        }
    }
}

void CTModAnimationManager::Update(float deltaTime, float forwardSpeed, float strafeSpeed, bool bOnGround, bool bTriggerJump,
                                   std::vector<D3DXMATRIX>& outBoneMatrices)
{
    outBoneMatrices.resize(m_pMesh->GetBones().size());
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);

    // Default to T-pose (Identity matrices) for all bones initially
    for (size_t i = 0; i < outBoneMatrices.size(); i++)
    {
        outBoneMatrices[i] = identity;
    }

    // --- STATE MACHINE TRANSITIONS ---
    if (m_animState == ETModAnimState::STATE_LOCOMOTION)
    {
        if (bTriggerJump || !bOnGround)
        {
            m_animState = ETModAnimState::STATE_JUMP;
            m_jumpTime = 0.0;
        }
    }
    else if (m_animState == ETModAnimState::STATE_JUMP)
    {
        if (bOnGround && m_jumpTime > 0.15)
        {
            m_animState = ETModAnimState::STATE_LANDING;
            m_landingBlendTime = 0.15f;
        }
    }
    else if (m_animState == ETModAnimState::STATE_LANDING)
    {
        if (!bOnGround)
        {
            m_animState = ETModAnimState::STATE_JUMP;
            m_jumpTime = 0.0;
        }
        else
        {
            m_landingBlendTime -= deltaTime;
            if (m_landingBlendTime <= 0.0f)
            {
                m_animState = ETModAnimState::STATE_LOCOMOTION;
            }
        }
    }

    // --- STATE 1: JUMP ANIMATION & LOCK ---
    if (m_animState == ETModAnimState::STATE_JUMP)
    {
        auto itJump = m_animations.find("Jumping");
        if (itJump != m_animations.end())
        {
            STModAnimation& anim = itJump->second;
            m_jumpTime += anim.ticksPerSecond * deltaTime;

            // Hold in midair tuck/apex as long as the character has not landed
            double maxJumpHold = anim.duration * 0.70;
            if (m_jumpTime > maxJumpHold && !bOnGround)
            {
                m_jumpTime = maxJumpHold;
            }
            else if (m_jumpTime >= anim.duration)
            {
                m_jumpTime = anim.duration - 0.001;
            }

            for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
            {
                if (m_pMesh->GetBones()[i].parentIndex == -1)
                {
                    CalculateBoneTransforms(&anim, m_jumpTime, i, identity, outBoneMatrices);
                }
            }
            return;
        }
    }

    // --- STATE 2: LOCOMOTION (8-WAY BLENDING & IDLE) ---
    float moveSpeed = sqrtf(forwardSpeed * forwardSpeed + strafeSpeed * strafeSpeed);

    if (moveSpeed < 0.1f)
    {
        // Standing Still: Idle animation
        auto itIdle = m_animations.find("Idle");
        if (itIdle != m_animations.end())
        {
            STModAnimation& anim = itIdle->second;
            if (anim.duration > 0.001f)
            {
                m_currentTime += anim.ticksPerSecond * deltaTime;
                m_currentTime = fmod(m_currentTime, anim.duration);

                for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
                {
                    if (m_pMesh->GetBones()[i].parentIndex == -1)
                    {
                        CalculateBoneTransforms(&anim, m_currentTime, i, identity, outBoneMatrices);
                    }
                }
            }
        }
    }
    else
    {
        // Direction and animation pairing
        // W: Walking, S: WalkingBackwards (facing forward, stepping back)
        // A: LeftStrafeWalk, D: RightStrafeWalk
        std::string fwdAnimName = (forwardSpeed >= 0.0f) ? "Walking" : "WalkingBackwards";
        std::string strafeAnimName = (strafeSpeed >= 0.0f) ? "RightStrafeWalk" : "LeftStrafeWalk";

        float absFwd = fabsf(forwardSpeed);
        float absStr = fabsf(strafeSpeed);

        auto itFwd = m_animations.find(fwdAnimName);
        if (itFwd == m_animations.end() && forwardSpeed < 0.0f)
            itFwd = m_animations.find("Walking");  // fallback

        auto itStr = m_animations.find(strafeAnimName);

        float playbackScale = moveSpeed / 2.5f;
        if (playbackScale < 0.6f)
            playbackScale = 0.6f;
        if (playbackScale > 1.6f)
            playbackScale = 1.6f;

        STModAnimation* pFwd = (itFwd != m_animations.end()) ? &itFwd->second : nullptr;
        STModAnimation* pStr = (itStr != m_animations.end()) ? &itStr->second : nullptr;

        double tps = pFwd ? pFwd->ticksPerSecond : 30.0;
        m_currentTime += tps * deltaTime * playbackScale;
        double dur = pFwd ? pFwd->duration : 30.0;
        if (dur > 0.001)
            m_currentTime = fmod(m_currentTime, dur);

        if (absStr < 0.05f || !pStr)
        {
            // Pure Forward / Backward
            if (pFwd && pFwd->duration > 0.001f)
            {
                for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
                {
                    if (m_pMesh->GetBones()[i].parentIndex == -1)
                    {
                        CalculateBoneTransforms(pFwd, m_currentTime, i, identity, outBoneMatrices);
                    }
                }
            }
        }
        else if (absFwd < 0.05f || !pFwd)
        {
            // Pure Strafe
            if (pStr && pStr->duration > 0.001f)
            {
                double strTime = fmod(m_currentTime, pStr->duration);
                for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
                {
                    if (m_pMesh->GetBones()[i].parentIndex == -1)
                    {
                        CalculateBoneTransforms(pStr, strTime, i, identity, outBoneMatrices);
                    }
                }
            }
        }
        else
        {
            // Diagonal Blending (W+A, W+D, S+A, S+D)
            float  blendFactor = absStr / (absFwd + absStr);  // 0.0 = 100% Forward/Back, 1.0 = 100% Strafe
            double strTime = pStr->duration > 0.001f ? fmod(m_currentTime, pStr->duration) : 0.0;

            for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
            {
                if (m_pMesh->GetBones()[i].parentIndex == -1)
                {
                    BlendBoneTransforms(pFwd, m_currentTime, pStr, strTime, blendFactor, i, identity, outBoneMatrices);
                }
            }
        }
    }
}

float CTModAnimationManager::GetAnimationDuration(const std::string& animName) const
{
    auto it = m_animations.find(animName);
    if (it == m_animations.end())
        return 0.0f;
    const STModAnimation& anim = it->second;
    if (anim.ticksPerSecond > 0.0)
        return (float)(anim.duration / anim.ticksPerSecond);
    return 0.0f;
}

bool CTModAnimationManager::PlaySingleAnimation(const std::string& animName, float animTimeInSeconds, std::vector<D3DXMATRIX>& outBoneMatrices)
{
    auto it = m_animations.find(animName);
    if (it == m_animations.end())
        return false;

    STModAnimation& anim = it->second;
    double          timeInTicks = animTimeInSeconds * anim.ticksPerSecond;
    if (timeInTicks >= anim.duration)
        timeInTicks = anim.duration - 0.001;
    if (timeInTicks < 0.0)
        timeInTicks = 0.0;

    outBoneMatrices.resize(m_pMesh->GetBones().size());
    D3DXMATRIX identity;
    D3DXMatrixIdentity(&identity);
    for (size_t i = 0; i < outBoneMatrices.size(); i++)
    {
        outBoneMatrices[i] = identity;
    }

    for (int i = 0; i < (int)m_pMesh->GetBones().size(); i++)
    {
        if (m_pMesh->GetBones()[i].parentIndex == -1)
        {
            CalculateBoneTransforms(&anim, timeInTicks, i, identity, outBoneMatrices);
        }
    }
    return true;
}
