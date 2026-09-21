#pragma once
#include <string>
#include <CVector.h>

class CClientPlayer;
class CControllerState;

class CTModMovementManager
{
public:
    static CTModMovementManager& GetSingleton()
    {
        static CTModMovementManager instance;
        return instance;
    }

    void Init();
    void DoPulse(float fDeltaTime);
    void Render();

    void        SetMovementMode(const std::string& mode);
    std::string GetMovementMode() const;
    void        SetAirAccelerate(float value);
    void        SetAutoBhop(bool enabled);

private:
    CTModMovementManager() = default;
    ~CTModMovementManager() = default;

    void Accelerate(CVector wishdir, float wishspeed, float accel, float fDeltaTime);

    std::string m_mode = "default";
    float       m_sv_accelerate = 10.0f;
    float       m_sv_airaccelerate = 100.0f;
    float       m_sv_friction = 4.0f;
    float       m_sv_maxspeed = 0.15f;
    float       m_sv_maxairspeed = 0.05f;
    float       m_jumpVelocity = 0.115f;
    bool        m_bAutoBhop = true;

    bool        m_bWasOnGround = true;
    std::string m_strActiveAnim = "";
    
    // Skeletal Mesh Pipeline
    class CTModSkeletalMesh* m_pSkeletalMesh = nullptr;
    class CTModAnimationManager* m_pAnimationManager = nullptr;
    
    class CTModSkeletalMesh* m_pFirstPersonMesh = nullptr;
};
